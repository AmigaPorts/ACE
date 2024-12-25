/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "buffer_scroll.h"
#include <ace/managers/key.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/scrollbuffer.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/palette.h>
#include <ace/utils/font.h>
#include "game.h"

#define TEST_SCROLL_BPP 4

typedef enum tMode {
	MODE_SIMPLE_LORES,
	MODE_SIMPLE_HIRES,
	MODE_SCROLL_LORES,
	MODE_SCROLL_HIRES,
	MODE_COUNT
} tMode;

static const char *s_pModeNames[MODE_COUNT] = {
	[MODE_SIMPLE_LORES] = "lores simplebuffer",
	[MODE_SIMPLE_HIRES] = "hires simplebuffer",
	[MODE_SCROLL_LORES] = "lores scrollbuffer",
	[MODE_SCROLL_HIRES] = "hires scrollbuffer",
};

static tMode s_eCurrentMode;
static tView *s_pView;
static tVPort *s_pVPort;
static tCameraManager *s_pCamera;
static tFont *s_pFont;
static tTextBitMap *s_pTextBitMap;

static void drawModeInfo(tBitMap *pBfr, UWORD uwX, UWORD uwY) {
	char szMsg[50];
	sprintf(szMsg, "Current mode is %s", s_pModeNames[s_eCurrentMode]);
	fontDrawStr(s_pFont, pBfr, uwX, uwY + 0 * 10, szMsg, 6, FONT_COOKIE, s_pTextBitMap);
	fontDrawStr(s_pFont, pBfr, uwX, uwY + 1 * 10, "WSAD to navigate", 6, FONT_COOKIE, s_pTextBitMap);
	for(UBYTE i = 0; i < 4; ++i) {
		sprintf(szMsg, "%d to %s", i + 1, s_pModeNames[i]);
		fontDrawStr(s_pFont, pBfr, uwX, uwY + (2 + i) * 10, szMsg, 6, FONT_COOKIE, s_pTextBitMap);
	}
}

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

	drawModeInfo(pBfr, 50, 50);
	logBlockEnd("fillBfr()");
}

static void initSimpleBuffer(UBYTE isHires, UWORD uwWidth, UWORD uwHeight) {
	viewLoad(0);
	systemUse();
	if(s_pView->pFirstVPort) {
		vPortDestroy(s_pView->pFirstVPort);
	}

	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, TEST_SCROLL_BPP,
		TAG_VPORT_HIRES, isHires,
	TAG_DONE);
	paletteLoadFromPath("data/amidb32.plt", s_pVPort->pPalette, 1 << SHOWCASE_BPP);

	tSimpleBufferManager *s_pBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_BOUND_WIDTH, uwWidth,
		TAG_SIMPLEBUFFER_BOUND_HEIGHT, uwHeight,
	TAG_DONE);
	s_pCamera = s_pBfr->pCamera;

	fillBfr(s_pBfr->pBack, uwWidth, uwHeight);

	viewLoad(s_pView);
	systemUnuse();
}

static void initScrollBuffer(UBYTE isHires) {
	viewLoad(0);
	systemUse();
	if(s_pView->pFirstVPort) {
		vPortDestroy(s_pView->pFirstVPort);
	}

	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, TEST_SCROLL_BPP,
		TAG_VPORT_HIRES, isHires,
	TAG_DONE);
	paletteLoadFromPath("data/amidb32.plt", s_pVPort->pPalette, 1 << SHOWCASE_BPP);

	// This will create buffer which is shorter than 640 with capability of
	// wrapped scrolling to simulate bigger buffer size
	tScrollBufferManager *s_pBfr = scrollBufferCreate(0,
		TAG_SCROLLBUFFER_VPORT, s_pVPort,
		TAG_SCROLLBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_SCROLLBUFFER_BOUND_WIDTH, 1024,
		TAG_SCROLLBUFFER_BOUND_HEIGHT, 600,
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

	blitRect(s_pBfr->pBack, 16, 16, 32, 32, 5);

	drawModeInfo(s_pBfr->pBack, 50, 50);

	viewLoad(s_pView);
	systemUnuse();
}

static void changeMode(tMode eMode) {
	s_eCurrentMode = eMode;
	switch(eMode) {
		case MODE_SIMPLE_LORES:
			initSimpleBuffer(0, 400, 300);
			break;
		case MODE_SIMPLE_HIRES:
			initSimpleBuffer(1, 720, 300);
			break;
		case MODE_SCROLL_LORES:
			initScrollBuffer(0);
			break;
		case MODE_SCROLL_HIRES:
			initScrollBuffer(1);
			break;
		default:
			break;
	}
}

void gsTestBufferScrollCreate(void) {
	logBlockBegin("gsTestBufferScrollCreate()");

	s_pView = viewCreate(0,
	TAG_DONE);

	s_pFont = fontCreateFromPath("data/fonts/silkscreen.fnt");
	s_pTextBitMap = fontCreateTextBitMap(320, s_pFont->uwHeight);
	s_eCurrentMode = MODE_SIMPLE_LORES;
	changeMode(s_eCurrentMode);
	logBlockEnd("gsTestBufferScrollCreate()");
	systemUnuse();
}

void gsTestBufferScrollLoop(void) {
	if (keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
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
		changeMode(MODE_SIMPLE_LORES);
	}
	else if(keyUse(KEY_2)) {
		changeMode(MODE_SIMPLE_HIRES);
	}
	else if(keyUse(KEY_3)) {
		changeMode(MODE_SCROLL_LORES);
	}
	else if(keyUse(KEY_4)) {
		changeMode(MODE_SCROLL_HIRES);
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

	fontDestroy(s_pFont);
	fontDestroyTextBitMap(s_pTextBitMap);

	logBlockEnd("gsTestBufferScrollDestroy()");
}
