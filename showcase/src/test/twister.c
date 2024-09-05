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
#define TWISTER_CENTER_X (86+32)
#define TWISTER_CENTER_Y (128)
#define TWISTER_CENTER_RADIUS 2
#define TWISTER_BLOCK_SIZE 32
#define TWISTER_MIN_BLOCK_X (-(((TWISTER_CENTER_X + TWISTER_BLOCK_SIZE - 1) / TWISTER_BLOCK_SIZE) + 2))
#define TWISTER_MAX_BLOCK_X (+((((320 - TWISTER_CENTER_X) + TWISTER_BLOCK_SIZE - 1) / TWISTER_BLOCK_SIZE) + 0))
#define TWISTER_MIN_BLOCK_Y (-(((TWISTER_CENTER_Y + TWISTER_BLOCK_SIZE - 1) / TWISTER_BLOCK_SIZE) + 2))
#define TWISTER_MAX_BLOCK_Y (+((((256 - TWISTER_CENTER_Y) + TWISTER_BLOCK_SIZE - 1) / TWISTER_BLOCK_SIZE) + 0))
#define TWISTER_BLOCKS_X (TWISTER_MAX_BLOCK_X - TWISTER_MIN_BLOCK_X)
#define TWISTER_BLOCKS_Y (TWISTER_MAX_BLOCK_Y - TWISTER_MIN_BLOCK_Y)

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBfr;
static tRandManager s_sRand;

static ULONG s_ps;
static UBYTE s_isVectors;
static UBYTE s_isAdvancePs;

static void testGrid(UBYTE ubSize) {
	UBYTE ubColor = 0;
	for(UWORD uwX = ubSize; uwX < 320; uwX += ubSize) {
		blitRect(s_pBfr->pBack, uwX, 0, 1, 256, 1 + (ubColor & 1));
		++ubColor;
	}
	for(UWORD uwY = ubSize; uwY < 256; uwY += ubSize) {
		blitRect(s_pBfr->pBack, 0, uwY, 320, 1, 1 + (ubColor & 1));
		++ubColor;
	}
}

void gsTestTwisterCreate(void) {
	// Prepare view & viewport
	s_pView = viewCreate(0, TAG_DONE);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, 2,
		TAG_DONE
	);
	s_pBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
		TAG_SIMPLEBUFFER_BOUND_WIDTH, SCREEN_PAL_WIDTH + CLIP_MARGIN_X,
		TAG_SIMPLEBUFFER_BOUND_HEIGHT, SCREEN_PAL_HEIGHT + CLIP_MARGIN_Y,
		TAG_DONE
	);

	// Init stuff
	s_ps = 0;
	s_isVectors = 0;
	s_isAdvancePs = 1;
	s_pVPort->pPalette[0] = 0x000;
	s_pVPort->pPalette[1] = 0x057;
	s_pVPort->pPalette[2] = 0x49b;
	s_pVPort->pPalette[3] = 0x8df;

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
	else if(keyUse(KEY_R)) {
		cameraSetCoord(s_pBfr->pCamera, 0, 0);
	}
	else if(keyUse(KEY_B)) {
		bitmapSaveBmp(s_pBfr->pFront, s_pVPort->pPalette, "twister.bmp");
	}
	else if(keyUse(KEY_I)) {
		s_isAdvancePs = !s_isAdvancePs;
	}
	else if(keyUse(KEY_O)) {
		--s_ps;
		s_isAdvancePs = 0;
	}
	else if(keyUse(KEY_P)) {
		++s_ps;
		s_isAdvancePs = 0;
	}
	else if(keyUse(KEY_V)) {
		s_isVectors = !s_isVectors;
	}

	if(s_isAdvancePs) {
		++s_ps;
	}

	UWORD uwShift = 0;
	// for(UBYTE i = 0; i <= 4; ++i) {
		uwShift = (uwShift << 1) | ((s_ps >> 0) & 1);
		uwShift = (uwShift << 1) | ((s_ps >> 1) & 1);
		uwShift = (uwShift << 1) | ((s_ps >> 2) & 1);
		uwShift = (uwShift << 1) | ((s_ps >> 3) & 1);
		uwShift = (uwShift << 1) | ((s_ps >> 4) & 1);
	// }

	for(BYTE y = TWISTER_MIN_BLOCK_Y; y < TWISTER_MAX_BLOCK_Y; ++y) {
		WORD yy = TWISTER_CENTER_Y + y * TWISTER_BLOCK_SIZE + uwShift;
		for(BYTE x = TWISTER_MIN_BLOCK_X; x < TWISTER_MAX_BLOCK_X; ++x) {
			WORD xx = TWISTER_CENTER_X + x * TWISTER_BLOCK_SIZE + uwShift;

			WORD wSrcX = xx - (y + 1) - (x + 1);
			WORD wSrcY = yy - (y + 1) + (x + 1);
			WORD wDstX = xx;
			WORD wDstY = yy;
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

			if(s_isVectors) {
				blitLine(s_pBfr->pBack, wSrcX, wSrcY, wDstX, wDstY, 2, 0xFFFF, 0);
				chunkyToPlanar(1, wSrcX, wSrcY, s_pBfr->pBack);
				chunkyToPlanar(3, wDstX, wDstY, s_pBfr->pBack);
			}
			else {
				blitCopy(
					s_pBfr->pFront, wSrcX, wSrcY,
					s_pBfr->pBack, wDstX, wDstY, wWidth, wHeight, MINTERM_COOKIE
				);
			}
		}
	}

	for(UWORD y = TWISTER_CENTER_Y - TWISTER_CENTER_RADIUS; y <= TWISTER_CENTER_Y + TWISTER_CENTER_RADIUS; ++y) {
		for(UWORD x = TWISTER_CENTER_X - TWISTER_CENTER_RADIUS; x <= TWISTER_CENTER_X + TWISTER_CENTER_RADIUS; ++x) {
			UBYTE ubColor = randUw(&s_sRand) & 3;
			chunkyToPlanar(ubColor, x, y, s_pBfr->pBack);
		}
	}

	if(keyUse(KEY_G)) {
		testGrid(16);
	}
	else if(keyUse(KEY_H)) {
		testGrid(8);
	}
	else if(keyUse(KEY_J)) {
		testGrid(4);
	}

	viewProcessManagers(s_pView);
	copProcessBlocks();
	systemIdleBegin();
	vPortWaitForEnd(s_pVPort);
	systemIdleEnd();
}

void gsTestTwisterDestroy(void) {
	systemUse();
	// Destroy buffer, view & viewport
	viewDestroy(s_pView);
}
