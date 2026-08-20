/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Showcase: mouse cursor as a hardware sprite on channel 0.
 * Arrow and pencil cursors are 16px 2BPP .bm files inside data/sprites.pak.
 * LMB swaps to the pencil and paints.
 */

#include "test/mouse_sprite.h"
#include <ace/generic/screen.h>
#include <ace/managers/blit.h>
#include <ace/managers/copper.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/sprite.h>
#include <ace/managers/system.h>
#include <hardware/dmabits.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/custom.h>
#include <ace/utils/extview.h>
#include <ace/utils/font.h>
#include <ace/utils/pak_file.h>
#include <ace/managers/memory.h>
#include "game.h"

#define COLOR_BG 0
#define COLOR_TEXT 1
#define COLOR_HINT 2
#define COLOR_DOT 3
/* Sprite ch0 uses palette 16–19: outline / fill / lead. */
#define COLOR_SPR_OUTLINE 17
#define COLOR_SPR_FILL 18
#define COLOR_SPR_LEAD 19

#define ARROW_HOT_X 0
#define ARROW_HOT_Y 0
#define PENCIL_HOT_X 6
#define PENCIL_HOT_Y 15

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBfr;
static tFont *s_pFont;
static tTextBitMap *s_pTextBitMap;
static tBitMap *s_pBmArrow, *s_pBmPencil;
static tPakFile *s_pPak;
static tSprite *s_pPtrSprite;
static char *s_szHudCoords;
static UWORD s_uwLastX, s_uwLastY;
static UWORD s_uwHudCoordX, s_uwHudCoordW, s_uwHudH;
static UBYTE s_ubWasLmb;
static BYTE s_bHotX, s_bHotY;

static void setCursorPos(WORD wX, WORD wY) {
	s_pPtrSprite->wX = (WORD)(wX - s_bHotX);
	s_pPtrSprite->wY = (WORD)(wY - s_bHotY);
	spriteRequestMetadataUpdate(s_pPtrSprite);
}

static void setCursorBitmap(tBitMap *pBm, BYTE bHotX, BYTE bHotY, WORD wX, WORD wY) {
	s_bHotX = bHotX;
	s_bHotY = bHotY;
	spriteSetBitmap(s_pPtrSprite, pBm);
	setCursorPos(wX, wY);
}

static void drawHudCoords(void) {
	UWORD uwX, uwY;

	if(!s_pFont || !s_szHudCoords) {
		return;
	}
	uwX = mouseGetX(MOUSE_PORT_1);
	uwY = mouseGetY(MOUSE_PORT_1);
	if(uwX > 999) {
		uwX = 999;
	}
	if(uwY > 999) {
		uwY = 999;
	}
	s_szHudCoords[0] = (char)('0' + (uwX / 100));
	s_szHudCoords[1] = (char)('0' + ((uwX / 10) % 10));
	s_szHudCoords[2] = (char)('0' + (uwX % 10));
	s_szHudCoords[3] = ' ';
	s_szHudCoords[4] = (char)('0' + (uwY / 100));
	s_szHudCoords[5] = (char)('0' + ((uwY / 10) % 10));
	s_szHudCoords[6] = (char)('0' + (uwY % 10));
	s_szHudCoords[7] = '\0';
	blitRect(s_pBfr->pBack, s_uwHudCoordX, 0, s_uwHudCoordW, s_uwHudH, COLOR_BG);
	fontDrawStr(
		s_pFont, s_pBfr->pBack, s_uwHudCoordX, 1, s_szHudCoords,
		COLOR_TEXT, FONT_LAZY, s_pTextBitMap
	);
}

