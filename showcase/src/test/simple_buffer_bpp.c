#include "test/simple_buffer_bpp.h"
#include <ace/managers/blit.h>
#include <ace/managers/game.h>
#include <ace/managers/key.h>
#include <ace/managers/system.h>
#include <ace/managers/timer.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <stdio.h>

#include "game.h"
#include "diagnostics.h"

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBuffer;
static ULONG s_ulAutoAdvanceStart;
static UBYTE s_isAutoAdvance = 0;

static UWORD makePaletteColor(UWORD uwIndex, UWORD uwColorCount) {
	UBYTE ubStep = uwColorCount > 1 ? (15 * uwIndex) / (uwColorCount - 1) : 0;
	UBYTE ubR = ubStep;
	UBYTE ubG = (uwIndex * 5) & 0x0F;
	UBYTE ubB = 15 - ubStep;

	return (ubR << 8) | (ubG << 4) | ubB;
}

#ifdef ACE_USE_AGA_FEATURES
static ULONG makePaletteColorAga(UWORD uwIndex, UWORD uwColorCount) {
	UBYTE ubStep = uwColorCount > 1 ? (255 * uwIndex) / (uwColorCount - 1) : 0;
	UBYTE ubR = ubStep;
	UBYTE ubG = (uwIndex * 37) & 0xFF;
	UBYTE ubB = 255 - ubStep;

	return ((ULONG)ubR << 16) | ((ULONG)ubG << 8) | ubB;
}
#endif

static void setupPalette(UBYTE ubBpp) {
	UWORD uwColorCount = diagnosticsIsCurrentEhb() ? 32 : (1 << ubBpp);

#ifdef ACE_USE_AGA_FEATURES
	if(diagnosticsIsCurrentAga()) {
		ULONG *pPalette = (ULONG *)s_pVPort->pPalette;

		pPalette[0] = 0x000000;
		for(UWORD i = 1; i < uwColorCount; ++i) {
			pPalette[i] = makePaletteColorAga(i, uwColorCount);
		}
		pPalette[uwColorCount - 1] = 0xFFFFFF;
		return;
	}
#endif

	s_pVPort->pPalette[0] = 0x000;
	for(UWORD i = 1; i < uwColorCount; ++i) {
		s_pVPort->pPalette[i] = makePaletteColor(i, uwColorCount);
	}
	s_pVPort->pPalette[uwColorCount - 1] = 0xFFF;
}

static void drawPattern(UBYTE ubBpp) {
	UWORD uwColorCount = 1 << ubBpp;
	UWORD uwWidth = s_pBuffer->uBfrBounds.uwX;
	UWORD uwHeight = s_pBuffer->uBfrBounds.uwY;
	UWORD uwStripeHeight = 12;
	UWORD uwBlockSize = 24;
	UBYTE ubBright = diagnosticsIsCurrentEhb() ? 31 : uwColorCount - 1;

	blitRect(s_pBuffer->pBack, 0, 0, uwWidth, uwHeight, 0);

	for(UWORD y = 0; y < uwHeight; y += uwStripeHeight) {
		UBYTE ubColor = 1 + ((y / uwStripeHeight) % (uwColorCount - 1));
		blitRect(s_pBuffer->pBack, 0, y, uwWidth, uwStripeHeight, ubColor);
	}

	for(UWORD y = 64; y < uwHeight; y += uwBlockSize) {
		for(UWORD x = 0; x < uwWidth; x += uwBlockSize) {
			UBYTE ubColor = (x / uwBlockSize + y / uwBlockSize) % uwColorCount;
			blitRect(s_pBuffer->pBack, x, y, uwBlockSize / 2, uwBlockSize / 2, ubColor);
		}
	}

	for(UWORD x = 0; x < uwWidth; x += 32) {
		blitRect(s_pBuffer->pBack, x, 0, 1, uwHeight, ubBright);
	}
	for(UWORD y = 0; y < uwHeight; y += 32) {
		blitRect(s_pBuffer->pBack, 0, y, uwWidth, 1, ubBright);
	}

	blitRect(s_pBuffer->pBack, 0, 0, uwWidth, 1, ubBright);
	blitRect(s_pBuffer->pBack, 0, uwHeight - 1, uwWidth, 1, ubBright);
	blitRect(s_pBuffer->pBack, 0, 0, 1, uwHeight, ubBright);
	blitRect(s_pBuffer->pBack, uwWidth - 1, 0, 1, uwHeight, ubBright);
}

