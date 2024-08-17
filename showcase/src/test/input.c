/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "test/blit.h"
#include <ace/utils/extview.h>
#include <ace/utils/font.h>
#include <ace/managers/blit.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/mouse.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/generic/screen.h>
#include "game.h"

#define COLOR_PRESSED_NEVER 4
#define COLOR_PRESSED_INACTIVE 2
#define COLOR_PRESSED_ACTIVE 1

typedef enum tJoyInputState {
	JOY_PRESSED_NEVER,
	JOY_PRESSED_INACTIVE,
	JOY_PRESSED_ACTIVE,
} tJoyInputState;

static tView *s_pTestInputView;
static tVPort *s_pTestInputVPort;
static tSimpleBufferManager *s_pTestInputBfr;
static tFont *s_pFont;
static tTextBitMap *s_pTextBitMap;
static tJoyInputState s_pJoyStates[4][6];

static void drawButtonAt(UWORD uwX, UWORD uwY, UBYTE ubWidth, UBYTE ubHeight, const char *szText, UBYTE ubColor) {
	blitRect(s_pTestInputBfr->pBack, uwX, uwY, ubWidth, ubHeight, ubColor);
	if(szText) {
		fontDrawStr(
			s_pFont, s_pTestInputBfr->pBack, uwX + ubWidth / 2, uwY + ubHeight / 2,
			szText, 0, FONT_CENTER | FONT_COOKIE, s_pTextBitMap
		);
	}
}

static void updateJoyState(UBYTE ubJoyIndex, UWORD uwOffsX, UWORD uwOffsY) {
	static const UBYTE pPressToColor[] = {
		[JOY_PRESSED_NEVER] = COLOR_PRESSED_NEVER,
		[JOY_PRESSED_INACTIVE] = COLOR_PRESSED_INACTIVE,
		[JOY_PRESSED_ACTIVE] = COLOR_PRESSED_ACTIVE,
	};

	UBYTE ubJoy = ubJoyIndex * JOY2;
	for(UBYTE ubDir = 0; ubDir < 6;  ++ubDir) {
		UBYTE ubButtonState = joyCheck(ubJoy + ubDir);
		if(ubButtonState) {
			s_pJoyStates[ubJoyIndex][ubDir] = JOY_PRESSED_ACTIVE;
		}
		else if(s_pJoyStates[ubJoyIndex][ubDir] != JOY_PRESSED_NEVER && ubButtonState == JOY_NACTIVE) {
			s_pJoyStates[ubJoyIndex][ubDir] = JOY_PRESSED_INACTIVE;
		}
	}

	drawButtonAt(uwOffsX + 1 * 20 + 2, uwOffsY + 0 * 20 + 2, 16, 16, "U", pPressToColor[s_pJoyStates[ubJoyIndex][JOY_UP]]);
	drawButtonAt(uwOffsX + 1 * 20 + 2, uwOffsY + 2 * 20 + 2, 16, 16, "D", pPressToColor[s_pJoyStates[ubJoyIndex][JOY_DOWN]]);
	drawButtonAt(uwOffsX + 0 * 20 + 2, uwOffsY + 1 * 20 + 2, 16, 16, "L", pPressToColor[s_pJoyStates[ubJoyIndex][JOY_LEFT]]);
	drawButtonAt(uwOffsX + 2 * 20 + 2, uwOffsY + 1 * 20 + 2, 16, 16, "R", pPressToColor[s_pJoyStates[ubJoyIndex][JOY_RIGHT]]);
	drawButtonAt(uwOffsX + 4 * 20 + 2, uwOffsY + 1 * 20 + 2, 16, 16, "F1", pPressToColor[s_pJoyStates[ubJoyIndex][JOY_FIRE]]);
	drawButtonAt(uwOffsX + 5 * 20 + 2, uwOffsY + 1 * 20 + 2, 16, 16, "F2", pPressToColor[s_pJoyStates[ubJoyIndex][JOY_FIRE2]]);
}

void gsTestInputCreate(void) {
	// Prepare view & viewport
	s_pTestInputView = viewCreate(0,
		TAG_VIEW_GLOBAL_PALETTE, 1,
		TAG_DONE
	);
	s_pTestInputVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pTestInputView,
		TAG_VPORT_BPP, SHOWCASE_BPP,
		TAG_DONE
	);
	s_pTestInputBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pTestInputVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
		TAG_DONE
	);
	s_pTestInputVPort->pPalette[0] = 0x000;
	s_pTestInputVPort->pPalette[1] = 0xAAA;
	s_pTestInputVPort->pPalette[2] = 0x666;
	s_pTestInputVPort->pPalette[3] = 0xFFF;
	s_pTestInputVPort->pPalette[4] = 0x333;

	s_pFont = fontCreate("data/fonts/silkscreen.fnt");
	s_pTextBitMap = fontCreateTextBitMap(320, s_pFont->uwHeight);

	fontDrawStr(
		s_pFont, s_pTestInputBfr->pBack, 0 + 160 / 2, 10,
		"Joy 1", 1, FONT_CENTER, s_pTextBitMap
	);
	fontDrawStr(
		s_pFont, s_pTestInputBfr->pBack, 160 + 160 / 2, 10,
		"Joy 2", 1, FONT_CENTER, s_pTextBitMap
	);

	for(UBYTE ubJoy = 0; ubJoy < 4; ++ubJoy) {
		for(UBYTE ubButton = 0; ubButton < 6; ++ubButton) {
			s_pJoyStates[ubJoy][ubButton] = JOY_PRESSED_NEVER;
		}
	}

	// Display view with its viewports
	systemUnuse();
	viewLoad(s_pTestInputView);
}

void gsTestInputLoop(void) {
	if (keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}

	updateJoyState(0, 0, 20);
	updateJoyState(1, 160, 20);

	vPortWaitForEnd(s_pTestInputVPort);
}

void gsTestInputDestroy(void) {
	viewLoad(0);
	systemUse();

	fontDestroy(s_pFont);
	fontDestroyTextBitMap(s_pTextBitMap);

	// Destroy buffer, view & viewport
	viewDestroy(s_pTestInputView);
}
