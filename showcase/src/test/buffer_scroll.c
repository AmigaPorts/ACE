/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "buffer_scroll.h"
#include <ace/managers/key.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/scrollbuffer.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/palette.h>
#include "game.h"

#define TEST_SCROLL_BPP 6

static tView *s_pView;
static tVPort *s_pVPort;
static tCameraManager *s_pCamera;

static void fillBfr(tBitMap *pBfr, UWORD uwWidth, UWORD uwHeight) {
	logBlockBegin(
		"fillBfr(pBfr: *%p, uwWidth: %hu, uwHeight: %hu)",
		pBfr, uwWidth, uwHeight
	);
	uwWidth -= 1;
	uwHeight -= 1;

	// Short lines
	for(UWORD x = 10; x <= uwWidth; x += 10) {
		blitRect(pBfr, x,          0, 1, 3, 7);
		blitRect(pBfr, x, uwHeight-3, 1, 3, 7);
	}
	for(UWORD y = 10; y <= uwHeight; y += 10) {
		blitRect(pBfr,         0, y, 3, 1, 7);
		blitRect(pBfr, uwWidth-3, y, 3, 1, 7);
	}

	// Long lines
	for(UWORD x = 100; x <= uwWidth; x += 100) {
		blitRect(pBfr, x,          0, 1, 5, 7);
		blitRect(pBfr, x, uwHeight-5, 1, 5, 7);
	}
	for(UWORD y = 100; y <= uwHeight; y += 100) {
		blitRect(pBfr,         0, y, 5, 1, 7);
		blitRect(pBfr, uwWidth-5, y, 5, 1, 7);
	}

	// Border
	blitRect(pBfr,       0,        0, uwWidth,        1, 6);
	blitRect(pBfr,       0, uwHeight, uwWidth,        1, 6);
	blitRect(pBfr,       0,        0,       1, uwHeight, 6);
	blitRect(pBfr, uwWidth,        0,       1, uwHeight, 6);
	logBlockEnd("fillBfr()");
}

void initSimple(void) {
	viewLoad(0);
	systemUse();
	if(s_pView->pFirstVPort) {
		vPortDestroy(s_pView->pFirstVPort);
	}

	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, TEST_SCROLL_BPP,
	TAG_DONE);
	paletteLoad("data/amidb32.plt", s_pVPort->pPalette, 1 << SHOWCASE_BPP);

	tSimpleBufferManager *s_pBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_BOUND_WIDTH, 640,
		TAG_SIMPLEBUFFER_BOUND_HEIGHT, 720,
	TAG_DONE);
	s_pCamera = s_pBfr->pCamera;

	fillBfr(s_pBfr->pBack, 640, 720);

	viewLoad(s_pView);
	systemUnuse();
}

void initScroll(void) {
	viewLoad(0);
	systemUse();
	if(s_pView->pFirstVPort) {
		vPortDestroy(s_pView->pFirstVPort);
	}

	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, TEST_SCROLL_BPP,
	TAG_DONE);
	paletteLoad("data/amidb32.plt", s_pVPort->pPalette, 1 << SHOWCASE_BPP);

	// This will create buffer which is shorter than 640 with capability of
	// wrapped scrolling to simulate bigger buffer size
	tScrollBufferManager *s_pBfr = scrollBufferCreate(0,
		TAG_SCROLLBUFFER_VPORT, s_pVPort,
		TAG_SCROLLBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_SCROLLBUFFER_BOUND_WIDTH, 640,
		TAG_SCROLLBUFFER_BOUND_HEIGHT, 720,
		TAG_SCROLLBUFFER_MARGIN_WIDTH, 32,
	TAG_DONE);
	s_pCamera = s_pBfr->pCamera;

	fillBfr(
		s_pBfr->pBack,
		bitmapGetByteWidth(s_pBfr->pBack) * 8, s_pBfr->uwBmAvailHeight
	);

	// Ensure that scroll buffer y-wraps nicely
	UBYTE ubColor = (1 << TEST_SCROLL_BPP) - 1;
	blitRect(s_pBfr->pBack, 0, 508, 1, 1, ubColor);
	blitRect(s_pBfr->pBack, 0, 509, 2, 1, ubColor);
	blitRect(s_pBfr->pBack, 0, 510, 3, 1, ubColor);
	blitRect(s_pBfr->pBack, 0, 511, 4, 1, ubColor);
	blitRect(s_pBfr->pBack, 0, 0, 5, 1, ubColor);
	blitRect(s_pBfr->pBack, 0, 1, 4, 1, ubColor);
	blitRect(s_pBfr->pBack, 0, 2, 3, 1, ubColor);
	blitRect(s_pBfr->pBack, 0, 3, 2, 1, ubColor);
	blitRect(s_pBfr->pBack, 0, 4, 1, 1, ubColor);

	blitRect(s_pBfr->pBack, 32, 32, 32, 32, 5);
	viewLoad(s_pView);
	systemUnuse();
}

void gsTestBufferScrollCreate(void) {
	logBlockBegin("gsTestBufferScrollCreate()");

	s_pView = viewCreate(0,
		TAG_VIEW_GLOBAL_PALETTE, 1,
	TAG_DONE);

	initScroll();
	logBlockEnd("gsTestBufferScrollCreate()");
	systemUnuse();
}

void gsTestBufferScrollLoop(void) {
	if (keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, g_pGameStates[GAME_STATE_MENU]);
		return;
	}

	WORD wDx = 0, wDy = 0;
	if(keyCheck(KEY_W)) {
		wDy = -1;
	}
	else if(keyCheck(KEY_S)) {
		wDy = 1;
	}
	if(keyCheck(KEY_A)) {
		wDx = -1;
	}
	if(keyCheck(KEY_D)) {
		wDx = 1;
	}

	if(keyUse(KEY_1)) {
		initSimple();
	}
	else if(keyUse(KEY_2)) {
		initScroll();
	}

	cameraMoveBy(s_pCamera, wDx, wDy);
	viewProcessManagers(s_pView);
	copProcessBlocks();
	vPortWaitForEnd(s_pVPort);
}

void gsTestBufferScrollDestroy(void) {
	logBlockBegin("gsTestBufferScrollDestroy()");
	systemUse();
	viewDestroy(s_pView);
	logBlockEnd("gsTestBufferScrollDestroy()");
}
