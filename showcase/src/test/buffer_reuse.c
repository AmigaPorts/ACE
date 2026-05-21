/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Shared CHIP buffer reuse (simpleBuffer -> tileBuffer on one extview)
 *
 * ACE buffer managers normally call bitmapCreate() and bitmapDestroy() for you.
 * This test shows how to own the CHIP memory yourself and pass it in, so the
 * same physical allocation can back different managers over the game lifecycle.
 *
 * Two separate things live in memory:
 *
 *   s_pChip      - Raw CHIP RAM (bitplane bytes). You allocate with memAllocChip().
 *                  This pointer should stay the same across phase 1 and phase 2.
 *
 *   s_pSharedBm  - A small tBitMap *header* (Fast RAM) describing how to interpret
 *                  s_pChip: width, height, BytesPerRow, plane pointers, etc.
 *                  Created with bitmapCreateFromMem() which sets BMF_EXTERNAL.
 *
 * With BMF_EXTERNAL, bitmapDestroy() only frees the tBitMap struct (~sizeof tBitMap),
 * NOT s_pChip. Only memFree(s_pChip, s_ulChipSize) releases the CHIP pool.
 *
 * Managers are told "use this bitmap, don't allocate your own" via:
 *   TAG_SIMPLEBUFFER_FRONT_BITMAP / TAG_TILEBUFFER_FRONT_BITMAP
 * On destroy they skip bitmapDestroy() for those pointers.
 *
 * Phase 1: simpleBuffer + small bitmap geometry (viewport size).
 * Phase 2: tileBuffer (uses scrollBuffer internally) + larger scroll geometry.
 *          Same s_pChip, new tBitMap wrapper, new managers on the same vPort.
 *
 * Controls:
 *   Fire/Return/Space - switch to phase 2
 *   Joystick          - scroll tilemap (phase 2)
 *   ESC               - exit to menu
 */

#include "test/buffer_reuse.h"
#include "game.h"
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/viewport/tilebuffer.h>
#include <ace/managers/viewport/scrollbuffer.h>
#include <ace/managers/viewport/camera.h>
#include <ace/managers/state.h>
#include <ace/managers/memory.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/blit.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>
#include <ace/utils/extview.h>
#include <ace/utils/font.h>
#include <ace/utils/bitmap.h>
#include <ace/generic/screen.h>

/* Tile map size for phase 2 (world size in tiles). */
#define REUSE_TILE_SHIFT     4
#define REUSE_TILE_SIZE      (1 << REUSE_TILE_SHIFT)
#define REUSE_MAP_TILES_X    20
#define REUSE_MAP_TILES_Y    12
#define REUSE_BOUND_WIDTH    (REUSE_MAP_TILES_X * REUSE_TILE_SIZE)
#define REUSE_BOUND_HEIGHT   (REUSE_MAP_TILES_Y * REUSE_TILE_SIZE)
#define REUSE_REDRAW_QUEUE   48
#define REUSE_TILE_COUNT     4

typedef enum tBufferReusePhase {
	BUFFER_REUSE_PHASE_SIMPLE = 0,
	BUFFER_REUSE_PHASE_TILE = 1,
} tBufferReusePhase;

/* View / viewport stay alive for both phases. */
static tView *s_pView;
static tVPort *s_pVPort;

/* Active buffer manager (only one at a time after the switch). */
static tSimpleBufferManager *s_pSimpleBfr;
static tTileBufferManager *s_pTileBfr;

/*
 * Shared display memory:
 *   s_pChip     - CHIP pool (you own this)
 *   s_pSharedBm - tBitMap wrapper recreated when geometry changes
 */
static tBitMap *s_pSharedBm;
static void *s_pChip;
static ULONG s_ulChipSize;

/* Tile graphics live in a separate bitmap (not part of the scroll buffer pool). */
static tBitMap *s_pTileSet;

static tBufferReusePhase s_ePhase;
static tFont *s_pFont;
static tTextBitMap *s_pTextBm;

static void bufferReuseDrawSimpleScreen(void);
static void bufferReuseStartTilePhase(void);
static void bufferReuseOnTileDraw(
	UWORD uwTileX, UWORD uwTileY,
	tBitMap *pBitMap, UWORD uwBitMapX, UWORD uwBitMapY
);
static void bufferReuseFillTileMap(void);

/**
 * TileBuffer redraw callback: blit one column-strip tile from s_pTileSet into the
 * scroll buffer bitmap at the pixel position the manager calculated.
 */
