/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "test/copper.h"
#include <ace/utils/extview.h>
#include <ace/managers/copper.h>
#include <ace/managers/blit.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include "game.h"

static tView *s_pTestCopperView;
static tVPort *s_pTestCopperVPort;
static tSimpleBufferManager *s_pTestCopperBfr;

static UBYTE s_isRawMode = 0;
static UWORD s_uwCopRawOffs;

static tCopBlock *s_pBarBlocks[32];

static UWORD s_uwBarY = 160;
static UBYTE s_ubBarHue = 0;

/**
 * Converts 24-bit HSV to 12-bit RGB
 * This fn is messy copypasta from stackoverflow to make it run on 12-bit.
 */
static UWORD colorHSV(UBYTE ubH, UBYTE ubS, UBYTE ubV) {
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

#define TEST_COPPER_COLOR_INSIDE 1
#define TEST_COPPER_COLOR_BORDER 2

void gsTestCopperCreate(void) {
	// Prepare view & viewport
	ULONG ulMode = VIEW_COPLIST_MODE_BLOCK;
	ULONG ulRawSize = 0;
	if(s_isRawMode) {
		ulMode = VIEW_COPLIST_MODE_RAW;
		ulRawSize = (
			simpleBufferGetRawCopperlistInstructionCount(SHOWCASE_BPP) +
			32 * 2 + // 32 bars - each consists of WAIT + MOVE instruction
			1 + // Final WAIT
			1 // Just to be sure
		);
	}

	s_pTestCopperView = viewCreate(0,
		TAG_VIEW_COPLIST_MODE, ulMode,         // <-- This is important in RAW mode
		TAG_VIEW_COPLIST_RAW_COUNT, ulRawSize, // <-- This is important in RAW mode
		TAG_DONE
	);

	s_pTestCopperVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pTestCopperView,
		TAG_VPORT_BPP, SHOWCASE_BPP,
		TAG_DONE
	);

	s_uwCopRawOffs = 0;
	s_pTestCopperBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pTestCopperVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, s_uwCopRawOffs, // <-- Important in RAW mode
		TAG_DONE
	);
	s_uwCopRawOffs += simpleBufferGetRawCopperlistInstructionCount(SHOWCASE_BPP);

	// Some dummy palette for borders etc.
	s_pTestCopperVPort->pPalette[0] = 0x000;
	s_pTestCopperVPort->pPalette[TEST_COPPER_COLOR_INSIDE] = 0xAAA;
	s_pTestCopperVPort->pPalette[TEST_COPPER_COLOR_BORDER] = 0x666;

	// Clear viewport, draw border around it
	UWORD uwMaxX = s_pTestCopperBfr->uBfrBounds.uwX-1;
	UWORD uwMaxY = s_pTestCopperBfr->uBfrBounds.uwY-1;
	blitRect(
		s_pTestCopperBfr->pBack, 0,0,
		s_pTestCopperBfr->uBfrBounds.uwX,
		s_pTestCopperBfr->uBfrBounds.uwY,
		TEST_COPPER_COLOR_INSIDE
	);
	blitLine(
		s_pTestCopperBfr->pBack, 0, 0, uwMaxX, 0,
		TEST_COPPER_COLOR_BORDER, 0xFFFF, 0
	);
	blitLine(
		s_pTestCopperBfr->pBack, 0, uwMaxY, uwMaxX, uwMaxY,
		TEST_COPPER_COLOR_BORDER, 0xFFFF, 0
	);
	blitLine(
		s_pTestCopperBfr->pBack, 0, 0, 0, uwMaxY,
		TEST_COPPER_COLOR_BORDER, 0xFFFF, 0
	);
	blitLine(
		s_pTestCopperBfr->pBack, uwMaxX, 0, uwMaxX, uwMaxY,
		TEST_COPPER_COLOR_BORDER, 0xFFFF, 0
	);

	// Calculate colors for copper bar.
	// I used HSV - it's a nice color model to get different shades of same color.
	// The V component is brightness - make it go 0..255, 255..0
	UWORD pColors[32];
	for(UBYTE i = 0; i < 16; ++i) {
		pColors[i] = colorHSV(s_ubBarHue, 255, i * 17);
	}
	for(UBYTE i = 16; i < 32; ++i) {
		pColors[i] = colorHSV(s_ubBarHue, 255, (31-i) * 17);
	}

	// Create a shaded copperbar. This works by changing color every line.
	// This copperbar is 32px wide with color change in every line, so copper
	// needs to wait for each of 32 lines and change color in each one of them.
	if(s_isRawMode) {
		// This is how you do stuff in copperlist's RAW mode.
		// - Copperlist is double buffered, so you need to regenerate it each frame
		// on back buffer.
		tCopBfr *pCopBfr = s_pTestCopperView->pCopList->pBackBfr;
		// - Go to position directly after simple buffer's commands.
		tCopCmd *pBarCmds = &pCopBfr->pList[s_uwCopRawOffs];
		// - Write WAIT + MOVE commands for each line.
		for(UBYTE i = 0; i < 32; ++i) {
			copSetWait(&pBarCmds[i * 2 + 0].sWait, 0, s_uwBarY + i);
			copSetMove(&pBarCmds[i * 2 + 1].sMove, &g_pCustom->color[1], pColors[i]);
		}
		// Copy the same thing to front buffer, so that copperlist has the same
		// structure on both buffers and we can just update parts we need
		for(UWORD i = 0; i < pCopBfr->uwAllocSize; ++i) {
			s_pTestCopperView->pCopList->pFrontBfr->pList[i].ulCode = (
				pCopBfr->pList[i].ulCode
			);
		}
	}
	else {
		// This is how you do stuff in copperlist's BLOCK mode.
		// - Create a block for each WAIT + N x MOVE instructions,
		// - Set WAIT cmd's x,y in copBlockCreate()
		for(UBYTE i = 0; i < 32; ++i) {
			s_pBarBlocks[i] = copBlockCreate(
				s_pTestCopperView->pCopList, 1, 0, s_uwBarY + i
			);
		}

		// - Use copMove() to append MOVE cmds to each block.
		for(UBYTE i = 0; i < 32; ++i) {
			copMove(
				s_pTestCopperView->pCopList, s_pBarBlocks[i],
				&g_pCustom->color[1], pColors[i]
			);
		}

		// Calling copMove() again on same copBlock would append next MOVE
		// instruction drectly after last one.
	}

	// Display view with its viewports
	viewLoad(s_pTestCopperView);
	systemUnuse();
}

