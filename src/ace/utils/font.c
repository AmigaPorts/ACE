/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <proto/graphics.h> // Bartman's compiler needs this
#include <ace/macros.h>
#include <ace/managers/system.h>
#include <ace/utils/font.h>
#include <ace/utils/file.h>
#include <ace/utils/assume.h>

/* Globals */

static tBitMap s_sTmpDest; // Temp bitmap for drawing text on single bitplane

/* Functions */

UBYTE fontGlyphWidth(const tFont *pFont, char c) {
	UBYTE ubIdx = (UBYTE)c;
	return pFont->pCharOffsets[ubIdx + 1] - pFont->pCharOffsets[ubIdx];
}

tFont *fontCreate(const char *szFontName) {
	logBlockBegin("fontCreate(szFontName: '%s')", szFontName);
	assumeNotNull(szFontName);

	tFile *pFontFile = fileOpen(szFontName, "r");
	if (!pFontFile) {
		logWrite("ERR: Couldn't open file\n");
		logBlockEnd("fontCreate()");
		return 0;
	}

	tFont *pFont = (tFont *) memAllocFast(sizeof(tFont));
	if (!pFont) {
		fileClose(pFontFile);
		logBlockEnd("fontCreate()");
		return 0;
	}

	fileRead(pFontFile, &pFont->uwWidth, sizeof(UWORD));
	fileRead(pFontFile, &pFont->uwHeight, sizeof(UWORD));
	fileRead(pFontFile, &pFont->ubChars, sizeof(UBYTE));
	logWrite(
		"Addr: %p, data width: %upx, chars: %u, font height: %upx\n",
		pFont, pFont->uwWidth, pFont->ubChars, pFont->uwHeight
	);

	pFont->pCharOffsets = memAllocFast(sizeof(UWORD) * pFont->ubChars);
	fileRead(pFontFile, pFont->pCharOffsets, sizeof(UWORD) * pFont->ubChars);

	pFont->pRawData = bitmapCreate(pFont->uwWidth, pFont->uwHeight, 1, 0);
	assumeNotNull(pFont->pRawData);

	UWORD uwPlaneByteSize = ((pFont->uwWidth+15)/16) * 2 * pFont->uwHeight;
	fileRead(pFontFile, pFont->pRawData->Planes[0], uwPlaneByteSize);

	fileClose(pFontFile);
	logBlockEnd("fontCreate()");
	return pFont;
}

// TODO: remove, generate initialized font struct in .c/.h with font_conv instead
tFont *fontCreateFromMem(const UBYTE* pData) {
	logBlockBegin("fontCreateFromMem(szFontName: '%p')", pData);
 	tFont *pFont = (tFont *) memAllocFast(sizeof(tFont));
	if (!pFont) {
		logBlockEnd("fontCreateFromMem()");
		return 0;
	}

	UWORD uwCurByte = 0;
	memcpy(&pFont->uwWidth, &pData[uwCurByte], sizeof(UWORD));
	uwCurByte += sizeof(UWORD);
	memcpy(&pFont->uwHeight, &pData[uwCurByte], sizeof(UWORD));
	uwCurByte += sizeof(UWORD);
	memcpy(&pFont->ubChars, &pData[uwCurByte], sizeof(UBYTE));
	uwCurByte += sizeof(UBYTE);

	logWrite(
		"Addr: %p, data width: %upx, chars: %u, font height: %upx\n",
		pFont, pFont->uwWidth, pFont->ubChars, pFont->uwHeight
	);

	pFont->pCharOffsets = memAllocFast(sizeof(UWORD) * pFont->ubChars);
	memcpy(pFont->pCharOffsets,&pData[uwCurByte],sizeof(UWORD) * pFont->ubChars);
	uwCurByte+=(sizeof(UWORD) * pFont->ubChars);

	pFont->pRawData = bitmapCreate(pFont->uwWidth, pFont->uwHeight, 1, 0);

	UWORD uwPlaneByteSize = ((pFont->uwWidth+15)/16) * 2 * pFont->uwHeight;
	memcpy(pFont->pRawData->Planes[0],&pData[uwCurByte],uwPlaneByteSize);

	logBlockEnd("fontCreateFromMem()");
	return pFont;
}

void fontDestroy(tFont *pFont) {
	systemUse();
	logBlockBegin("fontDestroy(pFont: %p)", pFont);
	assumeNotNull(pFont);

	if(pFont) {
		bitmapDestroy(pFont->pRawData);
		memFree(pFont->pCharOffsets, sizeof(UWORD) * pFont->ubChars);
		memFree(pFont, sizeof(tFont));
	}
	logBlockEnd("fontDestroy()");
	systemUnuse();
}

