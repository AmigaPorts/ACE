/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "test/scroll_tile_buffer.h"
#include <ace/managers/blit.h>
#include <ace/managers/bob.h>
#include <ace/managers/copper.h>
#include <ace/managers/key.h>
#include <ace/managers/system.h>
#include <ace/managers/timer.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/viewport/tilebuffer.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/font.h>
#include <stdio.h>

#include "game.h"

#define TILE_SIZE 16
#define TILE_SHIFT 4
#define TILE_COUNT 8
#define MAP_TILES_X 64
#define MAP_TILES_Y 64
#define HUD_HEIGHT 32
#define BOB_COUNT 5

typedef enum tMovePattern {
	MOVE_RIGHT,
	MOVE_DOWN,
	MOVE_LEFT,
	MOVE_UP,
	MOVE_LEFT_PULSE_UP,
	MOVE_UP_PULSE_RIGHT,
	MOVE_RIGHT_PULSE_DOWN,
	MOVE_DOWN_PULSE_LEFT,
	MOVE_SPIN_IN_PLACE_RIGHT,
	MOVE_SPIN_IN_PLACE_LEFT,
	MOVE_FAST_SPIN_RIGHT,
	MOVE_FAST_SPIN_DOWN,
	MOVE_FAST_SPIN_LEFT,
	MOVE_FAST_SPIN_UP,
	MOVE_FAST_SPIN_DOWN_RIGHT,
	MOVE_FAST_SPIN_DOWN_LEFT,
	MOVE_FAST_SPIN_UP_LEFT,
	MOVE_FAST_SPIN_UP_RIGHT,
	MOVE_COUNT
} tMovePattern;

#define SIM_PLAYER_SPEED_FP 512
#define SIM_PLAYER_FAST_SPEED_FP 768
#define SIM_AIM_RADIUS 72
#define SIM_AIM_SPEED 1

static const WORD s_pSinTable[64] = {
	0, 25, 50, 74, 98, 120, 142, 162,
	180, 197, 212, 225, 236, 244, 250, 254,
	256, 254, 250, 244, 236, 225, 212, 197,
	180, 162, 142, 120, 98, 74, 50, 25,
	0, -25, -50, -74, -98, -120, -142, -162,
	-180, -197, -212, -225, -236, -244, -250, -254,
	-256, -254, -250, -244, -236, -225, -212, -197,
	-180, -162, -142, -120, -98, -74, -50, -25
};

typedef struct tDiagBob {
	tBob sBob;
	tBitMap *pFrame;
	tBitMap *pMask;
	WORD wScreenX;
	WORD wScreenY;
	WORD wDx;
	WORD wDy;
	UWORD uwSize;
} tDiagBob;

static tView *s_pView;
static tVPort *s_pHudVPort;
static tVPort *s_pTileVPort;
static tSimpleBufferManager *s_pHudBuffer;
static tTileBufferManager *s_pTileBuffer;
static tBitMap *s_pTileset;
static tFont *s_pFont;
static tTextBitMap *s_pTextBitMap;
static UBYTE s_ubBpp = 5;
static UBYTE s_ubFmode = 0;
static UBYTE s_isBobsEnabled = 1;
static UBYTE s_isDblBuf = 1;
static UBYTE s_isManualMove = 0;
static ULONG s_ulMoveStart;
static LONG s_lSimPlayerX;
static LONG s_lSimPlayerY;
static UBYTE s_ubSimAimAngle;
static tDiagBob s_pBobs[BOB_COUNT];

static void initAutoMoveState(void) {
	s_lSimPlayerX = (LONG)s_pTileBuffer->pCamera->uPos.uwX << 8;
	s_lSimPlayerY = (LONG)s_pTileBuffer->pCamera->uPos.uwY << 8;
	s_ubSimAimAngle = 0;
}

