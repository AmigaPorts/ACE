/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Showcase: hardware sprites via the sprite manager.
 *
 * OCS walks three setups (attached 16-color, two-channel 32px, lone 16px).
 * AGA rebuild uses FMODE 32px fetch — one DMA channel per 32px sprite — and
 * cycles even/odd sprite palette banks so you can see ESPRM/OSPRM working.
 * Orb uses channel 0 so it draws in front of the others (priority = channel).
 * Graphics load from data/sprites.pak (pakFileOpen / pakFileGetFileByPath).
 */

#include "test/sprites.h"
#include <ace/generic/screen.h>
#include <ace/managers/copper.h>
#include <ace/managers/key.h>
#include <ace/managers/sprite.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/custom.h>
#include <ace/utils/extview.h>
#include <ace/utils/font.h>
#include <ace/utils/pak_file.h>
#include <ace/utils/palette.h>
#include <hardware/dmabits.h>
#include "game.h"

#ifdef ACE_USE_AGA_FEATURES
#include <ace/utils/sprite.h>
/* 32-bit sprite fetch, 16-bit playfield. FMODE is global — all sprites are 32px. */
#define SPRITE_FMODE_32 0x04
#endif

#define COLOR_BG 0
#define COLOR_TEXT 1
#define COLOR_DIM 2

#define ORB_W 16
#define ORB_H 24
#define ORB_X_MAX (SCREEN_PAL_WIDTH - ORB_W)
#define ORB_Y_MAX (SCREEN_PAL_HEIGHT - ORB_H)

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBfr;
static tFont *s_pFont;
static tTextBitMap *s_pTextBitMap;

/* Orb on ch0 (highest sprite priority). Rainbow attached ch2+3.
 * OCS stripe ch4+5, checker ch6. AGA stripe ch4, checker ch5 (odd bank). */
static tBitMap *s_pBmRainbowLo, *s_pBmRainbowHi;
static tBitMap *s_pBmStripe;
#ifndef ACE_USE_AGA_FEATURES
static tBitMap *s_pBmStripeR;
#endif
static tBitMap *s_pBmOrb, *s_pBmChecker;
static tSprite *s_pSprRainbowLo, *s_pSprRainbowHi;
static tSprite *s_pSprStripe;
#ifndef ACE_USE_AGA_FEATURES
static tSprite *s_pSprStripeR;
#endif
static tSprite *s_pSprOrb, *s_pSprChecker;
static WORD s_wXRainbow, s_wXStripe, s_wXOrb, s_wYOrb, s_wXChecker;
static BYTE s_bDirRainbow, s_bDirStripe, s_bDirChecker;
static BYTE s_bOrbVx, s_bOrbVy;
static UWORD s_pPal[32];
static tPakFile *s_pPak;

#ifdef ACE_USE_AGA_FEATURES
static UBYTE s_ubEvenBank;
static UBYTE s_ubOddBank;
static UBYTE s_ubBankTimer;
static UWORD s_pBankIce[15];
static UWORD s_pBankGold[15];
#endif

static void setSpritePos(tSprite *pSpr, WORD wX, WORD wY) {
	if(!pSpr) {
		return;
	}
	pSpr->wX = wX;
	pSpr->wY = wY;
	spriteRequestMetadataUpdate(pSpr);
}

static void labelAt(UWORD uwX, UWORD uwY, const char *sz) {
	if(!s_pFont) {
		return;
	}
	fontDrawStr(
		s_pFont, s_pBfr->pBack, uwX, uwY, sz, COLOR_TEXT, FONT_COOKIE, s_pTextBitMap
	);
}

static tBitMap *loadPakBitmap(const char *szName) {
	tFile *pFile;

	if(!s_pPak) {
		return 0;
	}
	pFile = pakFileGetFileByPath(s_pPak, szName);
	if(!pFile) {
		return 0;
	}
	return bitmapCreateFromFd(pFile, 0);
}

static void loadPakPalette(const char *szName, UWORD *pPal, UWORD uwMax) {
	tFile *pFile;

	if(!s_pPak) {
		return;
	}
	pFile = pakFileGetFileByPath(s_pPak, szName);
	if(!pFile) {
		return;
	}
	paletteLoadFromFd(pFile, pPal, uwMax);
}

