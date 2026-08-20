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

# Sprite + mouse assets are packed so the tests can demo pakFileOpen / GetFileByPath.
if(ACE_USE_AGA_FEATURES)
	set(_SPR_SRC ${RES_DIR}/sprites/aga)
	set(_SPR_ASSETS
		rainbow_lo.bm rainbow_hi.bm stripe.bm orb.bm checker.bm
		sprites.plt ice.plt gold.plt
	)
else()
	set(_SPR_SRC ${RES_DIR}/sprites/ocs)
	set(_SPR_ASSETS
		rainbow_lo.bm rainbow_hi.bm stripe_l.bm stripe_r.bm orb.bm checker.bm
		sprites.plt
	)
endif()

getToolPath(pak_tool TOOL_PAK_TOOL)
set(_SPR_STAGING ${CMAKE_CURRENT_BINARY_DIR}/sprites_pak)
set(_SPR_PAK ${DATA_DIR}/sprites.pak)
file(MAKE_DIRECTORY ${_SPR_STAGING})

set(_SPR_PAK_SRCS)
foreach(_spr_file ${_SPR_ASSETS})
	list(APPEND _SPR_PAK_SRCS ${_SPR_SRC}/${_spr_file})
endforeach()
list(APPEND _SPR_PAK_SRCS
	${RES_DIR}/sprites/mouse/arrow.bm
	${RES_DIR}/sprites/mouse/pencil.bm
)

# Pack order matches runtime GetFileByPath so disk_file keeps its 512-byte read
# buffer instead of seeking backwards (WARN: slow - read buffer discard).
if(ACE_USE_AGA_FEATURES)
	set(_SPR_PAK_ORDER
		arrow.bm pencil.bm
		sprites.plt ice.plt gold.plt
		rainbow_lo.bm rainbow_hi.bm stripe.bm orb.bm checker.bm
	)
else()
	set(_SPR_PAK_ORDER
		arrow.bm pencil.bm
		sprites.plt
		rainbow_lo.bm rainbow_hi.bm stripe_l.bm stripe_r.bm orb.bm checker.bm
	)
endif()
set(_SPR_ORDER_FILE ${CMAKE_CURRENT_BINARY_DIR}/sprites_pak_order.txt)
string(REPLACE ";" "\n" _SPR_ORDER_TXT "${_SPR_PAK_ORDER}")
file(WRITE ${_SPR_ORDER_FILE} "${_SPR_ORDER_TXT}\n")

set(_SPR_PACK_CMDS)
foreach(_src ${_SPR_PAK_SRCS})
	get_filename_component(_spr_name ${_src} NAME)
	list(APPEND _SPR_PACK_CMDS
		COMMAND ${CMAKE_COMMAND} -E copy "${_src}" "${_SPR_STAGING}/${_spr_name}"
	)
endforeach()

add_custom_command(
	OUTPUT ${_SPR_PAK}
	${_SPR_PACK_CMDS}
	COMMAND ${TOOL_PAK_TOOL} ${_SPR_STAGING} ${_SPR_PAK} -r ${_SPR_ORDER_FILE}
	DEPENDS ${_SPR_PAK_SRCS} ${_SPR_ORDER_FILE}
	COMMENT "Packing sprite assets into sprites.pak"
	VERBATIM
)
target_sources(${GAME_EXECUTABLE} PUBLIC ${_SPR_PAK})
