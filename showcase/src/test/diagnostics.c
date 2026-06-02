#include "diagnostics.h"
#include "test/simple_buffer_bpp.h"
#include "test/scroll_tile_buffer.h"
#include <ace/managers/blit.h>
#include <ace/managers/game.h>
#include <ace/managers/key.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <stdio.h>

typedef struct tDiagnosticDef {
	const char *szName;
	UBYTE ubBpp;
	UBYTE ubFmode;
	UBYTE isEhb;
	UBYTE isAga;
	tStateCb cbCreate;
	tStateCb cbLoop;
	tStateCb cbDestroy;
} tDiagnosticDef;

#define DIAG_SIMPLE_BUFFER(szName, ubBpp, ubFmode, isEhb, isAga) { \
	szName, ubBpp, ubFmode, isEhb, isAga, \
	diagSimpleBufferBppCreate, diagSimpleBufferBppLoop, diagSimpleBufferBppDestroy \
}

static const tDiagnosticDef s_pDiagnostics[] = {
	DIAG_SIMPLE_BUFFER("SimpleBuffer 2 BPP", 2, 0, 0, 0),
	DIAG_SIMPLE_BUFFER("SimpleBuffer 3 BPP", 3, 0, 0, 0),
	DIAG_SIMPLE_BUFFER("SimpleBuffer 4 BPP", 4, 0, 0, 0),
	DIAG_SIMPLE_BUFFER("SimpleBuffer 5 BPP", 5, 0, 0, 0),
	DIAG_SIMPLE_BUFFER("SimpleBuffer 5 BPP EHB", 6, 0, 1, 0),
#ifdef ACE_USE_AGA_FEATURES
	DIAG_SIMPLE_BUFFER("SimpleBuffer AGA 6 BPP FMODE 0", 6, 0, 0, 1),
	DIAG_SIMPLE_BUFFER("SimpleBuffer AGA 6 BPP FMODE 1", 6, 1, 0, 1),
	DIAG_SIMPLE_BUFFER("SimpleBuffer AGA 6 BPP FMODE 2", 6, 2, 0, 1),
	DIAG_SIMPLE_BUFFER("SimpleBuffer AGA 6 BPP FMODE 3", 6, 3, 0, 1),
	DIAG_SIMPLE_BUFFER("SimpleBuffer AGA 7 BPP FMODE 0", 7, 0, 0, 1),
	DIAG_SIMPLE_BUFFER("SimpleBuffer AGA 7 BPP FMODE 1", 7, 1, 0, 1),
	DIAG_SIMPLE_BUFFER("SimpleBuffer AGA 7 BPP FMODE 2", 7, 2, 0, 1),
	DIAG_SIMPLE_BUFFER("SimpleBuffer AGA 7 BPP FMODE 3", 7, 3, 0, 1),
	DIAG_SIMPLE_BUFFER("SimpleBuffer AGA 8 BPP FMODE 0", 8, 0, 0, 1),
	DIAG_SIMPLE_BUFFER("SimpleBuffer AGA 8 BPP FMODE 1", 8, 1, 0, 1),
	DIAG_SIMPLE_BUFFER("SimpleBuffer AGA 8 BPP FMODE 2", 8, 2, 0, 1),
	DIAG_SIMPLE_BUFFER("SimpleBuffer AGA 8 BPP FMODE 3", 8, 3, 0, 1),
#endif
};

static UBYTE s_ubCurrentTest = 0;
static tFont *s_pFont;
static tTextBitMap *s_pTextBitMap;
static UBYTE s_isCurrentTestCreated = 0;
static tView *s_pMenuView;
static tVPort *s_pMenuVPort;
static tSimpleBufferManager *s_pMenuBuffer;
#ifdef ACE_USE_AGA_FEATURES
static UBYTE s_ubPreferredSimpleFmode = 0;
#endif

typedef enum tDiagnosticsMode {
	DIAGNOSTICS_MODE_MENU,
	DIAGNOSTICS_MODE_SIMPLE_BUFFER,
	DIAGNOSTICS_MODE_SCROLL_TILE_BUFFER,
} tDiagnosticsMode;

static tDiagnosticsMode s_eMode = DIAGNOSTICS_MODE_MENU;

#define DIAG_TEST_COUNT (sizeof(s_pDiagnostics) / sizeof(s_pDiagnostics[0]))