static UWORD makePaletteColor(UWORD uwIndex, UWORD uwColorCount) {
	UWORD uwLastColor = uwColorCount - 1;
	UWORD uwSplit = uwLastColor / 2;

	if(uwIndex == 0) {
		return 0x000;
	}
	if(uwIndex == uwLastColor) {
		return 0xFFF;
	}
	if(uwIndex <= uwSplit) {
		UBYTE ubStep = uwSplit > 1 ? (15 * (uwIndex - 1)) / (uwSplit - 1) : 8;
		UBYTE ubR = ubStep / 4;
		UBYTE ubG = 2 + ubStep / 3;
		UBYTE ubB = 7 + ubStep / 2;

		return (ubR << 8) | (ubG << 4) | ubB;
	}

	UWORD uwBobSpan = uwLastColor - uwSplit - 1;
	UBYTE ubStep = uwBobSpan ? (15 * (uwIndex - uwSplit - 1)) / uwBobSpan : 8;
	UBYTE ubR = 8 + ubStep / 2;
	UBYTE ubG = ubStep / 5;
	UBYTE ubB = 3 + ubStep / 3;

	return (ubR << 8) | (ubG << 4) | ubB;
}

#ifdef ACE_USE_AGA_FEATURES
static ULONG makePaletteColorAga(UWORD uwIndex, UWORD uwColorCount) {
	UWORD uwLastColor = uwColorCount - 1;
	UWORD uwSplit = uwLastColor / 2;

	if(uwIndex == 0) {
		return 0x000000;
	}
	if(uwIndex == uwLastColor) {
		return 0xFFFFFF;
	}
	if(uwIndex <= uwSplit) {
		UBYTE ubStep = uwSplit > 1 ? (255 * (uwIndex - 1)) / (uwSplit - 1) : 128;
		UBYTE ubR = ubStep / 4;
		UBYTE ubG = 32 + ubStep / 3;
		UBYTE ubB = 112 + ubStep / 2;

		return ((ULONG)ubR << 16) | ((ULONG)ubG << 8) | ubB;
	}

	UWORD uwBobSpan = uwLastColor - uwSplit - 1;
	UBYTE ubStep = uwBobSpan ? (255 * (uwIndex - uwSplit - 1)) / uwBobSpan : 128;
	UBYTE ubR = 128 + ubStep / 2;
	UBYTE ubG = ubStep / 5;
	UBYTE ubB = 48 + ubStep / 3;

	return ((ULONG)ubR << 16) | ((ULONG)ubG << 8) | ubB;
}
#endif

static void setupPalette(void) {
	UWORD uwColorCount = 1 << s_ubBpp;

#ifdef ACE_USE_AGA_FEATURES
	if(s_ubBpp > 5) {
		ULONG *pPalette = (ULONG *)s_pHudVPort->pPalette;

		pPalette[0] = 0x000000;
		for(UWORD i = 1; i < uwColorCount; ++i) {
			pPalette[i] = makePaletteColorAga(i, uwColorCount);
		}
		pPalette[uwColorCount - 1] = 0xFFFFFF;
		return;
	}
#endif

	s_pHudVPort->pPalette[0] = 0x000;
	for(UWORD i = 1; i < uwColorCount; ++i) {
		s_pHudVPort->pPalette[i] = makePaletteColor(i, uwColorCount);
	}
	s_pHudVPort->pPalette[uwColorCount - 1] = 0xFFF;
}

static void drawHeaderLine(UWORD uwY, const char *szText, UBYTE ubTextColor) {
	fontDrawStr(
		s_pFont, s_pHudBuffer->pBack, 4, uwY,
		szText, ubTextColor, FONT_LEFT | FONT_TOP, s_pTextBitMap
	);
}

static void drawHeader(void) {
	char szLine[96];
	UBYTE ubTextColor = (1 << s_ubBpp) - 1;

	blitRect(
		s_pHudBuffer->pBack, 0, 0,
		s_pHudBuffer->uBfrBounds.uwX, s_pHudBuffer->uBfrBounds.uwY, 0
	);
	sprintf(
		szLine, "TILEBUFFER %s BPP:%u FMODE:%u BOBS:%s DBLBUF:%s",
		s_isManualMove ? "MANUAL" : "AUTO",
		s_ubBpp, s_ubFmode,
		s_isBobsEnabled ? "ON" : "OFF",
		s_isDblBuf ? "ON" : "OFF"
	);
	drawHeaderLine(4, szLine, ubTextColor);
	drawHeaderLine(14, "SPACE:auto/manual WSAD:move ESC:menu", ubTextColor);
#ifdef ACE_USE_AGA_FEATURES
	if(s_ubBpp > 5) {
		drawHeaderLine(24, "2-8:bpp Z/X/C/V:fmode 0/1/2/3 B:bobs D:dblbuf", ubTextColor);
	}
	else {
		drawHeaderLine(24, "2-8:bpp B:bobs D:dblbuf", ubTextColor);
	}
#else
	drawHeaderLine(24, "2-5:bpp B:bobs D:dblbuf", ubTextColor);
#endif
}