static void drawHudStatic(void) {
	tUwCoordYX sPrefix, sCoords, sSuffix;
	UWORD uwRightX;

	if(!s_pFont) {
		return;
	}

	sPrefix = fontMeasureText(s_pFont, "Mouse sprite");
	sCoords = fontMeasureText(s_pFont, "888 888");
	sSuffix = fontMeasureText(s_pFont, "LMB pencil  RMB clear");
	s_uwHudH = (UWORD)(s_pFont->uwHeight + 2);
	s_uwHudCoordX = (UWORD)((4 + sPrefix.uwX + 8 + 15) & ~15);
	s_uwHudCoordW = (UWORD)((sCoords.uwX + 15) & ~15);
	uwRightX = (UWORD)(s_uwHudCoordX + s_uwHudCoordW + 8);
	if(uwRightX + sSuffix.uwX > SCREEN_PAL_WIDTH) {
		uwRightX = (UWORD)(SCREEN_PAL_WIDTH - sSuffix.uwX);
	}

	fontDrawStr(
		s_pFont, s_pBfr->pBack, 4, 1, "Mouse sprite",
		COLOR_TEXT, FONT_LAZY, s_pTextBitMap
	);
	fontDrawStr(
		s_pFont, s_pBfr->pBack, uwRightX, 1, "LMB pencil  RMB clear",
		COLOR_TEXT, FONT_LAZY, s_pTextBitMap
	);
	fontDrawStr(
		s_pFont, s_pBfr->pBack, 4, SCREEN_PAL_HEIGHT - s_pFont->uwHeight - 2,
		"ESC back  ch0  arrow/pencil from sprites pak",
		COLOR_HINT, FONT_LAZY, s_pTextBitMap
	);
	drawHudCoords();
}

static void paintDot(UWORD uwX, UWORD uwY) {
	if(uwX > SCREEN_PAL_WIDTH - 3) {
		uwX = SCREEN_PAL_WIDTH - 3;
	}
	if(uwY < 12) {
		uwY = 12;
	}
	if(uwY > SCREEN_PAL_HEIGHT - 3) {
		uwY = SCREEN_PAL_HEIGHT - 3;
	}
	blitRect(s_pBfr->pBack, uwX, uwY, 2, 2, COLOR_DOT);
}

void gsTestMouseSpriteCreate(void) {
	s_pView = viewCreate(0, TAG_DONE);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, SHOWCASE_BPP,
		TAG_DONE
	);
	s_pBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
		TAG_DONE
	);

	s_pVPort->pPalette[COLOR_BG] = 0x024;
	s_pVPort->pPalette[COLOR_TEXT] = 0xFFF;
	s_pVPort->pPalette[COLOR_HINT] = 0xAAA;
	s_pVPort->pPalette[COLOR_DOT] = 0xF80;
	s_pVPort->pPalette[COLOR_SPR_OUTLINE] = 0x000;
	s_pVPort->pPalette[COLOR_SPR_FILL] = 0xFFF;
	s_pVPort->pPalette[COLOR_SPR_LEAD] = 0xF80;

	s_pFont = fontCreateFromPath("data/silkscreen.fnt");
	s_pTextBitMap = s_pFont
		? fontCreateTextBitMap(SCREEN_PAL_WIDTH, s_pFont->uwHeight)
		: 0;
	s_szHudCoords = memAllocFast(16);

	mouseCreate(MOUSE_PORT_1);
	mouseSetBounds(MOUSE_PORT_1, 0, 0, SCREEN_PAL_WIDTH - 1, SCREEN_PAL_HEIGHT - 1);
	mouseSetPosition(
		MOUSE_PORT_1, SCREEN_PAL_WIDTH / 2, SCREEN_PAL_HEIGHT / 2
	);
#ifdef AMIGA
	{
		UWORD uwDat = g_pCustom->joy0dat;
		g_sMouseManager.pMice[MOUSE_PORT_1].ubPrevHwX = (UBYTE)(uwDat & 0xFF);
		g_sMouseManager.pMice[MOUSE_PORT_1].ubPrevHwY = (UBYTE)(uwDat >> 8);
	}
