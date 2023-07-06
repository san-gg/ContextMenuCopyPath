#include <Windows.h>

int CopyToClipboard(const char * path) {
	if (!OpenClipboard(NULL)) return GetLastError();
	EmptyClipboard();
	SIZE_T len = strlen(path);
	HGLOBAL global_mem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(TCHAR));
	if (global_mem == NULL) {
		CloseClipboard();
		return 1;
	}
	LPTSTR lock_global_mem = (LPSTR)GlobalLock(global_mem);
	memcpy(lock_global_mem, path, len * sizeof(char));
	lock_global_mem[len] = (TCHAR)0;
	GlobalUnlock(global_mem);
	HANDLE return_val = SetClipboardData(CF_TEXT, global_mem);
	CloseClipboard();
	if (return_val == NULL) return GetLastError();
	return 0;
}