#ifdef ACE_USE_AGA_FEATURES
static ULONG rgb12to24(UWORD uwRgb12) {
	UBYTE ubR = (UBYTE)(((uwRgb12 >> 8) & 0xF) * 0x11);
	UBYTE ubG = (UBYTE)(((uwRgb12 >> 4) & 0xF) * 0x11);
	UBYTE ubB = (UBYTE)((uwRgb12 & 0xF) * 0x11);
	return ((ULONG)ubR << 16) | ((ULONG)ubG << 8) | ubB;
}

static void setPal12(UWORD uwIdx, UWORD uwRgb12) {
	((ULONG *)s_pVPort->pPalette)[uwIdx] = rgb12to24(uwRgb12);
}

/* Write COLOR0..COLOR31 in bank N: once for low nibble, again with LOCT for high. */
static void pokeAgaColor12(UWORD uwIdx, UWORD uwRgb12) {
	UBYTE ubBank = (UBYTE)(uwIdx / 32);
	UBYTE ubReg = (UBYTE)(uwIdx % 32);
	uwRgb12 &= 0x0FFF;
	g_pCustom->bplcon3 = (UWORD)((UWORD)ubBank << 13);
	g_pCustom->color[ubReg] = uwRgb12;
	g_pCustom->bplcon3 = (UWORD)(((UWORD)ubBank << 13) | BV(9));
	g_pCustom->color[ubReg] = uwRgb12;
	g_pCustom->bplcon3 = 0;
}

static void fillSpriteBank12(UWORD uwBase, const UWORD *pRgb, UBYTE ubCount) {
	UBYTE i;
	for(i = 0; i < ubCount; ++i) {
		pokeAgaColor12((UWORD)(uwBase + i), pRgb[i]);
	}
}

static void pokeEvenLowColors(void) {
	const UWORD *pSrc;
	UBYTE i;

	if(s_ubEvenBank == 2) {
		pSrc = s_pBankIce;
	}
	else if(s_ubEvenBank == 3) {
		pSrc = s_pBankGold;
	}
	else {
		pSrc = &s_pPal[20];
	}
	/* CH0 → 1–3, CH2 → 4–7, CH4 → 8–11 when Denise uses the 0–15 map. */
	for(i = 0; i < 15; ++i) {
		pokeAgaColor12((UWORD)(1 + i), pSrc[i % 15]);
	}
}

static void applySpriteBanks(void) {
	spriteSetEvenColorPaletteBank(s_ubEvenBank);
	spriteSetOddColorPaletteBank(s_ubOddBank);
	pokeEvenLowColors();
}
#else
static void setPal12(UWORD uwIdx, UWORD uwRgb12) {
	s_pVPort->pPalette[uwIdx] = uwRgb12;
}
#endif

static void processSprites(void) {
	if(s_pSprRainbowLo) {
		spriteProcess(s_pSprRainbowLo);
	}
	if(s_pSprRainbowHi) {
		spriteProcess(s_pSprRainbowHi);
	}
	if(s_pSprStripe) {
		spriteProcess(s_pSprStripe);
	}
#ifndef ACE_USE_AGA_FEATURES
	if(s_pSprStripeR) {
		spriteProcess(s_pSprStripeR);
	}
#endif
	if(s_pSprOrb) {
		spriteProcess(s_pSprOrb);
	}
	if(s_pSprChecker) {
		spriteProcess(s_pSprChecker);
	}
	spriteProcessChannel(0);
	spriteProcessChannel(2);
	spriteProcessChannel(3);
	spriteProcessChannel(4);
	spriteProcessChannel(5);
#ifndef ACE_USE_AGA_FEATURES
	spriteProcessChannel(6);
#endif
}

void gsTestSpritesCreate(void) {
	UBYTE i;

#ifdef ACE_USE_AGA_FEATURES
	s_pView = viewCreate(0,
		TAG_VIEW_GLOBAL_PALETTE, 1,
		TAG_VIEW_USES_AGA, 1,
		TAG_DONE
	);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, SHOWCASE_BPP,
		TAG_VPORT_USES_AGA, 1,
		TAG_VPORT_FMODE, SPRITE_FMODE_32,
		TAG_DONE
	);
#else
	s_pView = viewCreate(0,
		TAG_VIEW_GLOBAL_PALETTE, 1,
		TAG_DONE
	);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, SHOWCASE_BPP,
		TAG_DONE
	);
