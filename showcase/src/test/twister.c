/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "twister.h"
#include "../game.h"
#include <ace/generic/screen.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/key.h>
#include <ace/managers/system.h>
#include <ace/managers/rand.h>
#include <ace/managers/blit.h>
#include <ace/utils/chunky.h>

#define CLIP_MARGIN_X 32
#define CLIP_MARGIN_Y 16
#define TWISTER_CENTER_X 138
#define TWISTER_CENTER_Y 122
#define TWISTER_CENTER_RADIUS 2
#define TWISTER_BLOCK_SIZE 32

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBfr;
static tRandManager s_sRand;

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
		TAG_SIMPLEBUFFER_BOUND_WIDTH, SCREEN_PAL_WIDTH + CLIP_MARGIN_X,
		TAG_SIMPLEBUFFER_BOUND_HEIGHT, SCREEN_PAL_HEIGHT + CLIP_MARGIN_Y,
		TAG_DONE
	);

	// Init stuff
	s_ps = 0;
	s_pVPort->pPalette[0] = 0x000;
	s_pVPort->pPalette[1] = 0x057;
	s_pVPort->pPalette[2] = 0x49b;
	s_pVPort->pPalette[3] = 0x8df;

	// testGrid()
	// cameraSetCoord(s_pBfr->pCamera, 150, 180);
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
		cameraSetCoord(s_pBfr->pCamera, 0, 0);
	}
	if(keyUse(KEY_B)) {
		bitmapSaveBmp(s_pBfr->pFront, s_pVPort->pPalette, "twister.bmp");
	}

	s_ps += 1;
	UWORD uwShift = 0;
	// for(UBYTE i = 0; i <= 4; ++i) {
		uwShift = (uwShift << 1) | ((s_ps >> 0) & 1);
		uwShift = (uwShift << 1) | ((s_ps >> 1) & 1);
		uwShift = (uwShift << 1) | ((s_ps >> 2) & 1);
		uwShift = (uwShift << 1) | ((s_ps >> 3) & 1);
		uwShift = (uwShift << 1) | ((s_ps >> 4) & 1);
	// }

	// Original code was using 150,180 as camera coord on 800x600 bitmap.
	// Following uses same math, but compensates for 0,0 on smaller buffer.
	for(UBYTE y = 4; y <= 13; ++y) {
		WORD yy = y * TWISTER_BLOCK_SIZE + uwShift - 180;
		for(UBYTE x = 3; x <= 14; ++x) {
			WORD xx = x * TWISTER_BLOCK_SIZE + uwShift - 150;

			WORD wSrcX = xx + (16 - y) - x;
			WORD wSrcY = yy + (16 - y) + x;
			WORD wDstX = xx;
			WORD wDstY = yy + 16;
			WORD wWidth = TWISTER_BLOCK_SIZE;
			WORD wHeight = TWISTER_BLOCK_SIZE;

			if(wDstX < 0) {
				WORD wDelta = -wDstX;
				wSrcX += wDelta;
				wWidth -= wDelta;
				wDstX = 0;
			}
			if(wSrcX < 0) {
				WORD wDelta = -wSrcX;
				wDstX += wDelta;
				wWidth -= wDelta;
				wSrcX = 0;
			}
			if(wSrcX + wWidth > SCREEN_PAL_WIDTH + CLIP_MARGIN_X) {
				wWidth = SCREEN_PAL_WIDTH + CLIP_MARGIN_X - wSrcX;
			}
			if(wDstX + wWidth > SCREEN_PAL_WIDTH + CLIP_MARGIN_X) {
				wWidth = SCREEN_PAL_WIDTH + CLIP_MARGIN_X - wDstX;
			}

			if(wDstY < 0) {
				WORD wDelta = -wDstY;
				wSrcY += wDelta;
				wHeight -= wDelta;
				wDstY = 0;
			}
			if(wSrcY < 0) {
				WORD wDelta = -wSrcY;
				wDstY += wDelta;
				wWidth -= wDelta;
				wSrcY = 0;
			}
			if(wSrcY + wHeight > 0 + SCREEN_PAL_HEIGHT + CLIP_MARGIN_Y) {
				wHeight = 0 + SCREEN_PAL_HEIGHT + CLIP_MARGIN_Y - wSrcY;
			}
			if(wDstY + wHeight > 0 + SCREEN_PAL_HEIGHT + CLIP_MARGIN_Y) {
				wHeight = 0 + SCREEN_PAL_HEIGHT + CLIP_MARGIN_Y - wDstY;
			}

			if(wWidth <= 0 || wHeight <= 0) {
				continue;
			}

			blitCopy(
				s_pBfr->pFront, wSrcX, wSrcY,
				s_pBfr->pBack, wDstX, wDstY, wWidth, wHeight, MINTERM_COOKIE
			);
		}
	}

	for(UWORD y = TWISTER_CENTER_Y - TWISTER_CENTER_RADIUS; y <= TWISTER_CENTER_Y + TWISTER_CENTER_RADIUS; ++y) {
		for(UWORD x = TWISTER_CENTER_X - TWISTER_CENTER_RADIUS; x <= TWISTER_CENTER_X + TWISTER_CENTER_RADIUS; ++x) {
			UBYTE ubColor = randUw(&s_sRand) & 3;
			chunkyToPlanar(ubColor, x, y, s_pBfr->pBack);
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