static void bufferReuseOnTileDraw(
	UWORD uwTileX, UWORD uwTileY,
	tBitMap *pBitMap, UWORD uwBitMapX, UWORD uwBitMapY
) {
	tTileBufferTileIndex tile = s_pTileBfr->pTileData[uwTileX][uwTileY];
	UWORD uwSrcY = (UWORD)tile * REUSE_TILE_SIZE;
	blitCopy(
		s_pTileSet, 0, uwSrcY,
		pBitMap, uwBitMapX, uwBitMapY,
		REUSE_TILE_SIZE, REUSE_TILE_SIZE,
		MINTERM_COOKIE
	);
}

static void bufferReuseFillTileMap(void) {
	for(UWORD uwX = 0; uwX < REUSE_MAP_TILES_X; ++uwX) {
		for(UWORD uwY = 0; uwY < REUSE_MAP_TILES_Y; ++uwY) {
			s_pTileBfr->pTileData[uwX][uwY] = (uwX + uwY) % REUSE_TILE_COUNT;
		}
	}
}

/** Phase 1 one-shot draw into the simple buffer back bitmap. */
static void bufferReuseDrawSimpleScreen(void) {
	UWORD uwX, uwY;
	UBYTE isOdd = 0;
	tBitMap *pBm = s_pSimpleBfr->pBack;
	UWORD uwMaxX = s_pSimpleBfr->uBfrBounds.uwX;
	UWORD uwMaxY = s_pSimpleBfr->uBfrBounds.uwY;

	for(uwY = 0; uwY < uwMaxY; uwY += 16) {
		for(uwX = 0; uwX < uwMaxX; uwX += 16) {
			blitRect(pBm, uwX, uwY, 16, 16, isOdd ? 2 : 3);
			isOdd = !isOdd;
		}
		isOdd = !isOdd;
	}

	fontDrawStr(
		s_pFont, pBm, uwMaxX >> 1, 40,
		"Shared CHIP buffer", 1,
		FONT_COOKIE | FONT_CENTER | FONT_SHADOW, s_pTextBm
	);
	fontDrawStr(
		s_pFont, pBm, uwMaxX >> 1, 56,
		"Phase 1: Simple buffer", 1,
		FONT_COOKIE | FONT_CENTER | FONT_SHADOW, s_pTextBm
	);
	fontDrawStr(
		s_pFont, pBm, uwMaxX >> 1, uwMaxY - 32,
		"Fire/Return: tile buffer", 1,
		FONT_COOKIE | FONT_CENTER | FONT_SHADOW, s_pTextBm
	);
	fontDrawStr(
		s_pFont, pBm, uwMaxX >> 1, uwMaxY - 16,
		"ESC: menu", 1,
		FONT_COOKIE | FONT_CENTER | FONT_SHADOW, s_pTextBm
	);
	/* Compare this address with the log after switching phases. */
	logWrite("CHIP pool @ %p, size %lu\n", s_pChip, s_ulChipSize);
}

/**
 * Tear down phase 1 managers/wrappers and start phase 2 on the same s_pChip.
 */