#endif

	s_pPak = pakFileOpen("data/sprites.pak", 1);
	if(s_pPak) {
		s_pBmArrow = bitmapCreateFromFd(pakFileGetFileByPath(s_pPak, "arrow.bm"), 0);
		s_pBmPencil = bitmapCreateFromFd(pakFileGetFileByPath(s_pPak, "pencil.bm"), 0);
		pakFileClose(s_pPak);
		s_pPak = 0;
	}

	spriteManagerCreate(s_pView, 0, 0);
	systemSetDmaBit(DMAB_SPRITE, 1);
	s_pPtrSprite = spriteAdd(0, s_pBmArrow);
	s_bHotX = ARROW_HOT_X;
	s_bHotY = ARROW_HOT_Y;
	s_uwLastX = mouseGetX(MOUSE_PORT_1);
	s_uwLastY = mouseGetY(MOUSE_PORT_1);
	setCursorPos((WORD)s_uwLastX, (WORD)s_uwLastY);
	spriteProcess(s_pPtrSprite);
	spriteProcessChannel(0);

	s_uwLastX = 0xFFFF;
	s_uwLastY = 0xFFFF;
	s_ubWasLmb = 0;
	drawHudStatic();

	viewLoad(s_pView);
	systemUnuse();
}

void gsTestMouseSpriteLoop(void) {
	UWORD uwX, uwY;
	UBYTE isLmb;

	if(keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}

	mouseProcess();
	uwX = mouseGetX(MOUSE_PORT_1);
	uwY = mouseGetY(MOUSE_PORT_1);
	isLmb = mouseCheck(MOUSE_PORT_1, MOUSE_LMB);

	if(isLmb != s_ubWasLmb) {
		if(isLmb) {
			setCursorBitmap(s_pBmPencil, PENCIL_HOT_X, PENCIL_HOT_Y, (WORD)uwX, (WORD)uwY);
		}
		else {
			setCursorBitmap(s_pBmArrow, ARROW_HOT_X, ARROW_HOT_Y, (WORD)uwX, (WORD)uwY);
		}
		s_ubWasLmb = isLmb;
	}

	if(uwX != s_uwLastX || uwY != s_uwLastY) {
		setCursorPos((WORD)uwX, (WORD)uwY);
		s_uwLastX = uwX;
		s_uwLastY = uwY;
		drawHudCoords();
	}

	if(isLmb) {
		paintDot(uwX, uwY);
	}
	if(mouseUse(MOUSE_PORT_1, MOUSE_RMB)) {
		UWORD uwTop = s_pFont ? (UWORD)(s_pFont->uwHeight + 2) : 0;
		UWORD uwBot = s_pFont ? (UWORD)(s_pFont->uwHeight + 2) : 0;
		blitRect(
			s_pBfr->pBack, 0, uwTop,
			SCREEN_PAL_WIDTH, SCREEN_PAL_HEIGHT - uwTop - uwBot,
			COLOR_BG
		);
	}

	spriteProcess(s_pPtrSprite);
	spriteProcessChannel(0);
	copProcessBlocks();
	vPortWaitForEnd(s_pVPort);
}

void gsTestMouseSpriteDestroy(void) {
	viewLoad(0);
	systemUse();
	systemSetDmaBit(DMAB_SPRITE, 0);
	spriteManagerDestroy();
	if(s_pBmArrow) {
		bitmapDestroy(s_pBmArrow);
	}
	if(s_pBmPencil) {
		bitmapDestroy(s_pBmPencil);
	}
	mouseDestroy();
	if(s_szHudCoords) {
		memFree(s_szHudCoords, 16);
		s_szHudCoords = 0;
	}
	if(s_pTextBitMap) {
		fontDestroyTextBitMap(s_pTextBitMap);
	}
	if(s_pFont) {
		fontDestroy(s_pFont);
	}
	if(s_pPak) {
		pakFileClose(s_pPak);
		s_pPak = 0;
	}
	viewDestroy(s_pView);
}