tTextBitMap *fontCreateTextBitMap(UWORD uwWidth, UWORD uwHeight) {
	systemUse();
	logBlockBegin(
		"fontCreateTextBitMap(uwWidth: %hu, uwHeight: %hu)", uwWidth, uwHeight
	);

	tTextBitMap *pTextBitMap = memAllocFast(sizeof(*pTextBitMap));
	if(!pTextBitMap) {
		goto fail;
	}

	pTextBitMap->pBitMap = bitmapCreate(uwWidth, uwHeight, 1, BMF_CLEAR);
	if(!pTextBitMap->pBitMap) {
		goto fail;
	}

	// Mark pTextBitMap as without any text
	pTextBitMap->uwActualWidth = 0;
	pTextBitMap->uwActualHeight = 0;
	logBlockEnd("fontCreateTextBitmap()");
	systemUnuse();
	return pTextBitMap;

fail:
	if(pTextBitMap) {
		if(pTextBitMap->pBitMap) {
			bitmapDestroy(pTextBitMap->pBitMap);
		}
		memFree(pTextBitMap, sizeof(*pTextBitMap));
	}
	logBlockEnd("fontCreateTextBitmap()");
	systemUnuse();
	return 0;
}

UBYTE fontTextFitsInTextBitmap(
	const tFont *pFont, const tTextBitMap *pTextBitmap, const char *szText
) {
	assumeNotNull(pFont);
	assumeNotNull(pTextBitmap);
	assumeNotNull(szText);

	tUwCoordYX sBounds = fontMeasureText(pFont, szText);
	if(
		sBounds.uwX <= bitmapGetByteWidth(pTextBitmap->pBitMap) * 8 &&
		sBounds.uwY <= pTextBitmap->pBitMap->Rows
	) {
		return 1;
	}
	return 0;
}

tUwCoordYX fontMeasureText(const tFont *pFont, const char *szText) {
	assumeNotNull(pFont);
	assumeNotNull(szText);

	UWORD uwWidth = 0, uwHeight = 0, uwMaxWidth = 0;
	for (const char *p = szText; *p; ++p) {
		if(*p == '\n') {
			uwHeight += pFont->uwHeight;
			uwWidth = 0;
		}
		else {
			UBYTE ubGlyphWidth = fontGlyphWidth(pFont, *p);
			assumeMsg(ubGlyphWidth != 0, "Missing glyph for char");

			uwWidth += ubGlyphWidth + 1;
			uwMaxWidth = MAX(uwMaxWidth, uwWidth);
		}
	}
	uwHeight += pFont->uwHeight; // Add height of last line
	tUwCoordYX sBounds = {.uwX = uwMaxWidth, .uwY = uwHeight};
	return sBounds;
}

tTextBitMap *fontCreateTextBitMapFromStr(const tFont *pFont, const char *szText) {
	logBlockBegin(
		"fontCreateTextBitMapFromStr(pFont: %p, szText: '%s')", pFont, szText
	);
	assumeNotNull(pFont);
	assumeNotNull(szText);

	tUwCoordYX sBounds = fontMeasureText(pFont, szText);
	// If bitmap is too tight then blitter goes nuts with bltXdat caching when
	// going into next line of blit
	tTextBitMap *pTextBitMap = fontCreateTextBitMap(
		(blockCountCeil(sBounds.uwX, 16) + 1) * 16, sBounds.uwY
	);
	fontFillTextBitMap(pFont, pTextBitMap, szText);
	logBlockEnd("fontCreateTextBitMapFromStr()");
	return pTextBitMap;
}

tUwCoordYX fontDrawStr1bpp(
	const tFont *pFont, tBitMap *pBitMap, UWORD uwStartX, UWORD uwStartY,
	const char *szText
) {
	assumeNotNull(pFont);
	assumeNotNull(pBitMap);
	assumeNotNull(szText);

	UWORD uwX = uwStartX;
	UWORD uwY = uwStartY;
	UWORD uwBoundX = 0;
	for(const char *p = szText; *p; ++p) {
		if(*p == '\n') {
			uwBoundX = MAX(uwBoundX, uwX);
			uwX = uwStartX;
			uwY += pFont->uwHeight;
		}
		else {
			UBYTE ubGlyphWidth = fontGlyphWidth(pFont, *p);
			assumeMsg(ubGlyphWidth != 0, "Missing glyph for char");
			blitCopy(
				pFont->pRawData, pFont->pCharOffsets[(UBYTE)*p], 0, pBitMap, uwX, uwY,
				ubGlyphWidth, pFont->uwHeight, MINTERM_COOKIE
			);
			uwX += ubGlyphWidth + 1;
		}
	}
	tUwCoordYX sBounds = {.uwX = MAX(uwBoundX, uwX), .uwY = uwY + pFont->uwHeight};
	return sBounds;
}