static void drawPaletteSwatches(UBYTE ubBpp) {
	UWORD uwColorCount = 1 << ubBpp;
	UWORD uwWidth = s_pBuffer->uBfrBounds.uwX;
	UWORD uwHeight = s_pBuffer->uBfrBounds.uwY;
	UWORD uwSquare = 6;
	UWORD uwStep = 8;
	UWORD uwMaxCols = (uwWidth - 8) / uwStep;
	UWORD uwCols = uwColorCount < uwMaxCols ? uwColorCount : uwMaxCols;
	UWORD uwRows = (uwColorCount + uwCols - 1) / uwCols;
	UWORD uwStartX = 4;
	UWORD uwStartY = uwHeight - 4 - uwRows * uwStep;
	UWORD uwBackWidth = uwCols * uwStep + 4;
	UWORD uwBackHeight = uwRows * uwStep + 4;

	blitRect(s_pBuffer->pBack, 2, uwStartY - 2, uwBackWidth, uwBackHeight, 0);

	for(UWORD i = 0; i < uwColorCount; ++i) {
		UWORD uwX = uwStartX + (i % uwCols) * uwStep;
		UWORD uwY = uwStartY + (i / uwCols) * uwStep;

		blitRect(s_pBuffer->pBack, uwX, uwY, uwSquare, uwSquare, i);
	}
}

static void drawHeaderLine(UWORD uwY, const char *szText, UBYTE ubTextColor) {
	blitRect(s_pBuffer->pBack, 0, uwY, s_pBuffer->uBfrBounds.uwX, 9, 0);
	fontDrawStr(
		diagnosticsGetFont(), s_pBuffer->pBack, 4, uwY + 1,
		szText, ubTextColor, FONT_LEFT | FONT_TOP, diagnosticsGetTextBitMap()
	);
}

static void drawHeader(UBYTE ubBpp) {
	char szTitle[64];
	char szControls[96];
	const char *szBppRange;
	const char *szEhbControl = (ubBpp == 5 || diagnosticsIsCurrentEhb()) ? "T: EHB  " : "";
	UBYTE ubTextColor = diagnosticsIsCurrentEhb() ? 31 : (1 << ubBpp) - 1;

#ifdef ACE_USE_AGA_FEATURES
	szBppRange = "2-8";
#else
	szBppRange = "2-5";
#endif

	sprintf(szTitle, "DIAG: %s", diagnosticsGetCurrentName());
	drawHeaderLine(4, szTitle, ubTextColor);
	sprintf(
		szControls, "%s: BPP  %sSPACE: AUTO %s  ESC: menu",
		szBppRange, szEhbControl, s_isAutoAdvance ? "ON" : "OFF"
	);
	drawHeaderLine(13, szControls, ubTextColor);

#ifdef ACE_USE_AGA_FEATURES
	if(diagnosticsIsCurrentAga()) {
		char szMode[96];

		sprintf(szMode, "Z/X/C/V: FMODE 0/1/2/3");
		drawHeaderLine(22, szMode, ubTextColor);
	}
	else
#endif
	if(diagnosticsIsCurrentEhb()) {
		drawHeaderLine(22, "EHB: colors 32-63 are half-brite", ubTextColor);
	}
}