static void drawTile(UWORD uwTile, UBYTE ubBaseColor) {
	UWORD uwY = uwTile * TILE_SIZE;
	UBYTE ubColorA, ubColorB, ubColorC;

	if(s_ubBpp == 2) {
		ubColorA = 0;
		ubColorB = 1;
		ubColorC = 1;
	}
	else {
		UBYTE ubBlueColorCount = ((1 << s_ubBpp) - 1) / 2;
		ubColorA = 1 + (ubBaseColor % ubBlueColorCount);
		ubColorB = 1 + ((ubBaseColor + 2) % ubBlueColorCount);
		ubColorC = 1 + ((ubBaseColor + 4) % ubBlueColorCount);
	}

	blitRect(s_pTileset, 0, uwY, TILE_SIZE, TILE_SIZE, ubColorA);
	blitRect(s_pTileset, 0, uwY, TILE_SIZE, 1, ubColorC);
	blitRect(s_pTileset, 0, uwY + TILE_SIZE - 1, TILE_SIZE, 1, ubColorC);
	blitRect(s_pTileset, 0, uwY, 1, TILE_SIZE, ubColorC);
	blitRect(s_pTileset, TILE_SIZE - 1, uwY, 1, TILE_SIZE, ubColorC);

	if(uwTile & 1) {
		blitLine(s_pTileset, 2, uwY + 2, 13, uwY + 13, ubColorB, 0xFFFF, 0);
		blitLine(s_pTileset, 13, uwY + 2, 2, uwY + 13, ubColorB, 0xFFFF, 0);
	}
	else {
		blitRect(s_pTileset, 4, uwY + 4, 8, 8, ubColorB);
	}
}

static void createTileset(void) {
	s_pTileset = bitmapCreate(
		TILE_SIZE, TILE_SIZE * TILE_COUNT, s_ubBpp,
		BMF_CLEAR | BMF_INTERLEAVED
	);

	for(UWORD i = 0; i < TILE_COUNT; ++i) {
		drawTile(i, i + 1);
	}
}

static void fillTileMap(void) {
	for(UWORD x = 0; x < MAP_TILES_X; ++x) {
		for(UWORD y = 0; y < MAP_TILES_Y; ++y) {
			s_pTileBuffer->pTileData[x][y] = (x + y * 3 + ((x ^ y) & 3)) % TILE_COUNT;
		}
	}
}

static UBYTE getBobColor(UBYTE ubSeed) {
	UBYTE ubMaxColor = (1 << s_ubBpp) - 1;
	UBYTE ubFirstBobColor = ubMaxColor / 2 + 1;
	UBYTE ubBobColorCount = ubMaxColor - ubFirstBobColor;

	if(s_ubBpp == 2) {
		return ubSeed % 2 ? 3 : 2;
	}

	if(!ubBobColorCount) {
		return ubFirstBobColor;
	}
	return ubFirstBobColor + (ubSeed % ubBobColorCount);
}

static void drawBobFrame(tDiagBob *pBob, UBYTE ubSeed) {
	UWORD uwSize = pBob->uwSize;
	UWORD uwSourceHeight = uwSize + 1;
	UBYTE ubColorA = getBobColor(ubSeed);
	UBYTE ubColorB = getBobColor(ubSeed + 3);
	UBYTE ubColorC = getBobColor(ubSeed + 7);
	UBYTE ubMaskColor = (1 << s_ubBpp) - 1;

	blitRect(pBob->pFrame, 0, 0, uwSize, uwSourceHeight, 0);
	blitRect(pBob->pMask, 0, 0, uwSize, uwSourceHeight, ubMaskColor);
	blitRect(pBob->pFrame, 0, 0, uwSize, uwSize, ubColorA);
	blitRect(pBob->pFrame, 0, 0, uwSize, 1, ubColorC);
	blitRect(pBob->pFrame, 0, uwSize - 1, uwSize, 1, ubColorC);
	blitRect(pBob->pFrame, 0, 0, 1, uwSize, ubColorC);
	blitRect(pBob->pFrame, uwSize - 1, 0, 1, uwSize, ubColorC);

	if(uwSize == 16) {
		blitLine(pBob->pFrame, 3, 3, 12, 12, ubColorB, 0xFFFF, 0);
		blitLine(pBob->pFrame, 12, 3, 3, 12, ubColorB, 0xFFFF, 0);
	}
	else {
		blitRect(pBob->pFrame, 8, 8, 16, 16, ubColorB);
		blitLine(pBob->pFrame, 4, 4, 27, 27, ubColorC, 0xFFFF, 0);
		blitLine(pBob->pFrame, 27, 4, 4, 27, ubColorC, 0xFFFF, 0);
	}
}

