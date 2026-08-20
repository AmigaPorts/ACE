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

# ---------------------------------------------------------------------------
# Sprites: PNG/GPL → extract → sprite_split_16to4 → convertBitmaps -i → pak
# ---------------------------------------------------------------------------
set(_SPR_MOUSE_RES ${RES_DIR}/sprites/mouse)
set(_SPR_GEN ${CMAKE_CURRENT_BINARY_DIR}/sprites_gen)
set(_SPR_STAGING ${CMAKE_CURRENT_BINARY_DIR}/sprites_pak)
set(_SPR_PAK ${DATA_DIR}/sprites.pak)
file(MAKE_DIRECTORY ${_SPR_GEN})
file(MAKE_DIRECTORY ${_SPR_STAGING})

convertPalette(${GAME_EXECUTABLE} ${_SPR_MOUSE_RES}/mouse.gpl ${_SPR_GEN}/mouse.plt)
extractBitmaps(
	TARGET ${GAME_EXECUTABLE} SOURCE ${_SPR_MOUSE_RES}/mouse.png
	DESTINATIONS
		${_SPR_GEN}/arrow.png 0 0 16 26
		${_SPR_GEN}/pencil.png 16 0 16 26
)
convertBitmaps(
	TARGET ${GAME_EXECUTABLE} PALETTE ${_SPR_GEN}/mouse.plt INTERLEAVED
	SOURCES ${_SPR_GEN}/arrow.png ${_SPR_GEN}/pencil.png
	DESTINATIONS ${_SPR_GEN}/arrow.bm ${_SPR_GEN}/pencil.bm
)

if(ACE_USE_AGA_FEATURES)
	set(_SPR_RES ${RES_DIR}/sprites/aga)
	extractBitmaps(
		TARGET ${GAME_EXECUTABLE} SOURCE ${_SPR_RES}/sprites.png
		DESTINATIONS
			${_SPR_GEN}/rainbow.png 0 0 32 26
			${_SPR_GEN}/stripe.png 32 0 32 26
			${_SPR_GEN}/orb.png 64 0 32 26
			${_SPR_GEN}/checker.png 96 0 32 26
	)
	set(_SPR_PAK_ORDER
		arrow.bm pencil.bm
		sprites.plt ice.plt gold.plt
		rainbow_lo.bm rainbow_hi.bm stripe.bm orb.bm checker.bm
	)
	convertPalette(${GAME_EXECUTABLE} ${_SPR_RES}/ice.gpl ${_SPR_GEN}/ice.plt)
	convertPalette(${GAME_EXECUTABLE} ${_SPR_RES}/gold.gpl ${_SPR_GEN}/gold.plt)
	convertPalette(${GAME_EXECUTABLE} ${_SPR_RES}/stripe_4.gpl ${_SPR_GEN}/stripe_4.plt)
	convertPalette(${GAME_EXECUTABLE} ${_SPR_RES}/orb_4.gpl ${_SPR_GEN}/orb_4.plt)
	convertPalette(${GAME_EXECUTABLE} ${_SPR_RES}/checker_4.gpl ${_SPR_GEN}/checker_4.plt)
else()
	set(_SPR_RES ${RES_DIR}/sprites/ocs)
	extractBitmaps(
		TARGET ${GAME_EXECUTABLE} SOURCE ${_SPR_RES}/sprites.png
		DESTINATIONS
			${_SPR_GEN}/rainbow.png 0 0 16 26
			${_SPR_GEN}/stripe_l.png 16 0 16 26
			${_SPR_GEN}/stripe_r.png 32 0 16 26
			${_SPR_GEN}/orb.png 48 0 16 26
			${_SPR_GEN}/checker.png 64 0 16 26
	)
	set(_SPR_PAK_ORDER
		arrow.bm pencil.bm
		sprites.plt
		rainbow_lo.bm rainbow_hi.bm stripe_l.bm stripe_r.bm orb.bm checker.bm
	)
	convertPalette(${GAME_EXECUTABLE} ${_SPR_RES}/stripe_l_4.gpl ${_SPR_GEN}/stripe_l_4.plt)
	convertPalette(${GAME_EXECUTABLE} ${_SPR_RES}/stripe_r_4.gpl ${_SPR_GEN}/stripe_r_4.plt)
	convertPalette(${GAME_EXECUTABLE} ${_SPR_RES}/orb_4.gpl ${_SPR_GEN}/orb_4.plt)
	convertPalette(${GAME_EXECUTABLE} ${_SPR_RES}/checker_4.gpl ${_SPR_GEN}/checker_4.plt)
endif()

convertPalette(${GAME_EXECUTABLE} ${_SPR_RES}/sprites.gpl ${_SPR_GEN}/sprites.plt)
convertPalette(${GAME_EXECUTABLE} ${_SPR_RES}/attached_16.gpl ${_SPR_GEN}/attached_16.plt)
convertPalette(${GAME_EXECUTABLE} ${_SPR_RES}/sprites_4.gpl ${_SPR_GEN}/sprites_4.plt)

