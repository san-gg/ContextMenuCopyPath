# CMakeList.txt : CMake project for ContextCopyPath, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("ContextCopyPath")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(BUILD_CODE "WinContextCopy.cpp")
	set(OPTIMIZE_FLAG "/O2")
else ()
	set(BUILD_CODE "LinuxContextCopy.cpp")
	set(OPTIMIZE_FLAG "-O3")
endif()

# Add source to this project's executable.
add_executable (ContextCopyPath "ContextCopyPath.cpp"  ${BUILD_CODE})

# set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE ${OPTIMIZE_FLAG})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ContextCopyPath PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