static void initDiagBob(
	UBYTE ubIndex, UWORD uwSize, WORD wX, WORD wY, WORD wDx, WORD wDy
) {
	tDiagBob *pBob = &s_pBobs[ubIndex];

	pBob->uwSize = uwSize;
	pBob->wScreenX = wX;
	pBob->wScreenY = wY;
	pBob->wDx = wDx;
	pBob->wDy = wDy;
	pBob->pFrame = bitmapCreate(uwSize, uwSize + 1, s_ubBpp, BMF_CLEAR | BMF_INTERLEAVED);
	pBob->pMask = bitmapCreate(uwSize, uwSize + 1, s_ubBpp, BMF_CLEAR | BMF_INTERLEAVED);
	drawBobFrame(pBob, ubIndex * 5 + 1);

	bobInit(
		&pBob->sBob, uwSize, uwSize, 1,
		bobCalcFrameAddress(pBob->pFrame, 0),
		bobCalcFrameAddress(pBob->pMask, 0),
		wX, wY
	);
}

static void createBobs(void) {
	bobManagerCreate(
		s_pTileBuffer->pScroll->pFront,
		s_pTileBuffer->pScroll->pBack,
		s_pTileBuffer->pScroll->uwBmAvailHeight
	);
	initDiagBob(0, 16, 24, 24, 1, 0);
	initDiagBob(1, 16, 96, 48, 0, 1);
	initDiagBob(2, 16, 168, 72, 1, 1);
	initDiagBob(3, 16, 248, 104, -1, 1);
	initDiagBob(4, 32, 128, 132, 1, -1);
	bobReallocateBuffers();
	bobDiscardUndraw();
}

static void destroyBobs(void) {
	bobManagerDestroy();
	for(UBYTE i = 0; i < BOB_COUNT; ++i) {
		bitmapDestroy(s_pBobs[i].pFrame);
		bitmapDestroy(s_pBobs[i].pMask);
		s_pBobs[i].pFrame = 0;
		s_pBobs[i].pMask = 0;
	}
}

static void updateBobPositions(void) {
	UWORD uwMaxX = s_pTileVPort->uwWidth;
	UWORD uwMaxY = s_pTileVPort->uwHeight;
	UWORD uwCameraX = s_pTileBuffer->pCamera->uPos.uwX;
	UWORD uwCameraY = s_pTileBuffer->pCamera->uPos.uwY;

	for(UBYTE i = 0; i < BOB_COUNT; ++i) {
		tDiagBob *pBob = &s_pBobs[i];
		WORD wMaxX = uwMaxX - pBob->uwSize;
		WORD wMaxY = uwMaxY - pBob->uwSize;

		pBob->wScreenX += pBob->wDx;
		pBob->wScreenY += pBob->wDy;
		if(pBob->wScreenX <= 0 || pBob->wScreenX >= wMaxX) {
			pBob->wDx = -pBob->wDx;
			pBob->wScreenX += pBob->wDx;
		}
		if(pBob->wScreenY <= 0 || pBob->wScreenY >= wMaxY) {
			pBob->wDy = -pBob->wDy;
			pBob->wScreenY += pBob->wDy;
		}

		pBob->sBob.sPos.uwX = uwCameraX + pBob->wScreenX;
		pBob->sBob.sPos.uwY = uwCameraY + pBob->wScreenY;
	}
}

