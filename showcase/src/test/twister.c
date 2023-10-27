/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "twister.h"
#include "../game.h"
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/key.h>
#include <ace/managers/system.h>
#include <ace/managers/rand.h>
#include <ace/managers/blit.h>
#include <ace/utils/chunky.h>

// #define CLIPPING_ENABLED
#define CLIP_MARGIN_X 32
#define CLIP_MARGIN_Y 16

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBfr;
static tRandManager s_sRand;

static UWORD s_sz;
static ULONG s_ps;

void gsTestTwisterCreate(void) {
	// Prepare view & viewport
	s_pView = viewCreate(0,
		TAG_VIEW_GLOBAL_PALETTE, 1,
		TAG_DONE
	);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, 2,
		TAG_DONE
	);
	s_pBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
		TAG_SIMPLEBUFFER_BOUND_WIDTH, 512,
		TAG_SIMPLEBUFFER_BOUND_HEIGHT, 512,
		TAG_DONE
	);

	// Init stuff
	s_sz = 32;
	s_ps = 0;
	s_pVPort->pPalette[0] = 0x000;
	s_pVPort->pPalette[1] = 0x057;
	s_pVPort->pPalette[2] = 0x49b;
	s_pVPort->pPalette[3] = 0x8df;

	// testGrid()
	cameraSetCoord(s_pBfr->pCamera, 150, 180);
	randInit(&s_sRand, 1911, 2184);

	// Display view with its viewports
	systemUnuse();
	viewLoad(s_pView);
}

void gsTestTwisterLoop(void) {
	if (keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}

	if(keyCheck(KEY_UP)) {
		cameraMoveBy(s_pBfr->pCamera, 0, -1);
	}
	if(keyCheck(KEY_DOWN)) {
		cameraMoveBy(s_pBfr->pCamera, 0, 1);
	}
	if(keyCheck(KEY_LEFT)) {
		cameraMoveBy(s_pBfr->pCamera, -1, 0);
	}
	if(keyCheck(KEY_RIGHT)) {
		cameraMoveBy(s_pBfr->pCamera, 1, 0);
	}
	if(keyUse(KEY_R)) {
		cameraSetCoord(s_pBfr->pCamera, 150, 180);
	}
	if(keyUse(KEY_B)) {
		bitmapSaveBmp(s_pBfr->pFront, s_pVPort->pPalette, "twister.bmp");
	}

	for(UWORD y = 298; y <= 302; ++y) {
		for(UWORD x = 284; x <= 288;  ++x) {
			UBYTE ubColor = randUw(&s_sRand) & 3;
			chunkyToPlanar(ubColor, x + 2, y + 2, s_pBfr->pFront);
		}
	}

	UWORD shift = 0;
	s_ps += 1;

	// for(UBYTE i = 0; i <= 4; ++i) {
		shift = (shift << 1) | ((s_ps >> 0) & 1);
		shift = (shift << 1) | ((s_ps >> 1) & 1);
		shift = (shift << 1) | ((s_ps >> 2) & 1);
		shift = (shift << 1) | ((s_ps >> 3) & 1);
		shift = (shift << 1) | ((s_ps >> 4) & 1);
	// }

	for(UBYTE y = 4; y <= 13; ++y) {
		UWORD yy = y * 32 + shift;
		for(UBYTE x = 3; x <= 14; ++x) {
			UWORD xx = x * 32 + shift;

			UWORD uwSrcX = xx + (16 - y) - x;
			UWORD uwSrcY = yy + (16 - y) + x;
			UWORD uwDstX = xx;
			UWORD uwDstY = yy + 16;
			WORD wWidth = 32;
			WORD wHeight = 32;

#if defined(CLIPPING_ENABLED)
			if(uwDstX < 150) {
				UWORD uwDelta = 150 - uwDstX;
				uwSrcX += uwDelta;
				wWidth -= uwDelta;
				uwDstX = 150;
			}
			if(uwSrcX < 150) {
				UWORD uwDelta = 150 - uwSrcX;
				uwDstX += uwDelta;
				wWidth -= uwDelta;
				uwSrcX = 150;
			}
			if(uwSrcX + wWidth > 150 + 320 + CLIP_MARGIN_X) {
				wWidth = 150 + 320 + CLIP_MARGIN_X - uwSrcX;
			}
			if(uwDstX + wWidth > 150 + 320 + CLIP_MARGIN_X) {
				wWidth = 150 + 300 + CLIP_MARGIN_X - uwDstX;
			}

			if(uwDstY < 180) {
				UWORD uwDelta = 180 - uwDstY;
				uwSrcY += uwDelta;
				wHeight -= uwDelta;
				uwDstY = 180;
			}
			if(uwSrcY < 150) {
				UWORD uwDelta = 150 - uwSrcY;
				uwDstY += uwDelta;
				wWidth -= uwDelta;
				uwSrcY = 150;
			}
			if(uwSrcY + wHeight > 180 + 256 + CLIP_MARGIN_Y) {
				wHeight = 180 + 256 + CLIP_MARGIN_Y - uwSrcY;
			}
			if(uwDstY + wHeight > 180 + 256 + CLIP_MARGIN_Y) {
				wHeight = 180 + 256 + CLIP_MARGIN_Y - uwDstY;
			}

			if(wWidth <= 0 || wHeight <= 0) {
				continue;
			}
#endif

			blitCopy(
				s_pBfr->pFront, uwSrcX, uwSrcY,
				s_pBfr->pBack, uwDstX, uwDstY, wWidth, wHeight, MINTERM_COOKIE
			);
		}
	}

	viewProcessManagers(s_pView);
	copProcessBlocks();
	vPortWaitForEnd(s_pVPort);
}

void gsTestTwisterDestroy(void) {
	systemUse();
	// Destroy buffer, view & viewport
	viewDestroy(s_pView);
}