static void bufferReuseStartTilePhase(void) {
	UWORD uwScrollW, uwScrollH;
	UBYTE ubBitmapFlags = BMF_CLEAR;

	logWrite("Switching to tile buffer, same CHIP @ %p\n", s_pChip);

	/*
	 * 1) Remove simpleBuffer from the viewport. Its destroy callback runs but
	 *    does NOT free s_pSharedBm (we passed TAG_SIMPLEBUFFER_FRONT_BITMAP).
	 *    Camera manager is left on the vPort; tileBuffer will reuse it.
	 */
	vPortRmManager(s_pVPort, (tVpManager*)s_pSimpleBfr);
	s_pSimpleBfr = 0;

	/*
	 * 2) Destroy the tBitMap *header* only (BMF_EXTERNAL).
	 *    s_pChip is unchanged — compare log address before/after this call.
	 *    We must drop the old header because simple and scroll bitmaps use
	 *    different widths/heights (BytesPerRow, Rows, plane layout).
	 */
	bitmapDestroy(s_pSharedBm);
	s_pSharedBm = 0;

	/* Scroll bitmap is taller/wider than the viewport; includes scroll margins. */
	scrollBufferGetBitmapDimensions(
		s_pVPort, REUSE_TILE_SIZE,
		REUSE_BOUND_WIDTH, REUSE_BOUND_HEIGHT,
		&uwScrollW, &uwScrollH
	);
	logWrite(
		"Scroll bitmap %ux%u (pool size %lu)\n",
		uwScrollW, uwScrollH, s_ulChipSize
	);

	/*
	 * 3) New wrapper on the same CHIP block. Requires s_ulChipSize >= scroll size
	 *    (we sized the pool to max(simple, scroll) in Create).
	 */
	s_pSharedBm = bitmapCreateFromMem(
		s_pChip, uwScrollW, uwScrollH, s_pVPort->ubBpp, ubBitmapFlags
	);
	if(!s_pSharedBm) {
		logWrite("ERR: bitmapCreateFromMem failed for scroll size\n");
		return;
	}

	/*
	 * 4) tileBuffer creates an internal scrollBuffer and attaches both to the vPort.
	 *    TAG_TILEBUFFER_FRONT_BITMAP forwards to scroll as external front bitmap.
	 */
	s_pTileBfr = tileBufferCreate(0,
		TAG_TILEBUFFER_VPORT, s_pVPort,
		TAG_TILEBUFFER_TILE_SHIFT, REUSE_TILE_SHIFT,
		TAG_TILEBUFFER_BOUND_TILE_X, REUSE_MAP_TILES_X,
		TAG_TILEBUFFER_BOUND_TILE_Y, REUSE_MAP_TILES_Y,
		TAG_TILEBUFFER_TILESET, s_pTileSet,
		TAG_TILEBUFFER_CALLBACK_TILE_DRAW, bufferReuseOnTileDraw,
		TAG_TILEBUFFER_REDRAW_QUEUE_LENGTH, REUSE_REDRAW_QUEUE,
		TAG_TILEBUFFER_BITMAP_FLAGS, ubBitmapFlags,
		TAG_TILEBUFFER_FRONT_BITMAP, s_pSharedBm,
		TAG_DONE
	);
	if(!s_pTileBfr) {
		logWrite("ERR: tileBufferCreate failed\n");
		return;
	}

	bufferReuseFillTileMap();
	cameraSetCoord(s_pTileBfr->pCamera, 0, 0);
	tileBufferRedrawAll(s_pTileBfr);
	s_ePhase = BUFFER_REUSE_PHASE_TILE;
}

void gsTestBufferReuseCreate(void) {
	UWORD uwSimpleW, uwSimpleH;
	UWORD uwScrollW, uwScrollH;
	ULONG ulSimpleSize, ulScrollSize;
	UBYTE ubBitmapFlags = BMF_CLEAR;

	logBlockBegin("gsTestBufferReuseCreate");
	systemUse();
	s_ePhase = BUFFER_REUSE_PHASE_SIMPLE;

	s_pView = viewCreate(0,
		TAG_VIEW_WINDOW_WIDTH, SCREEN_PAL_WIDTH,
		TAG_VIEW_WINDOW_HEIGHT, SCREEN_PAL_HEIGHT - 32,
		TAG_DONE
	);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, SHOWCASE_BPP,
		TAG_DONE
	);

	/*
	 * Phase 1 bitmap size = viewport (simpleBuffer default bounds).
	 * Phase 2 needs scrollBuffer dimensions (larger due to tile margins + wrap).
	 * Allocate CHIP once for the larger of the two so both phases fit in s_pChip.
	 */
	uwSimpleW = s_pVPort->uwWidth;
	uwSimpleH = s_pVPort->uwHeight;

	scrollBufferGetBitmapDimensions(
		s_pVPort, REUSE_TILE_SIZE,
		REUSE_BOUND_WIDTH, REUSE_BOUND_HEIGHT,
		&uwScrollW, &uwScrollH
	);
	ulSimpleSize = bitmapGetBufferSize(
		uwSimpleW, uwSimpleH, s_pVPort->ubBpp, ubBitmapFlags
	);
	ulScrollSize = bitmapGetBufferSize(
		uwScrollW, uwScrollH, s_pVPort->ubBpp, ubBitmapFlags
	);
	s_ulChipSize = ulSimpleSize > ulScrollSize ? ulSimpleSize : ulScrollSize;
	s_pChip = memAllocChip(s_ulChipSize);
	logWrite(
		"CHIP pool %lu bytes (simple %lu, scroll %lu) @ %p\n",
		s_ulChipSize, ulSimpleSize, ulScrollSize, s_pChip
	);

	/* Wrap CHIP for phase 1 geometry; manager will not allocate or free planes. */
	s_pSharedBm = bitmapCreateFromMem(
		s_pChip, uwSimpleW, uwSimpleH, s_pVPort->ubBpp, ubBitmapFlags
	);
	s_pSimpleBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_FRONT_BITMAP, s_pSharedBm,
		TAG_DONE
	);

	/* Tileset is independent CHIP (small); not reused as the scroll buffer. */
	s_pTileSet = bitmapCreate(
		REUSE_TILE_SIZE, REUSE_TILE_SIZE * REUSE_TILE_COUNT,
		s_pVPort->ubBpp, ubBitmapFlags
	);
	for(UBYTE i = 0; i < REUSE_TILE_COUNT; ++i) {
		blitRect(
			s_pTileSet, 0, i * REUSE_TILE_SIZE,
			REUSE_TILE_SIZE, REUSE_TILE_SIZE,
			(UBYTE)(i + 2)
		);
	}

	s_pVPort->pPalette[0] = 0x000;
	s_pVPort->pPalette[1] = 0xFFF;
	s_pVPort->pPalette[2] = 0xF00;
	s_pVPort->pPalette[3] = 0x0F0;
	s_pVPort->pPalette[4] = 0x00F;

	s_pFont = fontCreateFromPath("data/fonts/silkscreen.fnt");
	s_pTextBm = fontCreateTextBitMap(320, s_pFont->uwHeight);
	bufferReuseDrawSimpleScreen();

	viewLoad(s_pView);
	logBlockEnd("gsTestBufferReuseCreate");
	systemUnuse();
}

void gsTestBufferReuseLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}

	if(s_ePhase == BUFFER_REUSE_PHASE_SIMPLE) {
		if(keyUse(KEY_RETURN) || keyUse(KEY_SPACE) || joyUse(JOY1_FIRE)) {
			bufferReuseStartTilePhase();
		}
		viewProcessManagers(s_pView);
		vPortWaitForEnd(s_pVPort);
		return;
	}

	/* Phase 2: camera drives scroll; tileBufferQueueProcess redraws dirty tiles. */
	if(joyUse(JOY1_LEFT)) {
		cameraMoveBy(s_pTileBfr->pCamera, -REUSE_TILE_SIZE, 0);
	}
	if(joyUse(JOY1_RIGHT)) {
		cameraMoveBy(s_pTileBfr->pCamera, REUSE_TILE_SIZE, 0);
	}
	if(joyUse(JOY1_UP)) {
		cameraMoveBy(s_pTileBfr->pCamera, 0, -REUSE_TILE_SIZE);
	}
	if(joyUse(JOY1_DOWN)) {
		cameraMoveBy(s_pTileBfr->pCamera, 0, REUSE_TILE_SIZE);
	}

	viewProcessManagers(s_pView);
	tileBufferQueueProcess(s_pTileBfr);
	vPortWaitForEnd(s_pVPort);
}

void gsTestBufferReuseDestroy(void) {
	logBlockBegin("gsTestBufferReuseDestroy");
	systemUse();

	/*
	 * tileBufferDestroy() does not remove the internal scrollBuffer from the vPort,
	 * so detach tile then scroll explicitly when leaving phase 2.
	 */
	if(s_pTileBfr) {
		vPortRmManager(s_pVPort, (tVpManager*)s_pTileBfr);
		s_pTileBfr = 0;
		if(s_pVPort->pFirstManager) {
			tVpManager *pScroll = vPortGetManager(s_pVPort, VPM_SCROLL);
			if(pScroll) {
				vPortRmManager(s_pVPort, pScroll);
			}
		}
	}
	if(s_pSimpleBfr) {
		vPortRmManager(s_pVPort, (tVpManager*)s_pSimpleBfr);
		s_pSimpleBfr = 0;
	}

	/* Header first, then the CHIP pool it described. */
	if(s_pSharedBm) {
		bitmapDestroy(s_pSharedBm);
		s_pSharedBm = 0;
	}
	if(s_pChip) {
		memFree(s_pChip, s_ulChipSize);
		s_pChip = 0;
	}
	if(s_pTileSet) {
		bitmapDestroy(s_pTileSet);
		s_pTileSet = 0;
	}
	fontDestroyTextBitMap(s_pTextBm);
	fontDestroy(s_pFont);
	viewDestroy(s_pView);
	s_pView = 0;
	s_pVPort = 0;

	logBlockEnd("gsTestBufferReuseDestroy");
	systemUnuse();
}