static void processScrollTileBuffer(void) {
	bobSetCurrentBuffer(s_pTileBuffer->pScroll->pBack);
	bobBegin(s_pTileBuffer->pScroll->pBack);
	tileBufferProcess(s_pTileBuffer);
	if(s_isBobsEnabled) {
		updateBobPositions();
		for(UBYTE i = 0; i < BOB_COUNT; ++i) {
			bobPush(&s_pBobs[i].sBob);
		}
	}
	bobPushingDone();
	bobEnd();
	scrollBufferProcess(s_pTileBuffer->pScroll);
	cameraProcess(s_pTileBuffer->pCamera);
}

static void createView(void) {
#ifdef ACE_USE_AGA_FEATURES
	if(s_ubBpp > 5) {
		s_pView = viewCreate(0,
			TAG_VIEW_GLOBAL_PALETTE, 1,
			TAG_VIEW_GLOBAL_BPP, 1,
			TAG_VIEW_USES_AGA, 1,
		TAG_END);
		s_pHudVPort = vPortCreate(0,
			TAG_VPORT_VIEW, s_pView,
			TAG_VPORT_HEIGHT, HUD_HEIGHT,
			TAG_VPORT_BPP, s_ubBpp,
			TAG_VPORT_USES_AGA, 1,
			TAG_VPORT_FMODE, s_ubFmode,
		TAG_END);
		s_pTileVPort = vPortCreate(0,
			TAG_VPORT_VIEW, s_pView,
			TAG_VPORT_BPP, s_ubBpp,
			TAG_VPORT_USES_AGA, 1,
			TAG_VPORT_FMODE, s_ubFmode,
		TAG_END);
	}
	else
#endif
	{
		s_pView = viewCreate(0,
			TAG_VIEW_GLOBAL_PALETTE, 1,
			TAG_VIEW_GLOBAL_BPP, 1,
		TAG_END);
		s_pHudVPort = vPortCreate(0,
			TAG_VPORT_VIEW, s_pView,
			TAG_VPORT_HEIGHT, HUD_HEIGHT,
			TAG_VPORT_BPP, s_ubBpp,
		TAG_END);
		s_pTileVPort = vPortCreate(0,
			TAG_VPORT_VIEW, s_pView,
			TAG_VPORT_BPP, s_ubBpp,
		TAG_END);
	}

	s_pHudBuffer = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pHudVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
	TAG_END);

	createTileset();
	s_pTileBuffer = tileBufferCreate(0,
		TAG_TILEBUFFER_VPORT, s_pTileVPort,
		TAG_TILEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_TILEBUFFER_BOUND_TILE_X, MAP_TILES_X,
		TAG_TILEBUFFER_BOUND_TILE_Y, MAP_TILES_Y,
		TAG_TILEBUFFER_TILE_SHIFT, TILE_SHIFT,
		TAG_TILEBUFFER_TILESET, s_pTileset,
		TAG_TILEBUFFER_IS_DBLBUF, s_isDblBuf,
		TAG_TILEBUFFER_REDRAW_QUEUE_LENGTH, 32,
		TAG_TILEBUFFER_MAX_TILESET_SIZE, TILE_COUNT,
	TAG_END);

	setupPalette();
	drawHeader();
	fillTileMap();
	cameraSetCoord(
		s_pTileBuffer->pCamera,
		s_pTileBuffer->pCamera->uMaxPos.uwX / 2,
		s_pTileBuffer->pCamera->uMaxPos.uwY / 2
	);
	tileBufferRedrawAll(s_pTileBuffer);
	createBobs();

	viewLoad(s_pView);
	s_ulMoveStart = timerGet();
	initAutoMoveState();
}

static void destroyView(void) {
	viewLoad(0);
	destroyBobs();
	viewDestroy(s_pView);
	bitmapDestroy(s_pTileset);
	s_pView = 0;
	s_pHudVPort = 0;
	s_pTileVPort = 0;
	s_pHudBuffer = 0;
	s_pTileBuffer = 0;
	s_pTileset = 0;
}

static void recreateView(void) {
	destroyView();
	createView();
}

static UBYTE getMaxBpp(void) {
#ifdef ACE_USE_AGA_FEATURES
	return 8;
#else
	return 5;
#endif
}

static void setBpp(UBYTE ubBpp) {
	if(ubBpp < 2 || ubBpp > getMaxBpp() || ubBpp == s_ubBpp) {
		return;
	}
	s_ubBpp = ubBpp;
#ifndef ACE_USE_AGA_FEATURES
	s_ubFmode = 0;
#endif
	recreateView();
}

