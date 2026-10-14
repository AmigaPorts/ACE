/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "examples/shooter_vertical/art.h"
#include <string.h>
#include <ace/managers/blit.h>

/* Hex digits are palette indices, '.' is transparent. Both attached pairs
 * share these colors; indices 5/6 are reserved for red/green shot DMA. */
static const char s_pPlayer[16][17] = {
	".......33.......", "......3ff3......",
	"......3bb3......", ".....3fbbf3.....",
	".....3b11b3.....", "....3fb11bf3....",
	"....3fb44bf3....", "...3ffb44bff3...",
	"..3f2fb44bf2f3..", ".3ff2ffbbff2ff3.",
	"3fff2ffffff2fff3", "32222ff99ff22223",
	".3333f9889f3333.", "....338ee833....",
	".....38ee83.....", "......8998......"
};
static const char s_pEnemy[12][17] = {
	"..a..........a..", "...a...77...a...",
	"...ca77dd77ac...", "..acddffffddca..",
	".acdd7dddd7ddca.", "acddffddddffddca",
	"addd55dddd55ddda", ".adddddddddddda.",
	"..acdd9999ddca..", "...accddddcca...",
	"..aa..c..c..aa..", ".a....e..e....a."
};

/* 3x5 uppercase glyphs: each row is a three-bit mask. */
static const UBYTE s_pGlyphs[36][5] = {
	{7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7},
	{5,5,7,1,1}, {7,4,7,1,7}, {7,4,7,5,7}, {7,1,2,2,2},
	{7,5,7,5,7}, {7,5,7,1,7},
	{2,5,7,5,5}, {6,5,6,5,6}, {3,4,4,4,3}, {6,5,5,5,6},
	{7,4,6,4,7}, {7,4,6,4,4}, {3,4,5,5,3}, {5,5,7,5,5},
	{7,2,2,2,7}, {1,1,1,5,2}, {5,5,6,5,5}, {4,4,4,4,7},
	{5,7,7,5,5}, {5,7,7,7,5}, {2,5,5,5,2}, {6,5,6,4,4},
	{2,5,5,7,3}, {6,5,6,5,5}, {3,4,2,1,6}, {7,2,2,2,2},
	{5,5,5,5,7}, {5,5,5,5,2}, {5,5,7,7,5}, {5,5,2,5,5},
	{5,5,2,2,2}, {7,1,2,4,7}
};

/** BytesPerRow already includes depth for ACE interleaved bitmaps. */
static void putPixel(tBitMap *pBitmap, UWORD uwX, UWORD uwY, UBYTE ubColor) {
	ULONG ulOffset = (ULONG)uwY * pBitmap->BytesPerRow + (uwX >> 3);
	UBYTE ubMask = 0x80 >> (uwX & 7);
	for(UBYTE ubPlane = 0; ubPlane < pBitmap->Depth; ++ubPlane) {
		UBYTE *pByte = (UBYTE *)pBitmap->Planes[ubPlane] + ulOffset;
		*pByte = (*pByte & ~ubMask) | ((ubColor & (1 << ubPlane)) ? ubMask : 0);
	}
}

/** Byte-aligned HUD rectangles avoid per-pixel clearing on the 68000. */
static void clearRect(
	tBitMap *pBitmap, UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight
) {
	for(UBYTE ubPlane = 0; ubPlane < pBitmap->Depth; ++ubPlane) {
		for(UWORD uwRow = uwY; uwRow < uwY + uwHeight; ++uwRow) {
			memset((UBYTE *)pBitmap->Planes[ubPlane] +
				(ULONG)uwRow * pBitmap->BytesPerRow + (uwX >> 3), 0, uwWidth >> 3);
		}
	}
}

/** Render only supported glyphs; spaces advance without writing pixels. */
static void drawText(
	tBitMap *pBitmap, UWORD uwX, UWORD uwY,
	const char *szText, UBYTE ubColor, UBYTE ubScale
) {
	while(*szText) {
		char cGlyph = *szText++;
		UBYTE ubIndex = 36;
		if(cGlyph >= '0' && cGlyph <= '9') {
			ubIndex = cGlyph - '0';
		}
		else if(cGlyph >= 'A' && cGlyph <= 'Z') {
			ubIndex = cGlyph - 'A' + 10;
		}
		if(ubIndex < 36) {
			for(UBYTE ubY = 0; ubY < 5 * ubScale; ++ubY) {
				for(UBYTE ubX = 0; ubX < 3 * ubScale; ++ubX) {
					if(s_pGlyphs[ubIndex][ubY / ubScale] & (4 >> (ubX / ubScale))) {
						putPixel(pBitmap, uwX + ubX, uwY + ubY, ubColor);
					}
				}
			}
		}
		uwX += 4 * ubScale;
	}
}

/** Decode the compact indexed maps without an external asset pipeline. */
static UBYTE mapColor(char cPixel) {
	if(cPixel >= '0' && cPixel <= '9') {
		return cPixel - '0';
	}
	if(cPixel >= 'a' && cPixel <= 'f') {
		return cPixel - 'a' + 10;
	}
	return 0;
}

