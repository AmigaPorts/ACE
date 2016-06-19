#include "test/blit.h"

#include <ace/utils/extview.h>
#include <ace/managers/game.h>
#include <ace/managers/copper.h>
#include <ace/managers/blit.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/viewport/simplebuffer.h>

#include "menu/menu.h"

static tView *s_pTestCopperView;
static tVPort *s_pTestCopperVPort;
static tSimpleBufferManager *s_pTestCopperBfr;

tCopBlock *pBar[32];

/**
 * Converts 24-bit HSV to 12-bit RGB
 * This fn is messy copypasta from stackoverflow with hackfixes to make it run
 * on 12-bit.
 */
UWORD colorHSV(UBYTE ubH, UBYTE ubS, UBYTE ubV) {
	UWORD uwRGB;
	UBYTE ubRegion, ubRem, p, q, t;

	if (ubS == 0) {
		ubV >>= 4; // 12-bit fit
		return (ubV << 8) | (ubV << 4) | ubV;
	}

	ubRegion = ubH / 43;
	ubRem = (ubH - (ubRegion * 43)) * 6; 

	p = (ubV * (255 - ubS)) >> 8;
	q = (ubV * (255 - ((ubS * ubRem) >> 8))) >> 8;
	t = (ubV * (255 - ((ubS * (255 - ubRem)) >> 8))) >> 8;

	ubV >>= 4; p >>= 4; q >>= 4; t >>= 4; // 12-bit fit
	switch (ubRegion) {
		case 0:
			return (ubV << 8) | (t << 4) | p;
		case 1:
			return (q << 8) | (ubV << 4) | p;
		case 2:
			return (p << 8) | (ubV << 4) | t;
		case 3:
			return (p << 8) | (q << 4) | ubV;
		case 4:
			return (t << 8) | (p << 4) | ubV;
		default:
			return (ubV << 8) | (p << 4) | q;
	}
}


void gsTestCopperCreate(void) {
	UBYTE i;
	
	// Prepare view & viewport
	s_pTestCopperView = viewCreate(V_GLOBAL_CLUT);
	s_pTestCopperVPort = vPortCreate(s_pTestCopperView, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT, WINDOW_SCREEN_BPP, 0);
	s_pTestCopperBfr = simpleBufferCreate(s_pTestCopperVPort, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT);
	s_pTestCopperVPort->pPalette[0] = 0x000;
	s_pTestCopperVPort->pPalette[1] = 0xAAA;
	s_pTestCopperVPort->pPalette[2] = 0x666;
	
	blitRect(s_pTestCopperBfr->pBuffer, 0,0, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT, 1);
	blitRect(s_pTestCopperBfr->pBuffer, 0,0, 1, 256, 2);
	blitRect(s_pTestCopperBfr->pBuffer, 319,0, 1, 256, 2);
	blitRect(s_pTestCopperBfr->pBuffer, 0,0, 320, 1, 2);
	blitRect(s_pTestCopperBfr->pBuffer, 0,255, 320, 1, 2);
	
	for(i = 0; i != 32; ++i)
		pBar[i] = copBlockCreate(s_pTestCopperView->pCopList, 1, 0, 50+i);
	for(i = 0; i != 16; ++i)
		copMove(s_pTestCopperView->pCopList, pBar[i], &custom.color[1], colorHSV(0,255,i << 3));
	for(i = 16; i != 32; ++i)
		copMove(s_pTestCopperView->pCopList, pBar[i], &custom.color[1], colorHSV(0,255,(31-i) << 3));

	// Display view with its viewports
	viewLoad(s_pTestCopperView);
}

void gsTestCopperLoop(void) {
	static UWORD uwY = 160;
	static BYTE bDir = 1;
	static UBYTE ubHue = 0;
	UBYTE i;
	
	if (keyUse(KEY_ESCAPE)) {
		gameChangeState(gsMenuCreate, gsMenuLoop, gsMenuDestroy);
		return;
	}
	
	if(uwY >= 280)
		bDir = -1;
	if(uwY <= 30)
		bDir = 1;
	
	uwY += 2*bDir;
	
	for(i = 0; i != 32; ++i)
		copWait(s_pTestCopperView->pCopList, pBar[i], 0, uwY+i);
	for(i = 0; i != 16; ++i)
		pBar[i]->pCmds[0].sMove.bfValue = colorHSV(ubHue,255,(i << 4) | i);
	for(i = 16; i != 32; ++i)
		pBar[i]->pCmds[0].sMove.bfValue = colorHSV(ubHue,255,((31-i) << 4) | (31-i));

	++ubHue;
	
	copProcessBlocks();
	WaitTOF();
}

void gsTestCopperDestroy(void) {
	// Destroy buffer, view & viewport
	viewDestroy(s_pTestCopperView);
}