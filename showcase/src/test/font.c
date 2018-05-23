/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "test/font.h"

#include <ace/managers/game.h>
#include <ace/managers/blit.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/extview.h>
#include <ace/utils/font.h>
#include <ace/generic/screen.h>

#include "main.h"
#include "menu/menu.h"

static tView *s_pTestFontView;
static tVPort *s_pTestFontVPort;
static tSimpleBufferManager *s_pTestFontBfr;

static char s_szSentence[20];
static tFont *s_pFontUI;
static tTextBitMap *s_pGlyph, *s_pGlyphCode;
static UBYTE s_ubPage;

void gsTestFontCreate(void) {
	// Prepare view & viewport
	s_pTestFontView = viewCreate(0,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_DONE
	);
	s_pTestFontVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pTestFontView,
		TAG_VPORT_BPP, SHOWCASE_BPP,
		TAG_DONE
	);
	s_pTestFontBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pTestFontVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
		TAG_DONE
	);
	s_pTestFontVPort->pPalette[0] = 0x000;
	s_pTestFontVPort->pPalette[1] = 0xAAA;
	s_pTestFontVPort->pPalette[2] = 0x666;
	s_pTestFontVPort->pPalette[3] = 0xFFF;
	s_pTestFontVPort->pPalette[4] = 0x111;

	// Load fonts
	s_pFontUI = fontCreate("data/fonts/silkscreen.fnt");
	s_pGlyph = 0;
	s_pGlyph = fontCreateTextBitMap(100, s_pFontUI->uwHeight);
	s_pGlyphCode = fontCreateTextBitMap(100, s_pFontUI->uwHeight);

	// Loop vars
	s_ubPage = 0;
	testFontDrawTable();
	memset(s_szSentence, 0, 20);

	// Display view with its viewports
	systemUnuse();
	viewLoad(s_pTestFontView);
}

void gsTestFontTableLoop(void) {
	if (keyUse(KEY_ESCAPE)) {
		gameChangeState(gsMenuCreate, gsMenuLoop, gsMenuDestroy);
		return;
	}

	if(keyUse(KEY_F2)) {
		testFontDrawSentence();
		gameChangeLoop(gsTestFontSentenceLoop);
	}

	if((keyUse(KEY_RIGHT) || keyUse(KEY_DOWN))) {
		if(s_ubPage < 3) {
				++s_ubPage;
		}
		else {
			s_ubPage = 0;
		}
		testFontDrawTable();
	}
	if((keyUse(KEY_LEFT) || keyUse(KEY_UP))) {
		if(s_ubPage) {
			--s_ubPage;
		}
		else {
			s_ubPage = 3;
		}
		testFontDrawTable();
	}

}

void gsTestFontSentenceLoop(void) {
	UBYTE isRedrawNeeded = 0;
	static const char szAllowedChars[] =
		"0123456789" "abcdefghijklmnopqrstuvwxyz" "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	UBYTE ubAllowedCharsCount = strlen(szAllowedChars);

	if (keyUse(KEY_ESCAPE)) {
		gameChangeState(gsMenuCreate, gsMenuLoop, gsMenuDestroy);
		return;
	}

	if(keyUse(KEY_F1)) {
		testFontDrawTable();
		gameChangeLoop(gsTestFontTableLoop);
	}

	isRedrawNeeded = 0;
	if(keyUse(KEY_BACKSPACE)) {
		UBYTE ubSentenceLength = strlen(s_szSentence);
		if(ubSentenceLength) {
			s_szSentence[ubSentenceLength-1] = 0;
			isRedrawNeeded = 1;
		}
	}

	for(UBYTE i = 0; i < ubAllowedCharsCount; ++i) {
		if(keyUse(szAllowedChars[i])) {
			UBYTE ubSentenceLength = strlen(s_szSentence);
			if(ubSentenceLength != 20) {
				s_szSentence[ubSentenceLength] = szAllowedChars[i];
				isRedrawNeeded = 1;
			}
		}
	}

	if(isRedrawNeeded) {
		testFontDrawSentence();
	}
}

void gsTestFontDestroy(void) {
	systemUse();
	// Free fonts
	fontDestroyTextBitMap(s_pGlyphCode);
	fontDestroyTextBitMap(s_pGlyph);
	fontDestroy(s_pFontUI);

	// Destroy buffer, view & viewport
	viewDestroy(s_pTestFontView);
}

void testFontDrawTable(void) {
	tFont *pFont;
	UWORD i;
	char szCodeBfr[3];

	pFont = s_pFontUI;

	// Background
	blitRect(
		s_pTestFontBfr->pBack, 0,0,
		s_pTestFontBfr->uBfrBounds.sUwCoord.uwX,
		s_pTestFontBfr->uBfrBounds.sUwCoord.uwY, 2
	);
	UWORD uwMaxX = s_pTestFontBfr->uBfrBounds.sUwCoord.uwX-1;
	UWORD uwMaxY = s_pTestFontBfr->uBfrBounds.sUwCoord.uwY-1;
	for(i = 0; i < 8; ++i) {
		// Vertical lines
		blitLine(s_pTestFontBfr->pBack, 40*i, 0, 40*i, uwMaxY, 0, 0xFFFF, 0);
		// Horizontal lines
		blitLine(s_pTestFontBfr->pBack, 0, 32*i, uwMaxX, 32*i, 0, 0xFFFF, 0);
	}
	// Last V line
	blitLine(s_pTestFontBfr->pBack, uwMaxX, 0, uwMaxX, uwMaxY, 0, 0xFFFF, 0);
	// Last H line
	blitLine(s_pTestFontBfr->pBack, 0, uwMaxY, uwMaxX, uwMaxY, 0, 0xFFFF, 0);

	for(i = 0; i < 64; ++i) {
		UBYTE ubCharIdx = s_ubPage*64+i;
		// Char - crashes because of font rendering bugs
		if(
			ubCharIdx && // Not a null char
			pFont->pCharOffsets[ubCharIdx] < pFont->pCharOffsets[ubCharIdx+1] &&
			ubCharIdx < pFont->ubChars
		) {
			sprintf(szCodeBfr, "%c", ubCharIdx);
			fontFillTextBitMap(pFont, s_pGlyph, szCodeBfr);
			fontDrawTextBitMap(
				s_pTestFontBfr->pBack, s_pGlyph,
				(i/8)*40+40/2, (i%8)*32+(32/2),
				3, FONT_CENTER|FONT_COOKIE
			);
		}

		// Char code
		sprintf(szCodeBfr, "%02X", ubCharIdx);
		fontFillTextBitMap(s_pFontUI, s_pGlyphCode, szCodeBfr);
		fontDrawTextBitMap(
			s_pTestFontBfr->pBack, s_pGlyphCode,
			(i/8)*40+40/2, (i%8)*32+32-2,
			0, FONT_HCENTER|FONT_BOTTOM|FONT_COOKIE
		);
	}
}

void testFontDrawSentence(void) {

}