void fontFillTextBitMap(
	const tFont *pFont, tTextBitMap *pTextBitMap, const char *szText
) {
	assumeNotNull(pFont);
	assumeNotNull(pTextBitMap);
	assumeNotNull(szText);

	if(pTextBitMap->uwActualWidth) {
		// Clear old contents
		// TODO: we could remove this clear if letter spacing would be
		// part of glyphs, but it may cause some problems when drawing last letter.
		blitRect(
			pTextBitMap->pBitMap, 0, 0,
			pTextBitMap->uwActualWidth, pTextBitMap->pBitMap->Rows, 0
		);
	}

	assumeMsg(
		fontTextFitsInTextBitmap(pFont, pTextBitMap, szText),
		"Text doesn't fit in text bitmap"
	);

	tUwCoordYX sBounds = fontDrawStr1bpp(pFont, pTextBitMap->pBitMap, 0, 0, szText);
	pTextBitMap->uwActualWidth = sBounds.uwX;
	pTextBitMap->uwActualHeight = sBounds.uwY;
}

void fontDestroyTextBitMap(tTextBitMap *pTextBitMap) {
	assumeNotNull(pTextBitMap);

	systemUse();
	bitmapDestroy(pTextBitMap->pBitMap);
	memFree(pTextBitMap, sizeof(tTextBitMap));
	systemUnuse();
}

void fontDrawTextBitMap(
	tBitMap *pDest, tTextBitMap *pTextBitMap,
	UWORD uwX, UWORD uwY, UBYTE ubColor, UBYTE ubFlags
) {
	assumeNotNull(pDest);
	assumeNotNull(pTextBitMap);
	assumeMsg(pTextBitMap->uwActualWidth > 0, "pTextBitMap has text of zero width");

	// Alignment flags
	if (ubFlags & FONT_RIGHT) {
		uwX -= pTextBitMap->uwActualWidth;
	}
	else if (ubFlags & FONT_HCENTER) {
		uwX -= pTextBitMap->uwActualWidth>>1;
	}
	if(ubFlags & FONT_BOTTOM) {
		uwY -= pTextBitMap->uwActualHeight;
	}
	else if(ubFlags & FONT_VCENTER) {
		uwY -= pTextBitMap->uwActualHeight>>1;
	}

	if(ubFlags & FONT_SHADOW) {
		fontDrawTextBitMap(pDest, pTextBitMap, uwX, uwY+1, 0, FONT_COOKIE);
	}

	// Helper destination bitmap
	s_sTmpDest.BytesPerRow = pDest->BytesPerRow;
	s_sTmpDest.Rows = pDest->Rows;
	s_sTmpDest.Depth = 1;

	// Text-drawing loop
	UBYTE isCookie = ubFlags & FONT_COOKIE;
	UBYTE isLazy = ubFlags & FONT_LAZY;
	UBYTE ubMinterm;
	for (UBYTE i = 0; i != pDest->Depth; ++i) {
		// Determine minterm for given bitplane
		if(isCookie) {
			ubMinterm = ubColor & 1 ? 0xEA : 0x2A;
		}
		else {
			if(ubColor & 1) {
				ubMinterm = MINTERM_COPY;
			}
			else {
				if(isLazy) {
					ubColor >>= 1;
					continue;
				}
				ubMinterm = 0x00;
			}
		}

		// Blit on given bitplane
		// OPTIMIZE: blitCopy does lots of calculations which are essentially the
		// same for all bitplanes since they share dimensions. Get rid of it.
		s_sTmpDest.Planes[0] = pDest->Planes[i];
		blitCopy(
			pTextBitMap->pBitMap, 0, 0, &s_sTmpDest, uwX, uwY,
			pTextBitMap->uwActualWidth, pTextBitMap->uwActualHeight, ubMinterm
		);
		ubColor >>= 1;
	}
}

void fontDrawStr(
	const tFont *pFont, tBitMap *pDest, UWORD uwX, UWORD uwY,
	const char *szText, UBYTE ubColor, UBYTE ubFlags, tTextBitMap *pTextBitMap
) {
	fontFillTextBitMap(pFont, pTextBitMap, szText);
	fontDrawTextBitMap(pDest, pTextBitMap, uwX, uwY, ubColor, ubFlags);
}