#endif
	s_pBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
		TAG_DONE
	);

	s_pPak = pakFileOpen("data/sprites.pak", 1);
	loadPakPalette("sprites.plt", s_pPal, 32);
	for(i = 0; i < 32; ++i) {
		setPal12(i, s_pPal[i]);
	}
#ifdef ACE_USE_AGA_FEATURES
	/* Mirror sprite colors 17-31 into 1-15 so even 4-color sprites show. */
	for(i = 1; i < 16; ++i) {
		setPal12(i, s_pPal[i + 16]);
	}
	loadPakPalette("ice.plt", s_pBankIce, 15);
	loadPakPalette("gold.plt", s_pBankGold, 15);
#endif

	s_pBmRainbowLo = loadPakBitmap("rainbow_lo.bm");
	s_pBmRainbowHi = loadPakBitmap("rainbow_hi.bm");
#ifdef ACE_USE_AGA_FEATURES
	s_pBmStripe = loadPakBitmap("stripe.bm");
#else
	s_pBmStripe = loadPakBitmap("stripe_l.bm");
	s_pBmStripeR = loadPakBitmap("stripe_r.bm");
#endif
	s_pBmOrb = loadPakBitmap("orb.bm");
	s_pBmChecker = loadPakBitmap("checker.bm");
	if(s_pPak) {
		pakFileClose(s_pPak);
		s_pPak = 0;
	}

	s_pFont = fontCreateFromPath("data/silkscreen.fnt");
	s_pTextBitMap = s_pFont
		? fontCreateTextBitMap(SCREEN_PAL_WIDTH, s_pFont->uwHeight)
		: 0;

#ifdef ACE_USE_AGA_FEATURES
	labelAt(8, 8, "AGA sprites  ESC back  sprites pak");
	labelAt(8, 36, "Rainbow  16-color 32px  attached ch2+3");
	labelAt(8, 96, "Stripe  AGA 32px 4-color  ch4  FMODE");
	labelAt(8, 156, "Orb ch0 front  Checker ch5 odd-bank");
#else
	labelAt(8, 8, "Hardware sprites  ESC back  sprites pak");
	labelAt(8, 36, "Rainbow  16-color 16px  attached ch2+3");
	labelAt(8, 96, "Stripe  4-color 32px  ch4+5");
	labelAt(8, 156, "Orb ch0 front  Checker ch6");
#endif

	spriteManagerCreate(s_pView, 0, 0);
	systemSetDmaBit(DMAB_SPRITE, 1);

	/* Orb on ch0 — hardware sprite priority is the channel (0 in front of 7). */
	s_pSprOrb = spriteAdd(0, s_pBmOrb);

	/* Attached pair → 16 colors from the lo/hi bitplanes. */
	s_pSprRainbowLo = spriteAdd(2, s_pBmRainbowLo);
	s_pSprRainbowHi = spriteAdd(3, s_pBmRainbowHi);
	spriteSetAttached(s_pSprRainbowHi, 1);

#ifdef ACE_USE_AGA_FEATURES
	s_pSprStripe = spriteAdd(4, s_pBmStripe);
	s_pSprChecker = spriteAdd(5, s_pBmChecker);
#else
	s_pSprStripe = spriteAdd(4, s_pBmStripe);
	s_pSprStripeR = spriteAdd(5, s_pBmStripeR);
	s_pSprChecker = spriteAdd(6, s_pBmChecker);
#endif

	s_wXRainbow = 40;
	s_wXStripe = 80;
	s_wXOrb = 48;
	s_wYOrb = 72;
	s_wXChecker = 180;
	s_bDirRainbow = 1;
	s_bDirStripe = -1;
	s_bDirChecker = 1;
	s_bOrbVx = 2;
	s_bOrbVy = 2;

	setSpritePos(s_pSprRainbowLo, s_wXRainbow, 48);
	setSpritePos(s_pSprRainbowHi, s_wXRainbow, 48);
	setSpritePos(s_pSprStripe, s_wXStripe, 108);
#ifndef ACE_USE_AGA_FEATURES
	setSpritePos(s_pSprStripeR, (WORD)(s_wXStripe + 16), 108);
#endif
	setSpritePos(s_pSprOrb, s_wXOrb, s_wYOrb);
	setSpritePos(s_pSprChecker, s_wXChecker, 168);

	processSprites();

	viewLoad(s_pView);
	/* Sprites in front of playfield. */
	g_pCustom->bplcon2 = 0x20;

