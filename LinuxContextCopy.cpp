#include <iostream>
#include <cstring>
#include <vector>
#include <memory>
#include <xcb/xcb.h>

typedef std::vector<xcb_atom_t> atoms;
typedef std::shared_ptr<std::vector<uint8_t>> buffer_ptr;

enum ATOM_LIST {
	ATOM = 0,
	TARGETS,
	CLIPBOARD,
	CLIPBOARD_MANAGER,
	SAVE_TARGETS
};

#define COMMON_ATOMS_COUNT 5

xcb_atom_t common_atoms[COMMON_ATOMS_COUNT] = {0};
std::vector<std::pair<xcb_atom_t, buffer_ptr>> m_data;
xcb_connection_t* m_connection = nullptr;
xcb_window_t m_window = -1;

int connect_x11() {
	// xcb_connection_t* m_connection = nullptr;

	::m_connection = xcb_connect(nullptr, nullptr);
	if(::m_connection == nullptr) return 1;
	const xcb_setup_t* setup = xcb_get_setup(::m_connection);
	if (!setup) return 1;
	xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;
	if (!screen) return 1;

	uint32_t event_mask =
	// Just in case that some program reports SelectionNotify events
	// with XCB_EVENT_MASK_PROPERTY_CHANGE mask.
	XCB_EVENT_MASK_PROPERTY_CHANGE |
	// To receive DestroyNotify event and stop the message loop.
	XCB_EVENT_MASK_STRUCTURE_NOTIFY;

	// Temporal background window used to own the clipboard and process
	// all events related about the clipboard in a background thread
	::m_window = xcb_generate_id(::m_connection);
	xcb_create_window(m_connection, 0, ::m_window, screen->root, 0, 0, 1, 1, 0,
					XCB_WINDOW_CLASS_INPUT_OUTPUT,
					screen->root_visual,
					XCB_CW_EVENT_MASK,
					&event_mask);
	// if(::m_window) return 1;
	return 0;
}

void get_atoms(const char ** name, xcb_atom_t * atoms_list, int N = 0) {
	std::vector<xcb_intern_atom_cookie_t> cookies(N);

	for(int i = 0; i < N; i++)
		cookies[i] = xcb_intern_atom(m_connection, false, std::strlen(name[i]), name[i]);
	for(int i = 0; i < N; i++) {
		xcb_intern_atom_reply_t * reply = xcb_intern_atom_reply(m_connection, cookies[i], nullptr);
		if(reply) {
			atoms_list[i] = reply->atom;
			free(reply);
		}
	}

	return;
}

void get_common_atoms_list() {
	const char * str_common_atoms[] = {
		"ATOM",
		"TARGETS",
		"CLIPBOARD",
		"CLIPBOARD_MANAGER",
		"SAVE_TARGETS"
	};
	get_atoms(str_common_atoms, ::common_atoms, COMMON_ATOMS_COUNT);
}

void get_text_atoms_list(xcb_atom_t * text_atoms) {
	const char * str_text_atoms[] = {
		// utf-8 formats
		"UTF8_STRING",

		// Following formats are not required can be removed.
		"text/plain;charset=utf-8",
		"text/plain;charset=UTF-8",
		// Required for gedit (and maybe gtk+ apps)
		"GTK_TEXT_BUFFER_CONTENTS",
		// ANSI C strings
		"STRING",
		"TEXT",
		"text/plain"
	};
	get_atoms(str_text_atoms, text_atoms, 7);
}

void set_data(const char* data) {
	xcb_atom_t text_atoms[7] = { 0 };
	size_t data_len = std::strlen(data);
	get_text_atoms_list(text_atoms);
	buffer_ptr shared_data_buf = std::make_shared<std::vector<uint8_t>>(data_len);
	std::copy(data, data + data_len, shared_data_buf->begin());
	for (int i = 0; i < 7; i++) ::m_data.push_back({ text_atoms[i], shared_data_buf });
}

xcb_window_t get_x11_selection_owner() {
	xcb_window_t result = 0;
	xcb_get_selection_owner_cookie_t cookie = xcb_get_selection_owner(::m_connection, ::common_atoms[CLIPBOARD]);
	xcb_get_selection_owner_reply_t * reply = xcb_get_selection_owner_reply(m_connection, cookie, nullptr);
	if(reply) {
		result = reply->owner;
		free(reply);
	}
	return result;
}

int set_x11_selection_owner() {
	xcb_void_cookie_t cookie = xcb_set_selection_owner_checked(
		::m_connection,
		::m_window,
		::common_atoms[CLIPBOARD],
		XCB_CURRENT_TIME
	);
	xcb_generic_error_t* err = xcb_request_check(m_connection, cookie);
	if(err) {
		free(err);
		return 1;
	}
	return 0;
}

