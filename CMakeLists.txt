cmake_minimum_required(VERSION 2.8.5)
project(ACE)

# Adhere to GNU filesystem layout conventions
include(GNUInstallDirs)

# Lowercase project name for binaries and packaging
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

# Version number in format X.YY.ZZ
set(VER_X  1)
set(VER_YY 00)
set(VER_ZZ 00)
set(VER_FULL "${VER_X}.${VER_YY}.${VER_ZZ}")

if(NOT AMIGA)
    message(SEND_ERROR
        "This project only compiles for Amiga"
    )
endif()

set(
    SOURCES
    src/fixmath/fix16.c
    src/fixmath/fix16_exp.c
    src/fixmath/fix16_sqrt.c
    src/fixmath/fix16_str.c
    src/fixmath/fix16_trig.c
    src/fixmath/fract32.c
    src/fixmath/uint32.c
    src/ace/managers/blit.c
    src/ace/managers/copper.c
    src/ace/managers/game.c
    src/ace/managers/joy.c
    src/ace/managers/key.c
    src/ace/managers/log.c
    src/ace/managers/memory.c
    src/ace/managers/mouse.c
    src/ace/managers/rand.c
    src/ace/managers/system.c
    src/ace/managers/timer.c
    src/ace/managers/viewport/camera.c
    src/ace/managers/viewport/scrollbuffer.c
    src/ace/managers/viewport/simplebuffer.c
    src/ace/managers/viewport/tilebuffer.c
    src/ace/utils/bitmap.c
    src/ace/utils/bmframe.c
    src/ace/utils/chunky.c
    src/ace/utils/custom.c
    src/ace/utils/dir.c
    src/ace/utils/extview.c
    src/ace/utils/file.c
    src/ace/utils/font.c
    src/ace/utils/palette.c
    src/ace/utils/tag.c
)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# Linux/other UNIX get a lower-case binary name
set(TARGET_NAME ${PROJECT_NAME_LOWER})
add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${TARGET_NAME})

set(INSTALL_DEST ${CMAKE_INSTALL_BINDIR})
install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DEST})