void diagSimpleBufferBppCreate(void) {
	UBYTE ubBpp = diagnosticsGetCurrentBpp();

#ifdef ACE_USE_AGA_FEATURES
	if(diagnosticsIsCurrentAga()) {
		s_pView = viewCreate(0,
			TAG_VIEW_GLOBAL_PALETTE, 1,
			TAG_VIEW_USES_AGA, 1,
		TAG_END);
		s_pVPort = vPortCreate(0,
			TAG_VPORT_VIEW, s_pView,
			TAG_VPORT_BPP, ubBpp,
			TAG_VPORT_USES_AGA, 1,
			TAG_VPORT_FMODE, diagnosticsGetCurrentFmode(),
		TAG_END);
	}
	else
#endif
	{
		s_pView = viewCreate(0,
			TAG_VIEW_GLOBAL_PALETTE, 1,
		TAG_END);
		s_pVPort = vPortCreate(0,
			TAG_VPORT_VIEW, s_pView,
			TAG_VPORT_BPP, ubBpp,
		TAG_END);
	}

	s_pBuffer = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
	TAG_END);

	setupPalette(ubBpp);

	drawPattern(ubBpp);
	drawPaletteSwatches(ubBpp);
	drawHeader(ubBpp);

	viewLoad(s_pView);
	s_ulAutoAdvanceStart = timerGet();
}

void diagSimpleBufferBppLoop(void) {
	if(s_isAutoAdvance && timerGetDelta(s_ulAutoAdvanceStart, timerGet()) >= systemGetVerticalBlankFrequency() * 2) {
		diagnosticsNextTest();
		return;
	}

	if(keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}
	if(keyUse(KEY_SPACE)) {
		s_isAutoAdvance = !s_isAutoAdvance;
		s_ulAutoAdvanceStart = timerGet();
		drawPattern(diagnosticsGetCurrentBpp());
		drawPaletteSwatches(diagnosticsGetCurrentBpp());
		drawHeader(diagnosticsGetCurrentBpp());
		return;
	}
	if(keyUse(KEY_2)) {
		s_ulAutoAdvanceStart = timerGet();
		diagnosticsSelectSimpleBufferBpp(2);
		return;
	}
	if(keyUse(KEY_3)) {
		s_ulAutoAdvanceStart = timerGet();
		diagnosticsSelectSimpleBufferBpp(3);
		return;
	}
	if(keyUse(KEY_4)) {
		s_ulAutoAdvanceStart = timerGet();
		diagnosticsSelectSimpleBufferBpp(4);
		return;
	}
	if(keyUse(KEY_5)) {
		s_ulAutoAdvanceStart = timerGet();
		diagnosticsSelectSimpleBufferBpp(5);
		return;
	}
	if(keyUse(KEY_T)) {
		s_ulAutoAdvanceStart = timerGet();
		diagnosticsToggleSimpleBufferEhb();
		return;
	}
#ifdef ACE_USE_AGA_FEATURES
	if(keyUse(KEY_6)) {
		s_ulAutoAdvanceStart = timerGet();
		diagnosticsSelectSimpleBufferBpp(6);
		return;
	}
	if(keyUse(KEY_7)) {
		s_ulAutoAdvanceStart = timerGet();
		diagnosticsSelectSimpleBufferBpp(7);
		return;
	}
	if(keyUse(KEY_8)) {
		s_ulAutoAdvanceStart = timerGet();
		diagnosticsSelectSimpleBufferBpp(8);
		return;
	}
	if(keyUse(KEY_Z)) {
		s_ulAutoAdvanceStart = timerGet();
		diagnosticsSelectSimpleBufferFmode(0);
		return;
	}
	if(keyUse(KEY_X)) {
		s_ulAutoAdvanceStart = timerGet();
		diagnosticsSelectSimpleBufferFmode(1);
		return;
	}
	if(keyUse(KEY_C)) {
		s_ulAutoAdvanceStart = timerGet();
		diagnosticsSelectSimpleBufferFmode(2);
		return;
	}
	if(keyUse(KEY_V)) {
		s_ulAutoAdvanceStart = timerGet();
		diagnosticsSelectSimpleBufferFmode(3);
		return;
	}
#endif

	vPortWaitForEnd(s_pVPort);
}

void diagSimpleBufferBppDestroy(void) {
	viewLoad(0);
	viewDestroy(s_pView);
}