int set_data_clipboard() {
	if (::m_window != get_x11_selection_owner())
		return 1;
	// If the CLIPBOARD_MANAGER atom is not 0, we assume that there
	// is a clipboard manager available were we can leave our data.
	xcb_atom_t x11_clipboard_manager = ::common_atoms[CLIPBOARD_MANAGER];
	if (x11_clipboard_manager == 0) return 1;
	// Start the SAVE_TARGETS mechanism so the X11 CLIPBOARD_MANAGER
	// will save our clipboard data from now on.

	// Ask to the selection owner for its content on each known text format/atom.
	xcb_convert_selection(::m_connection,
						::m_window, // Send us the result
						x11_clipboard_manager, // Clipboard selection
						::common_atoms[SAVE_TARGETS], // The clipboard format that we're requesting
						::common_atoms[CLIPBOARD], // Leave result in this window's property
						XCB_CURRENT_TIME);
	xcb_flush(m_connection);
	return 0;
}

int handle_selection_notify_event(xcb_selection_notify_event_t* event) {
	if(event->requestor != ::m_window) {
		// std::cout << "event->requestor == ::m_window\n";
		return 1;
	}
	xcb_atom_t target_atom;
	if(event->target == ::common_atoms[TARGETS])
		target_atom = ::common_atoms[ATOM];
	else target_atom = event->target;
	xcb_get_property_cookie_t cookie = xcb_get_property(
		::m_connection,
		true,
		event->requestor,
		event->property,
		target_atom,
		0, 0x1fffffff  // 0x1fffffff = INT32_MAX / 4
	);
	xcb_generic_error_t* err = nullptr;
	xcb_get_property_reply_t* reply = xcb_get_property_reply(m_connection, cookie, &err);
	if(err) {
		// Handle Error
		free(err);
	}
	if(reply) free(reply);
	return 0;
}

int handle_selection_request_event(xcb_selection_request_event_t* event) {
	int returnVal = 0;
	if (event->target == ::common_atoms[TARGETS]) {
		atoms targets;
		targets.push_back(::common_atoms[TARGETS]);
		targets.push_back(::common_atoms[SAVE_TARGETS]);
		for (auto &t : ::m_data)
			targets.push_back(t.first);
		xcb_change_property(
			::m_connection,
			XCB_PROP_MODE_REPLACE,
			event->requestor,
			event->property,
			::common_atoms[ATOM],
			8*sizeof(xcb_atom_t),
			targets.size(),
			&targets[0]
		);
	}
	else {
		for (auto &t : ::m_data) {
			if(t.first == event->target) {
				returnVal = 1;
				xcb_change_property(
					::m_connection,
					XCB_PROP_MODE_REPLACE,
					event->requestor,
					event->property,
					event->target,
					8,
					t.second->size(),
					&(*(t.second))[0]
				);
			}
		}
	}
	xcb_selection_notify_event_t notify;
	notify.response_type = XCB_SELECTION_NOTIFY;
	notify.pad0          = 0;
	notify.sequence      = 0;
	notify.time          = event->time;
	notify.requestor     = event->requestor;
	notify.selection     = event->selection;
	notify.target        = event->target;
	notify.property      = event->property;
	xcb_send_event(
		::m_connection,
		false,
		event->requestor,
		XCB_EVENT_MASK_NO_EVENT, // SelectionNotify events go without mask
		(const char*)&notify
	);
	xcb_flush(m_connection);
	return returnVal;
}

void process_x11_events() {
	bool stop = false;
	xcb_generic_event_t* event = nullptr;
	while(!stop && (event = xcb_wait_for_event(::m_connection))) {
		int type = (event->response_type & ~0x80);
		switch (type) {
		
		case XCB_DESTROY_NOTIFY:
			// std::cout << "XCB_DESTROY_NOTIFY\n";
			stop = true;
			break;
		
		// Someone is requesting the clipboard content from us.
		case XCB_SELECTION_REQUEST:
			// std::cout << "XCB_SELECTION_REQUEST\n";
			if(handle_selection_request_event((xcb_selection_request_event_t*) event))
				stop = true;
			break;
		
		// We've requested the clipboard content and this is the answer.
		case XCB_SELECTION_NOTIFY:
			// std::cout << "XCB_SELECTION_NOTIFY\n";
			if(handle_selection_notify_event((xcb_selection_notify_event_t*) event))
				stop = true;
			break;
		
		// For these events - do nothing
		case XCB_SELECTION_CLEAR:
		case XCB_PROPERTY_NOTIFY:
			// std::cout << "caught XCB_SELECTION_CLEAR XCB_PROPERTY_NOTIFY break;\n";
			stop = true;
			break;
		}
		free(event);
	}
	return;
}

int CopyToClipboard(const char* path) {
	if(connect_x11()) return 1;
	get_common_atoms_list();
	// - check if all the common_atoms are non-zero otherwise return.
	set_data(path);
	if(set_x11_selection_owner()) return 1;
	if(set_data_clipboard()) return 1;

	process_x11_events();
	
	// disconnect from X11
	xcb_destroy_window(::m_connection, ::m_window);
	xcb_flush(::m_connection);
	xcb_disconnect(::m_connection);
	return 0;
}