splitAttachedSprite(
	TARGET ${GAME_EXECUTABLE}
	SOURCE ${_SPR_GEN}/rainbow.png
	MATCH_PALETTE ${_SPR_GEN}/attached_16.plt
	LO ${_SPR_GEN}/rainbow_lo.png
	HI ${_SPR_GEN}/rainbow_hi.png
)
convertBitmaps(
	TARGET ${GAME_EXECUTABLE} PALETTE ${_SPR_GEN}/sprites_4.plt INTERLEAVED
	SOURCES ${_SPR_GEN}/rainbow_lo.png ${_SPR_GEN}/rainbow_hi.png
	DESTINATIONS ${_SPR_GEN}/rainbow_lo.bm ${_SPR_GEN}/rainbow_hi.bm
)

if(ACE_USE_AGA_FEATURES)
	convertBitmaps(
		TARGET ${GAME_EXECUTABLE} PALETTE ${_SPR_GEN}/stripe_4.plt INTERLEAVED
		SOURCES ${_SPR_GEN}/stripe.png DESTINATIONS ${_SPR_GEN}/stripe.bm
	)
	convertBitmaps(
		TARGET ${GAME_EXECUTABLE} PALETTE ${_SPR_GEN}/orb_4.plt INTERLEAVED
		SOURCES ${_SPR_GEN}/orb.png DESTINATIONS ${_SPR_GEN}/orb.bm
	)
	convertBitmaps(
		TARGET ${GAME_EXECUTABLE} PALETTE ${_SPR_GEN}/checker_4.plt INTERLEAVED
		SOURCES ${_SPR_GEN}/checker.png DESTINATIONS ${_SPR_GEN}/checker.bm
	)
	set(_SPR_PAK_SRCS
		${_SPR_GEN}/arrow.bm ${_SPR_GEN}/pencil.bm
		${_SPR_GEN}/sprites.plt ${_SPR_GEN}/ice.plt ${_SPR_GEN}/gold.plt
		${_SPR_GEN}/rainbow_lo.bm ${_SPR_GEN}/rainbow_hi.bm
		${_SPR_GEN}/stripe.bm ${_SPR_GEN}/orb.bm ${_SPR_GEN}/checker.bm
	)
else()
	convertBitmaps(
		TARGET ${GAME_EXECUTABLE} PALETTE ${_SPR_GEN}/stripe_l_4.plt INTERLEAVED
		SOURCES ${_SPR_GEN}/stripe_l.png DESTINATIONS ${_SPR_GEN}/stripe_l.bm
	)
	convertBitmaps(
		TARGET ${GAME_EXECUTABLE} PALETTE ${_SPR_GEN}/stripe_r_4.plt INTERLEAVED
		SOURCES ${_SPR_GEN}/stripe_r.png DESTINATIONS ${_SPR_GEN}/stripe_r.bm
	)
	convertBitmaps(
		TARGET ${GAME_EXECUTABLE} PALETTE ${_SPR_GEN}/orb_4.plt INTERLEAVED
		SOURCES ${_SPR_GEN}/orb.png DESTINATIONS ${_SPR_GEN}/orb.bm
	)
	convertBitmaps(
		TARGET ${GAME_EXECUTABLE} PALETTE ${_SPR_GEN}/checker_4.plt INTERLEAVED
		SOURCES ${_SPR_GEN}/checker.png DESTINATIONS ${_SPR_GEN}/checker.bm
	)
	set(_SPR_PAK_SRCS
		${_SPR_GEN}/arrow.bm ${_SPR_GEN}/pencil.bm
		${_SPR_GEN}/sprites.plt
		${_SPR_GEN}/rainbow_lo.bm ${_SPR_GEN}/rainbow_hi.bm
		${_SPR_GEN}/stripe_l.bm ${_SPR_GEN}/stripe_r.bm
		${_SPR_GEN}/orb.bm ${_SPR_GEN}/checker.bm
	)
endif()

getToolPath(pak_tool TOOL_PAK_TOOL)
# Stable order file: only rewrite when content changes so configure does not
# bump mtime and force sprites.pak to rebuild every build.
set(_SPR_ORDER_FILE ${CMAKE_CURRENT_BINARY_DIR}/sprites_pak_order.txt)
string(REPLACE ";" "\n" _SPR_ORDER_TXT "${_SPR_PAK_ORDER}")
set(_SPR_ORDER_CONTENT "${_SPR_ORDER_TXT}\n")
set(_SPR_ORDER_EXISTING "")
if(EXISTS "${_SPR_ORDER_FILE}")
	file(READ "${_SPR_ORDER_FILE}" _SPR_ORDER_EXISTING)
endif()
if(NOT _SPR_ORDER_CONTENT STREQUAL _SPR_ORDER_EXISTING)
	file(WRITE "${_SPR_ORDER_FILE}" "${_SPR_ORDER_CONTENT}")
endif()

set(_SPR_PACK_CMDS)
foreach(_src ${_SPR_PAK_SRCS})
	get_filename_component(_spr_name ${_src} NAME)
	list(APPEND _SPR_PACK_CMDS
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_src}" "${_SPR_STAGING}/${_spr_name}"
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