static void diagnosticsMenuCreate(void) {
	s_pMenuView = viewCreate(0,
		TAG_VIEW_GLOBAL_PALETTE, 1,
	TAG_END);
	s_pMenuVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pMenuView,
		TAG_VPORT_BPP, 4,
	TAG_END);
	s_pMenuBuffer = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pMenuVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
	TAG_END);

	s_pMenuVPort->pPalette[0] = 0x000;
	s_pMenuVPort->pPalette[1] = 0xFFF;
	s_pMenuVPort->pPalette[2] = 0x08F;
	s_pMenuVPort->pPalette[3] = 0x0F8;

	blitRect(
		s_pMenuBuffer->pBack, 0, 0,
		s_pMenuBuffer->uBfrBounds.uwX, s_pMenuBuffer->uBfrBounds.uwY, 0
	);
	fontDrawStr(
		s_pFont, s_pMenuBuffer->pBack, 16, 24,
		"ACE DIAGNOSTICS", 1, FONT_LEFT | FONT_TOP, s_pTextBitMap
	);
	fontDrawStr(
		s_pFont, s_pMenuBuffer->pBack, 16, 48,
		"1 SIMPLE BUFFER", 2, FONT_LEFT | FONT_TOP, s_pTextBitMap
	);
	fontDrawStr(
		s_pFont, s_pMenuBuffer->pBack, 16, 64,
		"2 SCROLLTILEBUFFER", 3, FONT_LEFT | FONT_TOP, s_pTextBitMap
	);
	fontDrawStr(
		s_pFont, s_pMenuBuffer->pBack, 16, 88,
		"ESC QUIT", 1, FONT_LEFT | FONT_TOP, s_pTextBitMap
	);

	viewLoad(s_pMenuView);
}

static void diagnosticsMenuDestroy(void) {
	viewLoad(0);
	viewDestroy(s_pMenuView);
}

static void diagnosticsMenuLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		gameExit();
		return;
	}
	if(keyUse(KEY_1)) {
		diagnosticsShowSimpleBuffer();
		return;
	}
	if(keyUse(KEY_2)) {
		diagnosticsShowScrollTileBuffer();
		return;
	}

	vPortWaitForEnd(s_pMenuVPort);
}

static void diagnosticsCreateCurrentTest(void) {
	switch(s_eMode) {
		case DIAGNOSTICS_MODE_MENU:
			diagnosticsMenuCreate();
			break;
		case DIAGNOSTICS_MODE_SIMPLE_BUFFER:
			s_pDiagnostics[s_ubCurrentTest].cbCreate();
			break;
		case DIAGNOSTICS_MODE_SCROLL_TILE_BUFFER:
			diagScrollTileBufferCreate();
			break;
	}
	s_isCurrentTestCreated = 1;
}

static void diagnosticsDestroyCurrentTest(void) {
	if(s_isCurrentTestCreated) {
		switch(s_eMode) {
			case DIAGNOSTICS_MODE_MENU:
				diagnosticsMenuDestroy();
				break;
			case DIAGNOSTICS_MODE_SIMPLE_BUFFER:
				s_pDiagnostics[s_ubCurrentTest].cbDestroy();
				break;
			case DIAGNOSTICS_MODE_SCROLL_TILE_BUFFER:
				diagScrollTileBufferDestroy();
				break;
		}
		s_isCurrentTestCreated = 0;
	}
}

void gsTestDiagSimpleBufferCreate(void) {
	s_pFont = fontCreateFromPath("data/fonts/quaver.fnt");
	s_pTextBitMap = fontCreateTextBitMap(336, s_pFont->uwHeight);
	s_eMode = DIAGNOSTICS_MODE_SIMPLE_BUFFER;
	diagnosticsCreateCurrentTest();
}

void gsTestDiagSimpleBufferLoop(void) {
	s_pDiagnostics[s_ubCurrentTest].cbLoop();
}

void gsTestDiagSimpleBufferDestroy(void) {
	diagnosticsDestroyCurrentTest();
	fontDestroyTextBitMap(s_pTextBitMap);
	fontDestroy(s_pFont);
}

void gsTestDiagScrollTileBufferCreate(void) {
	s_pFont = fontCreateFromPath("data/fonts/quaver.fnt");
	s_pTextBitMap = fontCreateTextBitMap(336, s_pFont->uwHeight);
	s_eMode = DIAGNOSTICS_MODE_SCROLL_TILE_BUFFER;
	diagnosticsCreateCurrentTest();
}

void gsTestDiagScrollTileBufferLoop(void) {
	diagScrollTileBufferLoop();
}

void gsTestDiagScrollTileBufferDestroy(void) {
	diagnosticsDestroyCurrentTest();
	fontDestroyTextBitMap(s_pTextBitMap);
	fontDestroy(s_pFont);
}

