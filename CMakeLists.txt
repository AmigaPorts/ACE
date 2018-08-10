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
	message(SEND_ERROR
		"This project only compiles for Amiga"
	)
endif()

file(GLOB_RECURSE SOURCES src/*.c inc/*.h)

include_directories(
	${PROJECT_SOURCE_DIR}/include
)

# Linux/other UNIX get a lower-case binary name
set(TARGET_NAME ${PROJECT_NAME_LOWER})
add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${TARGET_NAME})

set(INSTALL_DEST ${CMAKE_INSTALL_BINDIR})
install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DEST})
