cmake_minimum_required(VERSION 2.8.5)
project(ACE)

# Adhere to GNU filesystem layout conventions
include(GNUInstallDirs)

# Lowercase project name for binaries and packaging
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

# Version number in format X.Y.Z
set(VER_X  0)
set(VER_Y 0)
set(VER_Z 1)
set(VER_FULL "${VER_X}.${VER_Y}.${VER_Z}")

if(NOT AMIGA)
	message(SEND_ERROR "This project only compiles for Amiga")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DAMIGA -Wall -Wextra -fomit-frame-pointer")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DACE_DEBUG")
file(GLOB_RECURSE SOURCES src/*.c)
file(GLOB HEADERS_ACE include/ace/*.h)
file(GLOB HEADERS_ACE_GENERIC include/ace/generic/*.h)
file(GLOB HEADERS_ACE_UTILS include/ace/utils/*.h)
file(GLOB HEADERS_ACE_MANAGERS include/ace/managers/*.h)
file(GLOB HEADERS_ACE_MANAGERS_VP include/ace/managers/viewport/*.h)
file(GLOB HEADERS_FIXMATH include/fixmath/*.h)
set(
	HEADERS
	${HEADERS_ACE} ${HEADERS_ACE_GENERIC} ${HEADERS_ACE_UTILS}
	${HEADERS_ACE_MANAGERS} ${HEADERS_ACE_MANAGERS_VP} ${HEADERS_FIXMATH}
)

include_directories(
	${PROJECT_SOURCE_DIR}/include
)

# Linux/other UNIX get a lower-case binary name
set(TARGET_NAME ${PROJECT_NAME_LOWER})
add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${TARGET_NAME})

install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${HEADERS_ACE} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ace)
install(FILES ${HEADERS_ACE_GENERIC} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ace/generic)
install(FILES ${HEADERS_ACE_UTILS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ace/utils)
install(FILES ${HEADERS_ACE_MANAGERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ace/managers)
install(FILES ${HEADERS_ACE_MANAGERS_VP} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ace/managers/viewport)
install(FILES ${HEADERS_FIXMATH} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fixmath)