void gsTestCopperLoop(void) {
	static BYTE bDir = 1;

	if (keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}

	if(keyUse(KEY_M)) {
		// Change modes and restart
		s_isRawMode = !s_isRawMode;
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_COPPER]);
		return;
	}

	// Regenerate colors for different hue
	++s_ubBarHue;
	UWORD pColors[32];
	for(UBYTE i = 0; i < 16; ++i) {
		pColors[i] = colorHSV(s_ubBarHue, 255, i * 17);
	}
	for(UBYTE i = 16; i < 32; ++i) {
		pColors[i] = colorHSV(s_ubBarHue, 255, (31-i) * 17);
	}

	// We want to move our copperbar up or down.
	// In RAW mode, you need to take care of doing double WAIT for going past y=255,
	// In BLOCK mode this is done automatically.
	UWORD uwMaxY = s_isRawMode ? 220 : 280;
	if(s_uwBarY >= uwMaxY) {
		bDir = -1;
		s_uwBarY = uwMaxY;
	}
	if(s_uwBarY <= 30) {
		bDir = 1;
		s_uwBarY = 30;
	}
	s_uwBarY += 2 * bDir;

	if(s_isRawMode) {
		tCopBfr *pCopBfr = s_pTestCopperView->pCopList->pBackBfr;
		tCopCmd *pBarCmds = &pCopBfr->pList[s_uwCopRawOffs];
		for(UBYTE i = 0; i < 32; ++i) {
			// Replace WAIT cmd's Y value.
			pBarCmds[i * 2 + 0].sWait.bfWaitY = s_uwBarY + i;
			// Replace color value
			pBarCmds[i * 2 + 1].sMove.bfValue = pColors[i];
		}
	}
	else {
		for(UBYTE i = 0; i < 32; ++i) {
			// Replace WAIT cmd's Y value.
			copBlockWait(s_pTestCopperView->pCopList, s_pBarBlocks[i], 0, s_uwBarY+i);
			// Replace color value
			s_pBarBlocks[i]->pCmds[0].sMove.bfValue = pColors[i];
		}
	}

	// For debugging, copDumpBfr() / copDumpBlocks() are your best friends.
	// Bind them to some key press and inspect copperlist often.
	// When using RAW mode, you really need to know what you're doing !
	if(keyUse(KEY_C)) {
		if(s_isRawMode) {
			logWrite("Front:\n");
			copDumpBfr(s_pTestCopperView->pCopList->pFrontBfr);
			logWrite("Back:\n");
			copDumpBfr(s_pTestCopperView->pCopList->pBackBfr);
		}
		else {
			copDumpBlocks();
		}
	}

	// This regenerates copperlist parts for simpleBuffer in both modes,
	// but we don't need it since we're not using scrolling.
	// vPortProcessManagers();

	// Call copProcessBlocks even if you're in RAW mode - swaps copper buffers.
	copProcessBlocks();
	vPortWaitForEnd(s_pTestCopperVPort);
}

void gsTestCopperDestroy(void) {
	systemUse();
	// Destroy buffer, view & viewport
	viewDestroy(s_pTestCopperView);
}