static void setFmode(UBYTE ubFmode) {
#ifdef ACE_USE_AGA_FEATURES
	if(s_ubBpp <= 5 || s_ubFmode == ubFmode) {
		return;
	}
	s_ubFmode = ubFmode;
	recreateView();
#else
	(void)ubFmode;
#endif
}

static void handleConfigKeys(void) {
	if(keyUse(KEY_2)) {
		setBpp(2);
	}
	else if(keyUse(KEY_3)) {
		setBpp(3);
	}
	else if(keyUse(KEY_4)) {
		setBpp(4);
	}
	else if(keyUse(KEY_5)) {
		setBpp(5);
	}
#ifdef ACE_USE_AGA_FEATURES
	else if(keyUse(KEY_6)) {
		setBpp(6);
	}
	else if(keyUse(KEY_7)) {
		setBpp(7);
	}
	else if(keyUse(KEY_8)) {
		setBpp(8);
	}
#endif

#ifdef ACE_USE_AGA_FEATURES
	if(keyUse(KEY_Z)) {
		setFmode(0);
	}
	if(keyUse(KEY_X)) {
		setFmode(1);
	}
	if(keyUse(KEY_C)) {
		setFmode(2);
	}
	if(keyUse(KEY_V)) {
		setFmode(3);
	}
#endif
	if(!s_isManualMove && keyUse(KEY_D)) {
		s_isDblBuf = !s_isDblBuf;
		recreateView();
	}
	if(keyUse(KEY_B)) {
		s_isBobsEnabled = !s_isBobsEnabled;
		drawHeader();
	}
	if(keyUse(KEY_SPACE)) {
		s_isManualMove = !s_isManualMove;
		s_ulMoveStart = timerGet();
		initAutoMoveState();
		drawHeader();
	}
}

