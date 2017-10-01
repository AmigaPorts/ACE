#include "test/font.h"

#include <ace/managers/game.h>
#include <ace/managers/blit.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/extview.h>
#include <ace/utils/font.h>

#include "menu/menu.h"

tView *s_pTestFontView;
tVPort *s_pTestFontVPort;
tSimpleBufferManager *s_pTestFontBfr;
char s_szSentence[20];

tFont *s_pFontUI;
UBYTE s_ubPage;

void gsTestFontCreate(void) {
	// Prepare view & viewport
	s_pTestFontView = viewCreate(0,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_DONE
	);
	s_pTestFontVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pTestFontView,
		TAG_VPORT_BPP, WINDOW_SCREEN_BPP,
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
	
	// Loop vars
	s_ubPage = 0;
	testFontDrawTable();
	memset(s_szSentence, 0, 20);
	
	// Display view with its viewports
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
		if(s_ubPage < 3)
				++s_ubPage;
		else
			s_ubPage = 0;
		testFontDrawTable();
	}
	if((keyUse(KEY_LEFT) || keyUse(KEY_UP))) {
		if(s_ubPage)
			--s_ubPage;
		else
				s_ubPage = 3;
		testFontDrawTable();
	}
	
}

void gsTestFontSentenceLoop(void) {
	UBYTE i, d, ubRedraw;
	static const UBYTE szAllowedChars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	d = strlen(szAllowedChars);
	
	if (keyUse(KEY_ESCAPE)) {
		gameChangeState(gsMenuCreate, gsMenuLoop, gsMenuDestroy);
		return;
	}

	if(keyUse(KEY_F1)) {
		testFontDrawTable();
		gameChangeLoop(gsTestFontTableLoop);
	}
	
	ubRedraw = 0;
	if(keyUse(KEY_BACKSPACE)) {
		d = strlen(s_szSentence);
		if(d) {
			s_szSentence[d-1] = 0;
			ubRedraw = 1;
		}
	}
	
	for(i = 0; i != d; ++i)
		if(keyUse(szAllowedChars[i])) {
			d = strlen(s_szSentence);
			if(d != 20) {
				s_szSentence[d] = szAllowedChars[i];
				ubRedraw = 1;
			}
		}
		
	if(ubRedraw)
		testFontDrawSentence();
}

void gsTestFontDestroy(void) {
	// Free fonts
	fontDestroy(s_pFontUI);
	
	// Destroy buffer, view & viewport
	viewDestroy(s_pTestFontView);
}

void testFontDrawTable() {
	tFont *pFont;
	UWORD i;
	char szCodeBfr[3];
	
	pFont = s_pFontUI;
	
	// Background
	blitRect(s_pTestFontBfr->pBuffer, 0,0, 320,256, 2);
	for(i = 0; i != 8; ++i) {
		blitRect(s_pTestFontBfr->pBuffer, 40*i, 0, 1, 256, 0); // Vertical lines
		blitRect(s_pTestFontBfr->pBuffer, 0, 32*i, 320, 1, 0); // Horizontal lines
	}
	blitRect(s_pTestFontBfr->pBuffer, 319, 0, 1, 256, 0); // Last V line
	blitRect(s_pTestFontBfr->pBuffer, 0, 255, 320, 1, 0); // Last H line
	
	for(i = 0; i != 64; ++i) {
		// Char - crashes because of font rendering bugs
		// if(i && pFont->pCharOffsets[i] != pFont->pCharOffsets[i-1] && (s_ubPage*64)+i < pFont->ubChars) {
			// sprintf(szCodeBfr, "%c", (s_ubPage*64)+i);
			// fontDrawStr(
				// s_pTestFontBfr->pBuffer, pFont,
				// ((i/8)*40+40/2), (i%8)*32+(32/2),
				// szCodeBfr, 3, FONT_CENTER|FONT_COOKIE
			// );
		// }
		
		// Char code
		sprintf(szCodeBfr, "%02X", (s_ubPage*64)+i);
		fontDrawStr(
			s_pTestFontBfr->pBuffer, s_pFontUI,
			((i/8)*40+40/2), (i%8)*32+32-2,
			szCodeBfr, 0, FONT_HCENTER|FONT_BOTTOM|FONT_COOKIE
		);
	}
}

void testFontDrawSentence(void) {
	
}