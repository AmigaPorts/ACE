# Example asset pipeline: PNG sheets → interleaved .bm for hardware sprites.
# mouse = pointer art. ocs/aga = demo sprites (attached rainbow split 16→4+4).

function(convertShowcaseSprites TARGET RES_DIR DATA_DIR)
	set(_SPR_MOUSE_RES ${RES_DIR}/sprites/mouse)
	set(_SPR_GEN ${CMAKE_CURRENT_BINARY_DIR}/sprites_gen)
	set(_SPR_STAGING ${CMAKE_CURRENT_BINARY_DIR}/sprites_pak)
	set(_SPR_PAK ${DATA_DIR}/sprites.pak)
	file(MAKE_DIRECTORY ${_SPR_GEN})
	file(MAKE_DIRECTORY ${_SPR_STAGING})

	convertPalette(${TARGET} ${_SPR_MOUSE_RES}/mouse.gpl ${_SPR_GEN}/mouse.plt)
	extractBitmaps(
		TARGET ${TARGET} SOURCE ${_SPR_MOUSE_RES}/mouse.png
		DESTINATIONS
			${_SPR_GEN}/arrow.png 0 0 16 26
			${_SPR_GEN}/pencil.png 16 0 16 26
	)
	convertBitmaps(
		TARGET ${TARGET} PALETTE ${_SPR_GEN}/mouse.plt INTERLEAVED
		SOURCES ${_SPR_GEN}/arrow.png ${_SPR_GEN}/pencil.png
		DESTINATIONS ${_SPR_GEN}/arrow.bm ${_SPR_GEN}/pencil.bm
	)

	if(ACE_USE_AGA_FEATURES)
		set(_SPR_RES ${RES_DIR}/sprites/aga)
		extractBitmaps(
			TARGET ${TARGET} SOURCE ${_SPR_RES}/sprites.png
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
		convertPalette(${TARGET} ${_SPR_RES}/ice.gpl ${_SPR_GEN}/ice.plt AGA_COLORS)
		convertPalette(${TARGET} ${_SPR_RES}/gold.gpl ${_SPR_GEN}/gold.plt AGA_COLORS)
		convertPalette(${TARGET} ${_SPR_RES}/stripe_4.gpl ${_SPR_GEN}/stripe_4.plt AGA_COLORS)
		convertPalette(${TARGET} ${_SPR_RES}/orb_4.gpl ${_SPR_GEN}/orb_4.plt AGA_COLORS)
		convertPalette(${TARGET} ${_SPR_RES}/checker_4.gpl ${_SPR_GEN}/checker_4.plt AGA_COLORS)
		convertPalette(${TARGET} ${_SPR_RES}/sprites.gpl ${_SPR_GEN}/sprites.plt AGA_COLORS)
		convertPalette(${TARGET} ${_SPR_RES}/attached_16.gpl ${_SPR_GEN}/attached_16.plt AGA_COLORS)
		convertPalette(${TARGET} ${_SPR_RES}/sprites_4.gpl ${_SPR_GEN}/sprites_4.plt AGA_COLORS)
	else()
		set(_SPR_RES ${RES_DIR}/sprites/ocs)
		extractBitmaps(
			TARGET ${TARGET} SOURCE ${_SPR_RES}/sprites.png
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
		convertPalette(${TARGET} ${_SPR_RES}/stripe_l_4.gpl ${_SPR_GEN}/stripe_l_4.plt)
		convertPalette(${TARGET} ${_SPR_RES}/stripe_r_4.gpl ${_SPR_GEN}/stripe_r_4.plt)
		convertPalette(${TARGET} ${_SPR_RES}/orb_4.gpl ${_SPR_GEN}/orb_4.plt)
		convertPalette(${TARGET} ${_SPR_RES}/checker_4.gpl ${_SPR_GEN}/checker_4.plt)
		convertPalette(${TARGET} ${_SPR_RES}/sprites.gpl ${_SPR_GEN}/sprites.plt)
		convertPalette(${TARGET} ${_SPR_RES}/attached_16.gpl ${_SPR_GEN}/attached_16.plt)
		convertPalette(${TARGET} ${_SPR_RES}/sprites_4.gpl ${_SPR_GEN}/sprites_4.plt)
	endif()

	splitAttachedSprite(
		TARGET ${TARGET}
		SOURCE ${_SPR_GEN}/rainbow.png
		MATCH_PALETTE ${_SPR_GEN}/attached_16.plt
		LO ${_SPR_GEN}/rainbow_lo.png
		HI ${_SPR_GEN}/rainbow_hi.png
	)
	convertBitmaps(
		TARGET ${TARGET} PALETTE ${_SPR_GEN}/sprites_4.plt INTERLEAVED
		SOURCES ${_SPR_GEN}/rainbow_lo.png ${_SPR_GEN}/rainbow_hi.png
		DESTINATIONS ${_SPR_GEN}/rainbow_lo.bm ${_SPR_GEN}/rainbow_hi.bm
	)

	if(ACE_USE_AGA_FEATURES)
		convertBitmaps(
			TARGET ${TARGET} PALETTE ${_SPR_GEN}/stripe_4.plt INTERLEAVED
			SOURCES ${_SPR_GEN}/stripe.png DESTINATIONS ${_SPR_GEN}/stripe.bm
		)
		convertBitmaps(
			TARGET ${TARGET} PALETTE ${_SPR_GEN}/orb_4.plt INTERLEAVED
			SOURCES ${_SPR_GEN}/orb.png DESTINATIONS ${_SPR_GEN}/orb.bm
		)
		convertBitmaps(
			TARGET ${TARGET} PALETTE ${_SPR_GEN}/checker_4.plt INTERLEAVED
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
			TARGET ${TARGET} PALETTE ${_SPR_GEN}/stripe_l_4.plt INTERLEAVED
			SOURCES ${_SPR_GEN}/stripe_l.png DESTINATIONS ${_SPR_GEN}/stripe_l.bm
		)
		convertBitmaps(
			TARGET ${TARGET} PALETTE ${_SPR_GEN}/stripe_r_4.plt INTERLEAVED
			SOURCES ${_SPR_GEN}/stripe_r.png DESTINATIONS ${_SPR_GEN}/stripe_r.bm
		)
		convertBitmaps(
			TARGET ${TARGET} PALETTE ${_SPR_GEN}/orb_4.plt INTERLEAVED
			SOURCES ${_SPR_GEN}/orb.png DESTINATIONS ${_SPR_GEN}/orb.bm
		)
		convertBitmaps(
			TARGET ${TARGET} PALETTE ${_SPR_GEN}/checker_4.plt INTERLEAVED
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
	target_sources(${TARGET} PUBLIC ${_SPR_PAK})
endfunction()
