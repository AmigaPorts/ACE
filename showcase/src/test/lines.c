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

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBfrManager;

void gsTestLinesCreate(void) {
	s_pView = viewCreate(0, TAG_END);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_BPP, 4,
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

	// Prepare circle vertex positions
	const uint8_t uwVertCount = 12;
	const fix16_t fHalf = fix16_one>>1;
	fix16_t fAngle;
	tUwCoordYX pVerts[uwVertCount];
	UWORD uwRadius = 64;
	for(UBYTE v = 0; v != uwVertCount; ++v) {
		fAngle = (fix16_pi*v*2) / uwVertCount;
		pVerts[v].uwX = uwMaxX/2 + fix16_to_int(uwRadius * fix16_sin(fAngle) + fHalf);
		pVerts[v].uwY = uwMaxY/2 + fix16_to_int(uwRadius * fix16_cos(fAngle) + fHalf);
	}

	// Draw circle
	uwPattern = 0xE4E4;
	UBYTE v;
	for(v = 0; v < uwVertCount-1; ++v) {
		blitLine(
			s_pBfrManager->pBack,
			pVerts[v].uwX, pVerts[v].uwY,
			pVerts[v+1].uwX, pVerts[v+1].uwY,
			2, uwPattern, 0
		);
	}
	// Close the circle
	blitLine(
		s_pBfrManager->pBack,
		pVerts[v].uwX, pVerts[v].uwY,
		pVerts[0].uwX, pVerts[0].uwY,
		2, uwPattern, 0
	);

	viewLoad(s_pView);
	systemUnuse();
}

void gsTestLinesLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}

	vPortWaitForEnd(s_pVPort);
}

void gsTestLinesDestroy(void) {
	systemUse();
	viewDestroy(s_pView);
}
