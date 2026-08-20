# Example asset pipeline: PNG sheets → interleaved .bm for hardware sprites.
# mouse = pointer art. ocs/aga = demo sprites (attached rainbow split 16→4+4).

function(convertShowcaseSprites TARGET RES_DIR DATA_DIR)
	set(_SPR_MOUSE_RES ${RES_DIR}/sprites/mouse)
	set(_SPR_GEN ${CMAKE_CURRENT_BINARY_DIR}/sprites_gen)
	set(_SPR_STAGING ${CMAKE_CURRENT_BINARY_DIR}/sprites_pak)
	set(_SPR_PAK ${DATA_DIR}/sprites.pak)
	file(MAKE_DIRECTORY ${_SPR_GEN})
	file(MAKE_DIRECTORY ${_SPR_STAGING})

	if(ACE_USE_AGA_FEATURES)
		set(_SPR_AGA AGA)
	else()
		set(_SPR_AGA)
	endif()

	extractBitmaps(
		TARGET ${TARGET} SOURCE ${_SPR_MOUSE_RES}/mouse.png
		DESTINATIONS
			${_SPR_GEN}/arrow.png 0 0 16 26
			${_SPR_GEN}/pencil.png 16 0 16 26
	)
	convertSprite(
		TARGET ${TARGET} PALETTE ${_SPR_MOUSE_RES}/mouse.gpl
		SOURCE ${_SPR_GEN}/arrow.png DESTINATION ${_SPR_GEN}/arrow.bm
	)
	convertSprite(
		TARGET ${TARGET} PALETTE ${_SPR_MOUSE_RES}/mouse.gpl
		SOURCE ${_SPR_GEN}/pencil.png DESTINATION ${_SPR_GEN}/pencil.bm
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
		convertPalette(${TARGET} ${_SPR_RES}/sprites.gpl ${_SPR_GEN}/sprites.plt AGA_COLORS)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/sprites.gpl ${_SPR_AGA}
			SOURCE ${_SPR_GEN}/stripe.png DESTINATION ${_SPR_GEN}/stripe.bm
		)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/sprites.gpl ${_SPR_AGA}
			SOURCE ${_SPR_GEN}/orb.png DESTINATION ${_SPR_GEN}/orb.bm
		)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/sprites.gpl ${_SPR_AGA}
			SOURCE ${_SPR_GEN}/checker.png DESTINATION ${_SPR_GEN}/checker.bm
		)
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
		convertPalette(${TARGET} ${_SPR_RES}/sprites.gpl ${_SPR_GEN}/sprites.plt)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/sprites.gpl
			SOURCE ${_SPR_GEN}/stripe_l.png DESTINATION ${_SPR_GEN}/stripe_l.bm
		)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/sprites.gpl
			SOURCE ${_SPR_GEN}/stripe_r.png DESTINATION ${_SPR_GEN}/stripe_r.bm
		)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/sprites.gpl
			SOURCE ${_SPR_GEN}/orb.png DESTINATION ${_SPR_GEN}/orb.bm
		)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/sprites.gpl
			SOURCE ${_SPR_GEN}/checker.png DESTINATION ${_SPR_GEN}/checker.bm
		)
	endif()

	convertSprite(
		TARGET ${TARGET} PALETTE ${_SPR_RES}/attached_16.gpl ${_SPR_AGA}
		SOURCE ${_SPR_GEN}/rainbow.png
		ATTACHED
		LO ${_SPR_GEN}/rainbow_lo.bm
		HI ${_SPR_GEN}/rainbow_hi.bm
	)

	set(_SPR_PAK_SRCS)
	foreach(_name ${_SPR_PAK_ORDER})
		list(APPEND _SPR_PAK_SRCS ${_SPR_GEN}/${_name})
	endforeach()

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
