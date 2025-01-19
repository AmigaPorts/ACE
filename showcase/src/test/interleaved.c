/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "test/interleaved.h"
#include <ace/utils/extview.h>
#include <ace/utils/palette.h>
#include <ace/managers/key.h>
#include <ace/managers/blit.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include "game.h"

#define BOUNCE_MARGIN 32
#define BOUNCE_RECT_WIDTH 32
#define BOUNCE_RECT_HEIGHT 32

static tView *s_pTestInterleavedView;
static tVPort *s_pTestInterleavedVPort;
static tSimpleBufferManager *s_pTestInterleavedBfr;
static tBitMap *s_pSave;
static tBitMap *s_pBouncer;

static WORD wX, wY;
static BYTE bDx, bDy;

void gsTestInterleavedCreate(void) {
	s_pTestInterleavedView = viewCreate(0, TAG_DONE);
	s_pTestInterleavedVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pTestInterleavedView,
		TAG_VPORT_BPP, SHOWCASE_BPP,
		TAG_DONE
	);
	s_pTestInterleavedBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pTestInterleavedVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_DONE
	);
	paletteLoadFromPath(
		"data/amidb32.plt", s_pTestInterleavedVPort->pPalette, 1 << SHOWCASE_BPP
	);
	bitmapLoadFromPath(
		s_pTestInterleavedBfr->pBack, "data/32c_pal_interleaved.bm", 0, 0
	);

	s_pSave = bitmapCreate(BOUNCE_RECT_WIDTH + 16, BOUNCE_RECT_HEIGHT, SHOWCASE_BPP, BMF_INTERLEAVED);
	s_pBouncer = bitmapCreate(BOUNCE_RECT_WIDTH, BOUNCE_RECT_HEIGHT, SHOWCASE_BPP, BMF_INTERLEAVED);
	blitRect(s_pBouncer, 0, 0, BOUNCE_RECT_WIDTH, BOUNCE_RECT_HEIGHT, 0b10110);
	wX = 100;
	wY = 50;
	bDx = 3;
	bDy = 1;

	blitCopyAligned(
		s_pTestInterleavedBfr->pBack, wX & 0xFFF0, wY, s_pSave, 0, 0,
		BOUNCE_RECT_WIDTH + 16, BOUNCE_RECT_HEIGHT
	);

	systemUnuse();
	viewLoad(s_pTestInterleavedView);
}

void gsTestInterleavedLoop(void) {
	if (keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}

	blitCopyAligned(
		s_pSave, 0, 0, s_pTestInterleavedBfr->pBack, wX & 0xFFF0, wY,
		BOUNCE_RECT_WIDTH + 16, BOUNCE_RECT_HEIGHT
	);
	wX += bDx;
	wY += bDy;
	if(wX < BOUNCE_MARGIN) {
		bDx = -bDx;
		wX = BOUNCE_MARGIN;
	}
	if(wY < BOUNCE_MARGIN) {
		bDy = -bDy;
		wY = BOUNCE_MARGIN;
	}
	if(wX + BOUNCE_RECT_WIDTH >= 320 - BOUNCE_MARGIN) {
		bDx = -bDx;
		wX = 320 - BOUNCE_MARGIN - BOUNCE_RECT_WIDTH;
	}
	if(wY + BOUNCE_RECT_HEIGHT >= 256 - BOUNCE_MARGIN) {
		bDy = -bDy;
		wY = 256 - BOUNCE_MARGIN - BOUNCE_RECT_HEIGHT;
	}
	blitCopyAligned(
		s_pTestInterleavedBfr->pBack, wX & 0xFFF0, wY, s_pSave, 0, 0,
		BOUNCE_RECT_WIDTH + 16, BOUNCE_RECT_HEIGHT
	);

	blitCopy(
		s_pBouncer, 0, 0, s_pTestInterleavedBfr->pBack, wX, wY,
		BOUNCE_RECT_WIDTH, BOUNCE_RECT_HEIGHT, MINTERM_COOKIE
	);

	vPortWaitForPos(s_pTestInterleavedVPort, wY + BOUNCE_RECT_HEIGHT, 1);
}

void gsTestInterleavedDestroy(void) {
	systemUse();
	viewDestroy(s_pTestInterleavedView);
	bitmapDestroy(s_pSave);
	bitmapDestroy(s_pBouncer);
}