void shooterVertArtPalette(UWORD *pPalette) {
	static const UWORD pColors[32] = {
		0x012, 0x023, 0x035, 0x057, 0x079, 0x09b, 0x2bd, 0x7df,
		0xabc, 0xdef, 0x347, 0x568, 0x89a, 0xf94, 0xf46, 0xfff,
		0x000, 0x058, 0x68a, 0x246, 0x0cf, 0xf34, 0x3f7, 0x805,
		0xf60, 0xfb4, 0xa28, 0x6ef, 0xf5b, 0xd19, 0xff9, 0xdef
	};
	memcpy(pPalette, pColors, sizeof(pColors));
}

tBitMap *shooterVertArtPlayer(UBYTE ubHalf) {
	tBitMap *pBitmap = bitmapCreate(16, 18, 2, BMF_CLEAR | BMF_INTERLEAVED);
	if(!pBitmap) {
		return 0;
	}
	blitWait();
	for(UWORD uwY = 0; uwY < 16; ++uwY) {
		for(UWORD uwX = 0; uwX < 16; ++uwX) {
			UBYTE ubColor = mapColor(s_pPlayer[uwY][uwX]);
			putPixel(pBitmap, uwX, uwY + 1, (ubColor >> (ubHalf * 2)) & 3);
		}
	}
	return pBitmap;
}

tBitMap *shooterVertArtEnemies(void) {
	tBitMap *pBitmap = bitmapCreate(16, 24, 4, BMF_CLEAR | BMF_INTERLEAVED);
	if(!pBitmap) {
		return 0;
	}
	blitWait();
	for(UBYTE ubFrame = 0; ubFrame < 2; ++ubFrame) {
		for(UWORD uwY = 0; uwY < 12; ++uwY) {
			for(UWORD uwX = 0; uwX < 16; ++uwX) {
				UBYTE ubColor = mapColor(s_pEnemy[uwY][uwX]);
				if(ubFrame && (ubColor == 5 || ubColor == 14)) {
					ubColor = ubColor == 5 ? 14 : 8;
				}
				putPixel(pBitmap, uwX, uwY + ubFrame * 12, ubColor);
			}
		}
	}
	return pBitmap;
}

tBitMap *shooterVertArtShot(UBYTE ubColor) {
	tBitMap *pBitmap = bitmapCreate(16, 4, 2, BMF_CLEAR | BMF_INTERLEAVED);
	if(!pBitmap) {
		return 0;
	}
	blitWait();
	for(UWORD uwY = 0; uwY < 4; ++uwY) {
		putPixel(pBitmap, 0, uwY, ubColor);
		putPixel(pBitmap, 1, uwY, ubColor);
	}
	return pBitmap;
}

void shooterVertArtBackground(tBitMap *pBitmap) {
	ULONG ulRandom = 0x53484152;
	blitWait();
	/* Sparse deterministic stars leave the black-blue negative space readable. */
	for(UWORD uwStar = 0; uwStar < 420; ++uwStar) {
		ulRandom = ulRandom * 1664525UL + 1013904223UL;
		UWORD uwX = 4 + ((ulRandom >> 16) % 312);
		ulRandom = ulRandom * 1664525UL + 1013904223UL;
		UWORD uwY = 26 + ((ulRandom >> 16) % 226);
		UBYTE ubColor = 1 + (uwStar % 4);
		if(!(uwStar % 31)) {
			ubColor = 9;
			putPixel(pBitmap, uwX - 1, uwY, 2);
			putPixel(pBitmap, uwX + 1, uwY, 2);
			putPixel(pBitmap, uwX, uwY - 1, 2);
			putPixel(pBitmap, uwX, uwY + 1, 2);
		}
		putPixel(pBitmap, uwX, uwY, ubColor);
	}
	for(UWORD uwX = 8; uwX < 312; ++uwX) {
		putPixel(pBitmap, uwX, 23, uwX < 112 ? 5 : 2);
	}
	drawText(pBitmap, 8, 3, "STAR SENTINEL", 7, 2);
}

void shooterVertArtHud(
	tBitMap *pBitmap, ULONG ulScore, UBYTE ubLives, UBYTE isGameOver,
	UBYTE isScoreDirty, UBYTE isLivesDirty, UBYTE isStatusDirty
) {
	if(isScoreDirty) {
		char szScore[7];
		for(UBYTE ubDigit = 6; ubDigit--;) {
			szScore[ubDigit] = '0' + ulScore % 10;
			ulScore /= 10;
		}
		szScore[6] = 0;
		clearRect(pBitmap, 136, 3, 96, 9);
		drawText(pBitmap, 140, 5, "SCORE", 8, 1);
		drawText(pBitmap, 168, 5, szScore, 9, 1);
	}
	if(isLivesDirty) {
		char szLives[2] = {'0' + ubLives, 0};
		clearRect(pBitmap, 248, 3, 64, 9);
		drawText(pBitmap, 252, 5, "LIVES", 8, 1);
		drawText(pBitmap, 284, 4, szLives, 13, 1);
	}
	if(isStatusDirty) {
		clearRect(pBitmap, 8, 15, 304, 7);
		drawText(pBitmap, 8, 16, isGameOver ?
			"GAME OVER   R OR FIRE TO RESTART" :
			"ARROWS MOVE   SPACE FIRE   JOY2   ESC EXIT",
			isGameOver ? 14 : 8, 1);
	}
}
