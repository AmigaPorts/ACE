/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "menu/menu.h"
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/game.h>
#include <ace/managers/state.h>
#include <ace/managers/blit.h>
#include <ace/managers/system.h>
#include <ace/utils/extview.h>
#include <ace/generic/screen.h>
#include "game.h"
#include "menu/menulist.h"

static tView *s_pMenuView;
static tVPort *s_pMenuVPort;
static tSimpleBufferManager *s_pMenuBfr;

static tFont *s_pMenuFont;
static tTextBitMap *s_pTextBitMap;
static tMenuList *s_pMenuList; /// Menu list
static UBYTE s_ubMenuType;     /// Current menu list - see MENU_* macros

#define MENU_TITLE_LIST_GAP 12

static void menuLayoutVertCenter(
	tMenuList *pList, UBYTE ubItemCount,
	UWORD *puwTitleY, UWORD *puwListY
) {
	UWORD uwLineStep = menuListGetLineStep(pList);
	UWORD uwTitleBand = uwLineStep;
	UWORD uwListH = ubItemCount * uwLineStep;
	UWORD uwTotal = uwTitleBand + MENU_TITLE_LIST_GAP + uwListH;
	UWORD uwTop = (s_pMenuBfr->uBfrBounds.uwY - uwTotal) / 2;

	*puwTitleY = uwTop + (uwTitleBand >> 1);
	*puwListY = uwTop + uwTitleBand + MENU_TITLE_LIST_GAP;
}

void gsMenuCreate(void) {
	logBlockBegin("gsMenuCreate");
	// Prepare view & viewport
	s_pMenuView = viewCreate(0, TAG_DONE);
	s_pMenuVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pMenuView,
		TAG_VPORT_BPP, SHOWCASE_BPP,
		TAG_DONE
	);
	s_pMenuBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pMenuVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
		TAG_DONE
	);

	// Prepare palette
	s_pMenuVPort->pPalette[0] = 0x000;
	s_pMenuVPort->pPalette[1] = 0xAAA;
	s_pMenuVPort->pPalette[2] = 0x666;
	s_pMenuVPort->pPalette[3] = 0xFFF;
	s_pMenuVPort->pPalette[4] = 0x111;

	// Load font
	s_pMenuFont = fontCreateFromPath("data/silkscreen.fnt");
	s_pTextBitMap = fontCreateTextBitMap(320, s_pMenuFont->uwHeight);

	// Prepare menu lists
	s_pMenuList = menuListCreate(
		160, 100, TEST_STATE_COUNT, 4,
		s_pMenuFont, FONT_HCENTER|FONT_COOKIE|FONT_SHADOW,
		1, 2, 3,
		s_pMenuBfr->pBack
	);
	menuShowMain();

	// Display view with its viewports
	viewLoad(s_pMenuView);
	logBlockEnd("gsMenuCreate");
	systemUnuse();
}

void gsMenuLoop(void) {
	UBYTE ubSelected;

	if (keyUse(KEY_ESCAPE)) {
		if(s_ubMenuType == MENU_MAIN) {
			gameExit();
		}
		else {
			menuShowMain();
		}
		return;
	}

	// Menu list nav - up & down
	if(keyUse(KEY_UP) || joyUse(JOY1_UP)) {
		menuListMove(s_pMenuList, -1);
	}
	else if(keyUse(KEY_DOWN) || joyUse(JOY1_DOWN)) {
		menuListMove(s_pMenuList, +1);
	}

	// Menu list selection
	else if(keyUse(KEY_RETURN) || joyUse(JOY1_FIRE)) {
		ubSelected = s_pMenuList->ubSelected;
		if(s_pMenuList->pEntries[ubSelected].ubDisplayMode == MENULIST_ENABLED) {
			switch(s_ubMenuType) {
				case MENU_MAIN: menuSelectMain(); return;
				case MENU_TESTS: menuSelectTests(); return;
				case MENU_EXAMPLES: menuSelectExamples(); return;
			}
		}
	}
	vPortWaitForEnd(s_pMenuVPort);

	static UBYTE ubFrameCnt = 0;
	static UBYTE ubColor = 1;
	static BYTE bDir = 1;
	++ubFrameCnt;
	if(ubFrameCnt == 7) {
		if(ubColor >= 7) {
			bDir = -1;
		}
		else if(ubColor <= 1) {
			bDir = 1;
		}
		ubColor += bDir;
		ubFrameCnt = 0;
	}
	g_pCustom->color[4] = (ubColor << 8) | (ubColor << 4) | (ubColor);
}

