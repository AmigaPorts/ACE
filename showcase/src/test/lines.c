/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "test/lines.h"
#include <ace/managers/blit.h>
#include <ace/managers/key.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/custom.h>
#include <fixmath/fixmath.h>
#include "game.h"

#define STAR_POSITION_COUNT 60
#define STAR_ARM_COUNT 5
#define STAR_DIVISION (STAR_POSITION_COUNT / (STAR_ARM_COUNT * 2));
#define STAR_RADIUS 64
#define STAR_CENTER_X 200
#define STAR_CENTER_Y 100

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBfrManager;
static tWCoordYX s_pPositions[STAR_POSITION_COUNT];
static UBYTE s_ubFirstPosIndex;

void gsTestLinesCreate(void) {
	s_pView = viewCreate(0, TAG_END);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_BPP, 2,
		TAG_VPORT_VIEW, s_pView,
		TAG_END
	);
	s_pBfrManager = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_END
	);

	s_pVPort->pPalette[1] = 0xF00;
	s_pVPort->pPalette[2] = 0x0F0;
	s_pVPort->pPalette[3] = 0x00F;

	UWORD uwMinX = 0;
	UWORD uwMaxX = s_pBfrManager->uBfrBounds.uwX-1;
	UWORD uwMinY = 0;
	UWORD uwMaxY = s_pBfrManager->uBfrBounds.uwY-1;

	UWORD uwPattern = 0xFFFF;

	blitLine(s_pBfrManager->pBack, uwMinX, uwMinY, uwMinX, uwMaxY, 1, uwPattern, 0);
	blitLine(s_pBfrManager->pBack, uwMaxX, uwMinY, uwMaxX, uwMaxY, 1, uwPattern, 0);
	blitLine(s_pBfrManager->pBack, uwMinX, uwMinY, uwMaxX, uwMinY, 1, uwPattern, 0);
	blitLine(s_pBfrManager->pBack, uwMinX, uwMaxY, uwMaxX, uwMaxY, 1, uwPattern, 0);

	blitRect(s_pBfrManager->pBack, 32, 32, 32, 32, 2);
	blitLine(s_pBfrManager->pBack, 16, 16, 80, 80, 1, uwPattern, 0);

	// Prepare circle vertex positions.
	// For better accuracy, supply your own precalculated points or more accurate sin/cos table
	const fix16_t fHalf = fix16_one / 2;
	fix16_t fAngle;
	for(UBYTE ubPosIndex = 0; ubPosIndex != STAR_POSITION_COUNT; ++ubPosIndex) {
		fAngle = (fix16_pi*ubPosIndex*2) / STAR_POSITION_COUNT;
		WORD wSin = fix16_to_int(STAR_RADIUS * fix16_sin(fAngle) + fHalf);
		WORD wCos = fix16_to_int(STAR_RADIUS * fix16_cos(fAngle) + fHalf);
		s_pPositions[ubPosIndex].wX = wSin;
		s_pPositions[ubPosIndex].wY = wCos;
	}

	s_ubFirstPosIndex = 0;


	viewLoad(s_pView);
	systemUnuse();
}

void gsTestLinesLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}

	UWORD uwStartX = (STAR_CENTER_X - STAR_RADIUS - 1) & 0xFFF0;
	UWORD uwEndX = (STAR_CENTER_X + STAR_RADIUS + 1 + 15) & 0xFFF0;

	// Erase background
	vPortWaitForPos(s_pVPort, STAR_CENTER_Y + STAR_RADIUS, 0);
	blitRect(
		s_pBfrManager->pBack,
		uwStartX, STAR_CENTER_Y - STAR_RADIUS - 1,
		uwEndX - uwStartX, (STAR_RADIUS + 1) * 2 + 1, 0
	);

	// Draw circle
	UBYTE ubPosIndex = s_ubFirstPosIndex;
	for(UBYTE ubVertexIndex = 0; ubVertexIndex < STAR_ARM_COUNT * 2; ++ubVertexIndex) {
		UBYTE ubNextPosIndex = ubPosIndex + STAR_DIVISION;
		if(ubNextPosIndex >= STAR_POSITION_COUNT) {
			ubNextPosIndex -= STAR_POSITION_COUNT;
		}

		tWCoordYX sPoint, sNextPoint;
		if(ubVertexIndex & 1) {
			sPoint = (tWCoordYX){
				.wX = STAR_CENTER_X + s_pPositions[ubPosIndex].wX,
				.wY = STAR_CENTER_Y + s_pPositions[ubPosIndex].wY
			};
			sNextPoint = (tWCoordYX){
				.wX = STAR_CENTER_X + s_pPositions[ubNextPosIndex].wX / 2,
				.wY = STAR_CENTER_Y + s_pPositions[ubNextPosIndex].wY / 2
			};
		}
		else {
			sPoint = (tWCoordYX){
				.wX = STAR_CENTER_X + s_pPositions[ubPosIndex].wX / 2,
				.wY = STAR_CENTER_Y + s_pPositions[ubPosIndex].wY / 2
			};
			sNextPoint = (tWCoordYX){
				.wX = STAR_CENTER_X + s_pPositions[ubNextPosIndex].wX,
				.wY = STAR_CENTER_Y + s_pPositions[ubNextPosIndex].wY
			};
		}

		blitLinePlane(
			s_pBfrManager->pBack, sPoint.wX, sPoint.wY, sNextPoint.wX, sNextPoint.wY,
			0, 0xFFFF, BLIT_LINE_MODE_XOR, 1
		);
		ubPosIndex = ubNextPosIndex;
	}

	// Fill
	if(keyCheck(KEY_F)) {
		blitFillAligned(
			s_pBfrManager->pBack, uwStartX, STAR_CENTER_Y - STAR_RADIUS - 1,
			uwEndX - uwStartX, (STAR_RADIUS + 1) * 2 + 1, 0, FILL_XOR
		);
	}
	if(keyCheck(KEY_G)) {
		blitFillAligned(
			s_pBfrManager->pBack, uwStartX, STAR_CENTER_Y - STAR_RADIUS - 1,
			uwEndX - uwStartX, (STAR_RADIUS + 1) * 2 + 1, 0, FILL_CARRYIN | FILL_XOR
		);
	}

	if(++s_ubFirstPosIndex >= STAR_POSITION_COUNT) {
		s_ubFirstPosIndex -= STAR_POSITION_COUNT;
	}

	vPortWaitForEnd(s_pVPort);
}

void gsTestLinesDestroy(void) {
	systemUse();
	viewDestroy(s_pView);
}
