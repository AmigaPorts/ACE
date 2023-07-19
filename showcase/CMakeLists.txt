cmake_minimum_required(VERSION 3.14)
project(showcase)

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
file(GLOB_RECURSE HEADERS src/*.h)


include_directories(${PROJECT_SOURCE_DIR}/src)

set(RES_DIR ${CMAKE_CURRENT_LIST_DIR}/res)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)

# ACE
add_subdirectory(.. ace)
include_directories(../include)

# Linux/other UNIX get a lower-case binary name
set(TARGET_NAME ${PROJECT_NAME_LOWER})

if(ELF2HUNK)
  # Add elf2hunk step for Bartman compiler
  set(GAME_LINKED ${TARGET_NAME}.elf) # Intermediate executable
  set(GAME_EXE ${TARGET_NAME}.exe) # Use this to launch the game
  add_executable(${GAME_LINKED} ${SOURCES} ${HEADERS})
  add_custom_command(
    TARGET ${GAME_LINKED} POST_BUILD
    COMMAND ${ELF2HUNK} ${GAME_LINKED} ${GAME_EXE}
  )
else()
  # Just produce the executable with Bebbo compiler
  SET(GAME_LINKED ${TARGET_NAME})
  SET(GAME_EXE ${TARGET_NAME})
  add_executable(${GAME_LINKED} ${SOURCES} ${HEADERS})
endif()

target_link_libraries(${GAME_LINKED} ace)

# Copy example maps files as is
file(COPY "${RES_DIR}/data" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