static void getAutoMove(WORD *pDx, WORD *pDy) {
	ULONG ulElapsed = timerGetDelta(s_ulMoveStart, timerGet());
	ULONG ulFreq = systemGetVerticalBlankFrequency();
	ULONG ulStep = (ulElapsed / (ulFreq * 2)) % MOVE_COUNT;
	ULONG ulPulseDiv = ulFreq / 10;
	UBYTE isPulseOn;

	if(!ulPulseDiv) {
		ulPulseDiv = 1;
	}
	isPulseOn = ((ulElapsed / ulPulseDiv) & 1) == 0;
	*pDx = 0;
	*pDy = 0;

	if (ulStep < MOVE_SPIN_IN_PLACE_RIGHT) {
		switch((tMovePattern)ulStep) {
			case MOVE_RIGHT: *pDx = 2; break;
			case MOVE_DOWN: *pDy = 2; break;
			case MOVE_LEFT: *pDx = -2; break;
			case MOVE_UP: *pDy = -2; break;
			case MOVE_LEFT_PULSE_UP:
				*pDx = -2;
				*pDy = isPulseOn ? -2 : 0;
				break;
			case MOVE_UP_PULSE_RIGHT:
				*pDy = -2;
				*pDx = isPulseOn ? 2 : 0;
				break;
			case MOVE_RIGHT_PULSE_DOWN:
				*pDx = 2;
				*pDy = isPulseOn ? 2 : 0;
				break;
			case MOVE_DOWN_PULSE_LEFT:
				*pDy = 2;
				*pDx = isPulseOn ? -2 : 0;
				break;
			default: break;
		}
		s_lSimPlayerX = (LONG)s_pTileBuffer->pCamera->uPos.uwX << 8;
		s_lSimPlayerY = (LONG)s_pTileBuffer->pCamera->uPos.uwY << 8;
		s_ubSimAimAngle = 0;
	} else {
		if (ulStep == MOVE_SPIN_IN_PLACE_LEFT) {
			s_ubSimAimAngle = (s_ubSimAimAngle - SIM_AIM_SPEED) & 63;
		} else {
			s_ubSimAimAngle = (s_ubSimAimAngle + SIM_AIM_SPEED) & 63;
		}
		
		LONG lDx = 0, lDy = 0;
		switch((tMovePattern)ulStep) {
			case MOVE_SPIN_IN_PLACE_RIGHT:
			case MOVE_SPIN_IN_PLACE_LEFT:
				lDx = 0; lDy = 0; break;
			case MOVE_FAST_SPIN_RIGHT: lDx = SIM_PLAYER_FAST_SPEED_FP; break;
			case MOVE_FAST_SPIN_DOWN: lDy = SIM_PLAYER_FAST_SPEED_FP; break;
			case MOVE_FAST_SPIN_LEFT: lDx = -SIM_PLAYER_FAST_SPEED_FP; break;
			case MOVE_FAST_SPIN_UP: lDy = -SIM_PLAYER_FAST_SPEED_FP; break;
			case MOVE_FAST_SPIN_DOWN_RIGHT: lDx = SIM_PLAYER_FAST_SPEED_FP*7/10; lDy = SIM_PLAYER_FAST_SPEED_FP*7/10; break;
			case MOVE_FAST_SPIN_DOWN_LEFT: lDx = -SIM_PLAYER_FAST_SPEED_FP*7/10; lDy = SIM_PLAYER_FAST_SPEED_FP*7/10; break;
			case MOVE_FAST_SPIN_UP_LEFT: lDx = -SIM_PLAYER_FAST_SPEED_FP*7/10; lDy = -SIM_PLAYER_FAST_SPEED_FP*7/10; break;
			case MOVE_FAST_SPIN_UP_RIGHT: lDx = SIM_PLAYER_FAST_SPEED_FP*7/10; lDy = -SIM_PLAYER_FAST_SPEED_FP*7/10; break;
			default: break;
		}
		s_lSimPlayerX += lDx;
		s_lSimPlayerY += lDy;

		WORD wAimX = (s_pSinTable[(s_ubSimAimAngle + 16) & 63] * SIM_AIM_RADIUS) >> 8;
		WORD wAimY = (s_pSinTable[s_ubSimAimAngle] * SIM_AIM_RADIUS) >> 8;

		WORD wTargetCamX = (s_lSimPlayerX >> 8) + wAimX / 2;
		WORD wTargetCamY = (s_lSimPlayerY >> 8) + wAimY / 2;

		WORD wCamX = s_pTileBuffer->pCamera->uPos.uwX;
		WORD wCamY = s_pTileBuffer->pCamera->uPos.uwY;

		WORD wDeltaX = wTargetCamX - wCamX;
		WORD wDeltaY = wTargetCamY - wCamY;

		if (wDeltaX > 4) wDeltaX = 4;
		else if (wDeltaX < -4) wDeltaX = -4;
		if (wDeltaY > 4) wDeltaY = 4;
		else if (wDeltaY < -4) wDeltaY = -4;

		*pDx = wDeltaX;
		*pDy = wDeltaY;
	}
}

static void getManualMove(WORD *pDx, WORD *pDy) {
	*pDx = 0;
	*pDy = 0;

	if(keyCheck(KEY_A)) {
		*pDx = -2;
	}
	else if(keyCheck(KEY_D)) {
		*pDx = 2;
	}
	if(keyCheck(KEY_W)) {
		*pDy = -2;
	}
	else if(keyCheck(KEY_S)) {
		*pDy = 2;
	}
}

void gsTestDiagScrollTileBufferCreate(void) {
	if(s_ubBpp > getMaxBpp()) {
		s_ubBpp = getMaxBpp();
	}
	s_pFont = fontCreateFromPath("data/silkscreen.fnt");
	s_pTextBitMap = fontCreateTextBitMap(336, s_pFont->uwHeight);
	createView();
}

void gsTestDiagScrollTileBufferLoop(void) {
	WORD wDx;
	WORD wDy;

	if(keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}

	handleConfigKeys();
	if(s_isManualMove) {
		getManualMove(&wDx, &wDy);
	}
	else {
		getAutoMove(&wDx, &wDy);
	}
	cameraMoveBy(s_pTileBuffer->pCamera, wDx, wDy);
	processScrollTileBuffer();
	copProcessBlocks();
	vPortWaitForEnd(s_pTileVPort);
}

void gsTestDiagScrollTileBufferDestroy(void) {
	destroyView();
	fontDestroyTextBitMap(s_pTextBitMap);
	fontDestroy(s_pFont);
}