void gsMenuDestroy(void) {
	systemUse();
	logBlockBegin("gsMenuDestroy()");

	menuListDestroy(s_pMenuList);
	fontDestroyTextBitMap(s_pTextBitMap);
	fontDestroy(s_pMenuFont);
	viewDestroy(s_pMenuView);

	logBlockEnd("gsMenuDestroy()");
}

void menuDrawBg(void) {
	logBlockBegin("menuDrawBg()");
	UWORD uwX, uwY;
	UBYTE isOdd = 0;

	// Draw checkerboard
	for(uwY = 0; uwY <= s_pMenuBfr->uBfrBounds.uwY - 16; uwY += 16) {
		for(uwX = 0; uwX <= s_pMenuBfr->uBfrBounds.uwX - 16; uwX += 16) {
			UBYTE ubColor = isOdd ? 0 : 4;
			blitRect(s_pMenuBfr->pBack, uwX, uwY, 16, 16, ubColor);
			isOdd = !isOdd;
		}
		isOdd = !isOdd;
	}

	// Draw border
	UWORD uwMaxX = s_pMenuBfr->uBfrBounds.uwX-1;
	UWORD uwMaxY = s_pMenuBfr->uBfrBounds.uwY-1;
	blitLine(s_pMenuBfr->pBack, 0, 0, uwMaxX, 0, 1, 0xFFFF, 0);
	blitLine(s_pMenuBfr->pBack, 0, uwMaxY, uwMaxX, uwMaxY, 1, 0xFFFF, 0);
	blitLine(s_pMenuBfr->pBack, 0, 0, 0, uwMaxY, 1, 0xFFFF, 0);
	blitLine(s_pMenuBfr->pBack, uwMaxX, 0, uwMaxX, uwMaxY, 1, 0xFFFF, 0);
	logBlockEnd("menuDrawBg()");
}

/******************************************************* Main menu definition */

void menuShowMain(void) {
	systemUse();
	UWORD uwTitleY, uwListY;

	logWrite("menuShowMain\n");
	// Draw BG
	menuDrawBg();
	menuLayoutVertCenter(s_pMenuList, 3, &uwTitleY, &uwListY);
	fontDrawStr(
		s_pMenuFont, s_pMenuBfr->pBack, s_pMenuBfr->uBfrBounds.uwX >> 1, uwTitleY,
		"ACE Showcase", 1, FONT_COOKIE|FONT_CENTER|FONT_SHADOW, s_pTextBitMap
	);

	// Prepare new list
	s_pMenuList->sCoord.uwX = s_pMenuBfr->uBfrBounds.uwX >> 1;
	s_pMenuList->sCoord.uwY = uwListY;
	menuListResetCount(s_pMenuList, 3);
	menuListSetEntry(s_pMenuList, 0, MENULIST_ENABLED, "Tests");
	menuListSetEntry(s_pMenuList, 1, MENULIST_DISABLED, "Examples");
	menuListSetEntry(s_pMenuList, 2, MENULIST_ENABLED, "Quit");
	s_ubMenuType = MENU_MAIN;

	// Redraw list
	menuListDraw(s_pMenuList);
	systemUnuse();
}

void menuSelectMain(void) {
	switch(s_pMenuList->ubSelected) {
		case 0:
			menuShowTests();
			break;
		case 1:
			menuShowExamples();
			break;
		case 2:
			gameExit();
			return;
	}
}

