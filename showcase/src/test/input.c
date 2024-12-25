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

#define OFFS_JOY1_X 0
#define OFFS_JOY1_Y 0
#define OFFS_JOY2_X 160
#define OFFS_JOY2_Y 0
#define OFFS_JOY3_X 0
#define OFFS_JOY3_Y 80
#define OFFS_JOY4_X 160
#define OFFS_JOY4_Y 80

typedef enum tJoyInputState {
	JOY_PRESSED_NEVER,
	JOY_PRESSED_INACTIVE,
	JOY_PRESSED_ACTIVE,
} tJoyInputState;

typedef struct tButtonDef {
	UWORD uwX;
	UWORD uwY;
	UBYTE ubWidth;
	UBYTE ubHeight;
	const char *szLabel;
} tButtonDef;

static tView *s_pTestInputView;
static tVPort *s_pTestInputVPort;
static tSimpleBufferManager *s_pTestInputBfr;
static tFont *s_pFont;
static tTextBitMap *s_pTextBitMap;
static tJoyInputState s_pJoyStates[4][6];

static const tButtonDef s_pJoyButtonDefs[4][6] = {
	[0] = {
		[JOY_FIRE] = {.uwX = OFFS_JOY1_X + 4 * 20 + 2, .uwY = OFFS_JOY1_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "F1"},
		[JOY_UP] = {.uwX = OFFS_JOY1_X + 1 * 20 + 2, .uwY = OFFS_JOY1_Y + 0 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "U"},
		[JOY_DOWN] = {.uwX = OFFS_JOY1_X + 1 * 20 + 2, .uwY = OFFS_JOY1_Y + 2 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "D"},
		[JOY_LEFT] = {.uwX = OFFS_JOY1_X + 0 * 20 + 2, .uwY = OFFS_JOY1_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "L"},
		[JOY_RIGHT] = {.uwX = OFFS_JOY1_X + 2 * 20 + 2, .uwY = OFFS_JOY1_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "R"},
		[JOY_FIRE2] = {.uwX = OFFS_JOY1_X + 5 * 20 + 2, .uwY = OFFS_JOY1_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "F2"},
	},
	[1] = {
		[JOY_FIRE] = {.uwX = OFFS_JOY2_X + 4 * 20 + 2, .uwY = OFFS_JOY2_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "F1"},
		[JOY_UP] = {.uwX = OFFS_JOY2_X + 1 * 20 + 2, .uwY = OFFS_JOY2_Y + 0 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "U"},
		[JOY_DOWN] = {.uwX = OFFS_JOY2_X + 1 * 20 + 2, .uwY = OFFS_JOY2_Y + 2 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "D"},
		[JOY_LEFT] = {.uwX = OFFS_JOY2_X + 0 * 20 + 2, .uwY = OFFS_JOY2_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "L"},
		[JOY_RIGHT] = {.uwX = OFFS_JOY2_X + 2 * 20 + 2, .uwY = OFFS_JOY2_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "R"},
		[JOY_FIRE2] = {.uwX = OFFS_JOY2_X + 5 * 20 + 2, .uwY = OFFS_JOY2_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "F2"},
	},
	[2] = {
		[JOY_FIRE] = {.uwX = OFFS_JOY3_X + 4 * 20 + 2, .uwY = OFFS_JOY3_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "F1"},
		[JOY_UP] = {.uwX = OFFS_JOY3_X + 1 * 20 + 2, .uwY = OFFS_JOY3_Y + 0 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "U"},
		[JOY_DOWN] = {.uwX = OFFS_JOY3_X + 1 * 20 + 2, .uwY = OFFS_JOY3_Y + 2 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "D"},
		[JOY_LEFT] = {.uwX = OFFS_JOY3_X + 0 * 20 + 2, .uwY = OFFS_JOY3_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "L"},
		[JOY_RIGHT] = {.uwX = OFFS_JOY3_X + 2 * 20 + 2, .uwY = OFFS_JOY3_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "R"},
		[JOY_FIRE2] = {.uwX = OFFS_JOY3_X + 5 * 20 + 2, .uwY = OFFS_JOY3_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "F2"},
	},
	[3] = {
		[JOY_FIRE] = {.uwX = OFFS_JOY4_X + 4 * 20 + 2, .uwY = OFFS_JOY4_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "F1"},
		[JOY_UP] = {.uwX = OFFS_JOY4_X + 1 * 20 + 2, .uwY = OFFS_JOY4_Y + 0 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "U"},
		[JOY_DOWN] = {.uwX = OFFS_JOY4_X + 1 * 20 + 2, .uwY = OFFS_JOY4_Y + 2 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "D"},
		[JOY_LEFT] = {.uwX = OFFS_JOY4_X + 0 * 20 + 2, .uwY = OFFS_JOY4_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "L"},
		[JOY_RIGHT] = {.uwX = OFFS_JOY4_X + 2 * 20 + 2, .uwY = OFFS_JOY4_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "R"},
		[JOY_FIRE2] = {.uwX = OFFS_JOY4_X + 5 * 20 + 2, .uwY = OFFS_JOY4_Y + 1 * 20 + 2, .ubWidth = 16, .ubHeight = 16, .szLabel = "F2"},
	},
};