void diagnosticsChangeTo(UBYTE ubTestIndex) {
	diagnosticsDestroyCurrentTest();
	s_ubCurrentTest = ubTestIndex % DIAG_TEST_COUNT;
#ifdef ACE_USE_AGA_FEATURES
	if(s_pDiagnostics[s_ubCurrentTest].isAga) {
		s_ubPreferredSimpleFmode = s_pDiagnostics[s_ubCurrentTest].ubFmode;
	}
#endif
	diagnosticsCreateCurrentTest();
}

void diagnosticsShowMenu(void) {
	diagnosticsDestroyCurrentTest();
	s_eMode = DIAGNOSTICS_MODE_MENU;
	diagnosticsCreateCurrentTest();
}

void diagnosticsShowSimpleBuffer(void) {
	diagnosticsDestroyCurrentTest();
	s_eMode = DIAGNOSTICS_MODE_SIMPLE_BUFFER;
	diagnosticsCreateCurrentTest();
}

void diagnosticsShowScrollTileBuffer(void) {
	diagnosticsDestroyCurrentTest();
	s_eMode = DIAGNOSTICS_MODE_SCROLL_TILE_BUFFER;
	diagnosticsCreateCurrentTest();
}

void diagnosticsNextTest(void) {
	diagnosticsChangeTo((s_ubCurrentTest + 1) % DIAG_TEST_COUNT);
}

static UBYTE diagnosticsFindSimpleBuffer(UBYTE ubBpp, UBYTE ubFmode) {
#ifndef ACE_USE_AGA_FEATURES
	(void)ubFmode;
#endif
	for(UBYTE i = 0; i < DIAG_TEST_COUNT; ++i) {
		if(s_pDiagnostics[i].ubBpp != ubBpp) {
			continue;
		}
		if(s_pDiagnostics[i].isEhb) {
			continue;
		}
#ifdef ACE_USE_AGA_FEATURES
		if(ubBpp > 5) {
			if(s_pDiagnostics[i].isAga && s_pDiagnostics[i].ubFmode == ubFmode) {
				return i;
			}
		}
		else
#endif
		if(!s_pDiagnostics[i].isAga) {
			return i;
		}
	}

	return s_ubCurrentTest;
}

void diagnosticsSelectSimpleBufferBpp(UBYTE ubBpp) {
#ifdef ACE_USE_AGA_FEATURES
	UBYTE ubFmode = s_ubPreferredSimpleFmode;
#else
	UBYTE ubFmode = 0;
#endif
	UBYTE ubTestIndex = diagnosticsFindSimpleBuffer(ubBpp, ubFmode);

	if(ubTestIndex != s_ubCurrentTest) {
		diagnosticsChangeTo(ubTestIndex);
	}
}

void diagnosticsToggleSimpleBufferEhb(void) {
	if(s_pDiagnostics[s_ubCurrentTest].isEhb) {
		diagnosticsSelectSimpleBufferBpp(5);
		return;
	}
	if(s_pDiagnostics[s_ubCurrentTest].ubBpp != 5 || s_pDiagnostics[s_ubCurrentTest].isAga) {
		return;
	}

	for(UBYTE i = 0; i < DIAG_TEST_COUNT; ++i) {
		if(s_pDiagnostics[i].isEhb) {
			diagnosticsChangeTo(i);
			return;
		}
	}
}

void diagnosticsSelectSimpleBufferFmode(UBYTE ubFmode) {
#ifdef ACE_USE_AGA_FEATURES
	s_ubPreferredSimpleFmode = ubFmode;
	if(s_pDiagnostics[s_ubCurrentTest].isAga) {
		UBYTE ubTestIndex = diagnosticsFindSimpleBuffer(s_pDiagnostics[s_ubCurrentTest].ubBpp, ubFmode);

		if(ubTestIndex != s_ubCurrentTest) {
			diagnosticsChangeTo(ubTestIndex);
		}
	}
#else
	(void)ubFmode;
#endif
}

UBYTE diagnosticsGetCurrentBpp(void) {
	return s_pDiagnostics[s_ubCurrentTest].ubBpp;
}

UBYTE diagnosticsIsCurrentEhb(void) {
	return s_pDiagnostics[s_ubCurrentTest].isEhb;
}

#ifdef ACE_USE_AGA_FEATURES
UBYTE diagnosticsGetCurrentFmode(void) {
	return s_pDiagnostics[s_ubCurrentTest].ubFmode;
}

UBYTE diagnosticsIsCurrentAga(void) {
	return s_pDiagnostics[s_ubCurrentTest].isAga;
}
#endif

const char *diagnosticsGetCurrentName(void) {
	return s_pDiagnostics[s_ubCurrentTest].szName;
}

tFont *diagnosticsGetFont(void) {
	return s_pFont;
}

tTextBitMap *diagnosticsGetTextBitMap(void) {
	return s_pTextBitMap;
}