/******************************************************* Test menu definition */

void menuShowTests(void) {
	systemUse();
	UWORD uwTitleY, uwListY;

	// Draw BG
	menuDrawBg();
	menuLayoutVertCenter(s_pMenuList, TEST_STATE_COUNT, &uwTitleY, &uwListY);
	fontDrawStr(
		s_pMenuFont, s_pMenuBfr->pBack, s_pMenuBfr->uBfrBounds.uwX >> 1, uwTitleY,
		"Tests", 1, FONT_COOKIE|FONT_CENTER|FONT_SHADOW, s_pTextBitMap
	);

	s_pMenuList->sCoord.uwX = s_pMenuBfr->uBfrBounds.uwX >> 1;
	s_pMenuList->sCoord.uwY = uwListY;
	menuListResetCount(s_pMenuList, TEST_STATE_COUNT);
	menuListSetEntry(s_pMenuList, TEST_STATE_MENU, MENULIST_ENABLED, "Back");
	menuListSetEntry(s_pMenuList, TEST_STATE_BLIT, MENULIST_ENABLED, "Blits");
	menuListSetEntry(s_pMenuList, TEST_STATE_INPUT, MENULIST_ENABLED, "Input");
	menuListSetEntry(s_pMenuList, TEST_STATE_FONT, MENULIST_ENABLED, "Fonts");
	menuListSetEntry(s_pMenuList, TEST_STATE_COPPER, MENULIST_ENABLED, "Copper");
	menuListSetEntry(s_pMenuList, TEST_STATE_LINES, MENULIST_ENABLED, "Blitter shapes");
	menuListSetEntry(s_pMenuList, TEST_STATE_BLIT_SMALL_DEST, MENULIST_ENABLED, "Blits with small dst");
	menuListSetEntry(s_pMenuList, TEST_STATE_INTERLEAVED, MENULIST_ENABLED, "Interleaved bitmaps");
	menuListSetEntry(s_pMenuList, TEST_STATE_BUFFER_SCROLL, MENULIST_ENABLED, "Scroll buffer wrap");
	menuListSetEntry(s_pMenuList, TEST_STATE_BUFFER_REUSE, MENULIST_ENABLED, "Shared buffer reuse");
	menuListSetEntry(s_pMenuList, TEST_STATE_TWISTER, MENULIST_ENABLED, "Twister");
	s_ubMenuType = MENU_TESTS;

	// Redraw list
	menuListDraw(s_pMenuList);
	systemUnuse();
}

void menuSelectTests(void) {
	if (s_pMenuList->ubSelected) {
		stateChange(g_pGameStateManager, &g_pTestStates[s_pMenuList->ubSelected]);
	}
	else {
		menuShowMain();
	}
}

/*************************************************** Examples menu definition */

void menuShowExamples(void) {
	systemUse();
	UWORD uwTitleY, uwListY;

	// Draw BG
	menuDrawBg();
	menuLayoutVertCenter(s_pMenuList, 1, &uwTitleY, &uwListY);
	fontDrawStr(
		s_pMenuFont, s_pMenuBfr->pBack, s_pMenuBfr->uBfrBounds.uwX >> 1, uwTitleY,
		"Examples", 1, FONT_COOKIE|FONT_CENTER|FONT_SHADOW, s_pTextBitMap
	);

	// Prepare new list
	s_pMenuList->sCoord.uwX = s_pMenuBfr->uBfrBounds.uwX >> 1;
	s_pMenuList->sCoord.uwY = uwListY;
	menuListResetCount(s_pMenuList, 1);
	menuListSetEntry(s_pMenuList, 0, MENULIST_ENABLED, "Back");
	s_ubMenuType = MENU_EXAMPLES;

	// Redraw list
	menuListDraw(s_pMenuList);
	systemUnuse();
}

void menuSelectExamples(void) {
	switch(s_pMenuList->ubSelected) {
		case 0:
			menuShowMain();
			break;
	}
}