static void drawButtonAt(const tButtonDef *pDef, UBYTE ubColor) {
	blitRect(s_pTestInputBfr->pBack, pDef->uwX, pDef->uwY, pDef->ubWidth, pDef->ubHeight, ubColor);
	if(pDef->szLabel) {
		fontDrawStr(
			s_pFont, s_pTestInputBfr->pBack, pDef->uwX + pDef->ubWidth / 2, pDef->uwY + pDef->ubHeight / 2,
			pDef->szLabel, 0, FONT_CENTER | FONT_COOKIE, s_pTextBitMap
		);
	}
}

static void updateJoyState(UBYTE ubJoyIndex) {
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

		drawButtonAt(&s_pJoyButtonDefs[ubJoyIndex][ubDir], pPressToColor[s_pJoyStates[ubJoyIndex][ubDir]]);
	}
}

static void showParallelStatus(void) {
	char szMsg[40];
	sprintf(szMsg, "Parallel joys are %s F1 to toggle", joyIsParallelEnabled() ? "ON" : "OFF");
	blitRect(s_pTestInputBfr->pBack, 0, 256 - s_pFont->uwHeight, 320, s_pFont->uwHeight, 0);
	fontDrawStr(s_pFont, s_pTestInputBfr->pBack, 160, 256, szMsg, 3, FONT_BOTTOM|FONT_HCENTER, s_pTextBitMap);
}

void gsTestInputCreate(void) {
	// Prepare view & viewport
	s_pTestInputView = viewCreate(0, TAG_DONE);
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

	s_pFont = fontCreateFromPath("data/fonts/silkscreen.fnt");
	s_pTextBitMap = fontCreateTextBitMap(320, s_pFont->uwHeight);

	fontDrawStr(
		s_pFont, s_pTestInputBfr->pBack, 0 + 160 / 2, 10,
		"Joy 1", 1, FONT_CENTER, s_pTextBitMap
	);
	fontDrawStr(
		s_pFont, s_pTestInputBfr->pBack, 160 + 160 / 2, 10,
		"Joy 2", 1, FONT_CENTER, s_pTextBitMap
	);
	fontDrawStr(
		s_pFont, s_pTestInputBfr->pBack, 0 + 160 / 2, 80 + 10,
		"Joy 3", 1, FONT_CENTER, s_pTextBitMap
	);
	fontDrawStr(
		s_pFont, s_pTestInputBfr->pBack, 160 + 160 / 2, 80 + 10,
		"Joy 4", 1, FONT_CENTER, s_pTextBitMap
	);

	for(UBYTE ubJoy = 0; ubJoy < 4; ++ubJoy) {
		for(UBYTE ubButton = 0; ubButton < 6; ++ubButton) {
			s_pJoyStates[ubJoy][ubButton] = JOY_PRESSED_NEVER;
		}
	}

	fontDrawStr(s_pFont, s_pTestInputBfr->pBack, 0, 160, "TODO Mouse 1", 1, 0, s_pTextBitMap);
	fontDrawStr(s_pFont, s_pTestInputBfr->pBack, 0, 168, "TODO Mouse 2", 1, 0, s_pTextBitMap);
	fontDrawStr(s_pFont, s_pTestInputBfr->pBack, 0, 176, "TODO Keyboard", 1, 0, s_pTextBitMap);
	showParallelStatus();

	// Display view with its viewports
	systemUnuse();
	viewLoad(s_pTestInputView);
}

void gsTestInputLoop(void) {
	if (keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}

	if(keyUse(KEY_F1)) {
		if(joyIsParallelEnabled()) {
			joyDisableParallel();
		}
		else {
			joyEnableParallel();
		}
		showParallelStatus();
	}

	updateJoyState(0);
	updateJoyState(1);
	updateJoyState(2);
	updateJoyState(3);

	vPortWaitForEnd(s_pTestInputVPort);
}

void gsTestInputDestroy(void) {
	viewLoad(0);
	systemUse();
	joyDisableParallel();

	fontDestroy(s_pFont);
	fontDestroyTextBitMap(s_pTextBitMap);

	// Destroy buffer, view & viewport
	viewDestroy(s_pTestInputView);
}