#ifdef ACE_USE_AGA_FEATURES
	/* Banks 1/2/3 hold default / ice / gold; index 0 is unused (transparent). */
	fillSpriteBank12(32 + 1, s_pBankIce, 15);
	fillSpriteBank12(48 + 1, s_pBankGold, 15);
	s_ubEvenBank = 1;
	s_ubOddBank = 2;
	s_ubBankTimer = 0;
	applySpriteBanks();
#endif
	systemUnuse();
}

void gsTestSpritesLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}

#ifdef ACE_USE_AGA_FEATURES
	/* ~1.5s at 50Hz: rotate even/odd banks, keep them different. */
	if(++s_ubBankTimer >= 75) {
		s_ubBankTimer = 0;
		s_ubEvenBank = (UBYTE)(1 + (s_ubEvenBank % 3));
		s_ubOddBank = (UBYTE)(1 + (s_ubOddBank % 3));
		if(s_ubOddBank == s_ubEvenBank) {
			s_ubOddBank = (UBYTE)(1 + (s_ubOddBank % 3));
		}
		applySpriteBanks();
	}
#endif

	/* Bounce the rows independently; orb flies in 2D over the rest. */
	s_wXRainbow = (WORD)(s_wXRainbow + s_bDirRainbow);
	if(s_wXRainbow > 280 || s_wXRainbow < 8) {
		s_bDirRainbow = (BYTE)-s_bDirRainbow;
	}
	s_wXStripe = (WORD)(s_wXStripe + s_bDirStripe);
	if(s_wXStripe > 260 || s_wXStripe < 8) {
		s_bDirStripe = (BYTE)-s_bDirStripe;
	}
	s_wXChecker = (WORD)(s_wXChecker + s_bDirChecker);
	if(s_wXChecker > 280 || s_wXChecker < 8) {
		s_bDirChecker = (BYTE)-s_bDirChecker;
	}

	s_wXOrb = (WORD)(s_wXOrb + s_bOrbVx);
	s_wYOrb = (WORD)(s_wYOrb + s_bOrbVy);
	if(s_wXOrb <= 0) {
		s_wXOrb = 0;
		s_bOrbVx = (BYTE)-s_bOrbVx;
	}
	else if(s_wXOrb >= ORB_X_MAX) {
		s_wXOrb = ORB_X_MAX;
		s_bOrbVx = (BYTE)-s_bOrbVx;
	}
	if(s_wYOrb <= 0) {
		s_wYOrb = 0;
		s_bOrbVy = (BYTE)-s_bOrbVy;
	}
	else if(s_wYOrb >= ORB_Y_MAX) {
		s_wYOrb = ORB_Y_MAX;
		s_bOrbVy = (BYTE)-s_bOrbVy;
	}

	setSpritePos(s_pSprRainbowLo, s_wXRainbow, 48);
	setSpritePos(s_pSprRainbowHi, s_wXRainbow, 48);
	setSpritePos(s_pSprStripe, s_wXStripe, 108);
#ifndef ACE_USE_AGA_FEATURES
	setSpritePos(s_pSprStripeR, (WORD)(s_wXStripe + 16), 108);
#endif
	setSpritePos(s_pSprOrb, s_wXOrb, s_wYOrb);
	setSpritePos(s_pSprChecker, s_wXChecker, 168);

	processSprites();
	copProcessBlocks();
	vPortWaitForEnd(s_pVPort);
}

void gsTestSpritesDestroy(void) {
	viewLoad(0);
	systemUse();
	systemSetDmaBit(DMAB_SPRITE, 0);
	spriteManagerDestroy();
	if(s_pBmRainbowLo) {
		bitmapDestroy(s_pBmRainbowLo);
	}
	if(s_pBmRainbowHi) {
		bitmapDestroy(s_pBmRainbowHi);
	}
	if(s_pBmStripe) {
		bitmapDestroy(s_pBmStripe);
	}
#ifndef ACE_USE_AGA_FEATURES
	if(s_pBmStripeR) {
		bitmapDestroy(s_pBmStripeR);
	}
#endif
	if(s_pBmOrb) {
		bitmapDestroy(s_pBmOrb);
	}
	if(s_pBmChecker) {
		bitmapDestroy(s_pBmChecker);
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
