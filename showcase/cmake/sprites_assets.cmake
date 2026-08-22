# Example asset pipeline: PNG sheets → interleaved .bm for hardware sprites.
# mouse = pointer art. ocs/aga = demo sprites (attached rainbow split 16→4+4).
# Palettes go through convertPalette / palette_conv, not sprite_conv.

function(convertShowcaseSprites TARGET RES_DIR DATA_DIR)
	set(_SPR_MOUSE_RES ${RES_DIR}/sprites/mouse)
	set(_SPR_GEN ${CMAKE_CURRENT_BINARY_DIR}/sprites_gen)
	file(MAKE_DIRECTORY ${_SPR_GEN})
	file(MAKE_DIRECTORY ${DATA_DIR})

	extractBitmaps(
		TARGET ${TARGET} SOURCE ${_SPR_MOUSE_RES}/mouse.png
		DESTINATIONS
			${_SPR_GEN}/arrow.png 0 0 16 26
			${_SPR_GEN}/pencil.png 16 0 16 26
	)
	convertSprite(
		TARGET ${TARGET} PALETTE ${_SPR_MOUSE_RES}/mouse.gpl
		SOURCE ${_SPR_GEN}/arrow.png DESTINATION ${DATA_DIR}/arrow.bm
	)
	convertSprite(
		TARGET ${TARGET} PALETTE ${_SPR_MOUSE_RES}/mouse.gpl
		SOURCE ${_SPR_GEN}/pencil.png DESTINATION ${DATA_DIR}/pencil.bm
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
		convertPalette(${TARGET} ${_SPR_RES}/ice.gpl ${DATA_DIR}/ice.plt AGA_COLORS)
		convertPalette(${TARGET} ${_SPR_RES}/gold.gpl ${DATA_DIR}/gold.plt AGA_COLORS)
		convertPalette(${TARGET} ${_SPR_RES}/sprites.gpl ${DATA_DIR}/sprites.plt AGA_COLORS)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/sprites.gpl
			SOURCE ${_SPR_GEN}/stripe.png DESTINATION ${DATA_DIR}/stripe.bm
		)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/sprites.gpl
			SOURCE ${_SPR_GEN}/orb.png DESTINATION ${DATA_DIR}/orb.bm
		)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/sprites.gpl
			SOURCE ${_SPR_GEN}/checker.png DESTINATION ${DATA_DIR}/checker.bm
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
		convertPalette(${TARGET} ${_SPR_RES}/sprites.gpl ${DATA_DIR}/sprites.plt)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/sprites.gpl
			SOURCE ${_SPR_GEN}/stripe_l.png DESTINATION ${DATA_DIR}/stripe_l.bm
		)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/sprites.gpl
			SOURCE ${_SPR_GEN}/stripe_r.png DESTINATION ${DATA_DIR}/stripe_r.bm
		)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/sprites.gpl
			SOURCE ${_SPR_GEN}/orb.png DESTINATION ${DATA_DIR}/orb.bm
		)
		convertSprite(
			TARGET ${TARGET} PALETTE ${_SPR_RES}/checker_4.gpl
			SOURCE ${_SPR_GEN}/checker.png DESTINATION ${DATA_DIR}/checker.bm
		)
	endif()

	convertSprite(
		TARGET ${TARGET} PALETTE ${_SPR_RES}/attached_16.gpl
		SOURCE ${_SPR_GEN}/rainbow.png
		ATTACHED
		LO ${DATA_DIR}/rainbow_lo.bm
		HI ${DATA_DIR}/rainbow_hi.bm
	)
endfunction()
