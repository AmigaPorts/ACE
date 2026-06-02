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
  set(GAME_EXECUTABLE ${TARGET_NAME}.elf) # Intermediate executable
  set(GAME_OUTPUT_EXECUTABLE ${TARGET_NAME}.exe) # Use this to launch the game
  add_executable(${GAME_EXECUTABLE} ${SOURCES} ${HEADERS})
  add_custom_command(
    TARGET ${GAME_EXECUTABLE} POST_BUILD
    COMMAND ${ELF2HUNK} ${GAME_EXECUTABLE} ${GAME_OUTPUT_EXECUTABLE}
  )
else()
  # Just produce the executable with Bebbo compiler
  SET(GAME_EXECUTABLE ${TARGET_NAME})
  SET(GAME_OUTPUT_EXECUTABLE ${TARGET_NAME})
  add_executable(${GAME_EXECUTABLE} ${SOURCES} ${HEADERS})
endif()

target_link_libraries(${GAME_EXECUTABLE} ace)

set(RES_DIR ${CMAKE_CURRENT_LIST_DIR}/res)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)
file(MAKE_DIRECTORY ${DATA_DIR})

set(AMIDB32_PLT_PATH ${DATA_DIR}/amidb32.plt)
convertPalette(${GAME_EXECUTABLE} ${RES_DIR}/amidb32.gpl ${AMIDB32_PLT_PATH})
convertPalette(${GAME_EXECUTABLE} ${RES_DIR}/blitToSmall.gpl ${DATA_DIR}/blitToSmall.plt)

convertBitmaps(
	TARGET ${GAME_EXECUTABLE} PALETTE ${AMIDB32_PLT_PATH}
	SOURCES ${RES_DIR}/32c_pal.png DESTINATIONS ${DATA_DIR}/32c_pal.bm
)
convertBitmaps(
	TARGET ${GAME_EXECUTABLE} PALETTE ${AMIDB32_PLT_PATH} INTERLEAVED
	SOURCES ${RES_DIR}/32c_pal_interleaved.png DESTINATIONS ${DATA_DIR}/32c_pal_interleaved.bm
)

convertFont(
  TARGET ${GAME_EXECUTABLE}
	SOURCE ${RES_DIR}/silkscreen DESTINATION ${DATA_DIR}/silkscreen.fnt
)

file(MAKE_DIRECTORY ${DATA_DIR}/fonts)
file(COPY ${RES_DIR}/quaver.fnt DESTINATION ${DATA_DIR}/fonts)
