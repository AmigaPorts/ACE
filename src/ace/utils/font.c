/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/font.h>
#include <ace/utils/file.h>

/* Globals */

/* Functions */

UBYTE fontGlyphWidth(const tFont *pFont, char c) {
	UBYTE ubIdx = (UBYTE)c;
	return pFont->pCharOffsets[ubIdx + 1] - pFont->pCharOffsets[ubIdx];
}

tFont *fontCreate(const char *szFontName) {
	tFile *pFontFile;
	tFont *pFont;
	logBlockBegin("fontCreate(szFontName: '%s')", szFontName);

	pFontFile = fileOpen(szFontName, "r");
	if (!pFontFile) {
		logBlockEnd("fontCreate()");
		return 0;
	}

	pFont = (tFont *) memAllocFast(sizeof(tFont));
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
#ifdef AMIGA
	UWORD uwPlaneByteSize = ((pFont->uwWidth+15)/16) * 2 * pFont->uwHeight;
	fileRead(pFontFile, pFont->pRawData->Planes[0], uwPlaneByteSize);
#else
	logWrite("ERR: Unimplemented\n");
	memFree(pFont, sizeof(tFont));
	fileClose(pFontFile);
	logBlockEnd("fontCreate()");
	return 0;
#endif // AMIGA

	fileClose(pFontFile);
	logBlockEnd("fontCreate()");
	return pFont;
}

void fontDestroy(tFont *pFont) {
	logBlockBegin("fontDestroy(pFont: %p)", pFont);
	if (pFont) {
		bitmapDestroy(pFont->pRawData);
		memFree(pFont->pCharOffsets, sizeof(UWORD) * pFont->ubChars);
		memFree(pFont, sizeof(tFont));
	}
	logBlockEnd("fontDestroy()");
}

tTextBitMap *fontCreateTextBitMap(UWORD uwWidth, UWORD uwHeight) {
	logBlockBegin(
		"fontCreateTextBitMap(uwWidth: %hu, uwHeight: %hu)", uwWidth, uwHeight
	);
	// Init text bitmap struct - also setting uwActualWidth and height to zero.
	tTextBitMap *pTextBitMap = memAllocFast(sizeof(tTextBitMap));
	pTextBitMap->pBitMap = bitmapCreate(uwWidth, uwHeight, 1, 0);
	pTextBitMap->uwActualWidth = 0;
	pTextBitMap->uwActualHeight = 0;
	logBlockEnd("fontCreateTextBitmap()");
	return pTextBitMap;
}

UBYTE fontTextFitsInTextBitmap(
	const tFont *pFont, const tTextBitMap *pTextBitmap, const char *szText
) {
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
	UWORD uwWidth = 0, uwHeight = 0, uwMaxWidth = 0;
	for (const char *p = szText; *p; ++p) {
		if(*p == '\n') {
			uwHeight += pFont->uwHeight;
			uwWidth = 0;
		}
		else {
			uwWidth += fontGlyphWidth(pFont, *p) + 1;
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
	UWORD uwWidth = 0;
	UWORD uwMaxWidth = 0;
	UWORD uwHeight = pFont->uwHeight;
	// Text width measurement
	for (const char *p = szText; *p; ++p) {
		if(*p == '\n') {
			uwHeight += pFont->uwHeight;
			uwWidth = 0;
		}
		else {
			uwWidth += fontGlyphWidth(pFont, *p) + 1;
			uwMaxWidth = MAX(uwMaxWidth, uwWidth);
		}
	}
	tTextBitMap *pTextBitMap = fontCreateTextBitMap(uwWidth, uwHeight);
	fontFillTextBitMap(pFont, pTextBitMap, szText);
	logBlockEnd("fontCreateTextBitMapFromStr()");
	return pTextBitMap;
}

void fontFillTextBitMap(
	const tFont *pFont, tTextBitMap *pTextBitMap, const char *szText
) {
	blitRect(
		pTextBitMap->pBitMap, 0, 0,
		pTextBitMap->pBitMap->BytesPerRow*8, pTextBitMap->pBitMap->Rows, 0
	);

#if defined(ACE_DEBUG)
	if(!fontTextFitsInTextBitmap(pFont, pTextBitMap, szText)) {
		logWrite("ERR: Text '%s' doesn't fit in text bitmap\n", szText);
		return;
	}
#endif
	// Draw text on bitmap buffer
	UWORD uwX = 0;
	UWORD uwY = 0;
	pTextBitMap->uwActualWidth = 0;
	for(const char *p = szText; *p; ++p) {
		if(*p == '\n') {
			uwX = 0;
			uwY += pFont->uwHeight;
		}
		else {
			UBYTE ubGlyphWidth = fontGlyphWidth(pFont, *p);
			blitCopy(
				pFont->pRawData, pFont->pCharOffsets[(UBYTE)*p], 0,
				pTextBitMap->pBitMap, uwX, uwY,
				ubGlyphWidth,
				pFont->uwHeight, MINTERM_COOKIE, 0x01
			);
			uwX += ubGlyphWidth + 1;
			pTextBitMap->uwActualWidth = MAX(pTextBitMap->uwActualWidth, uwX);
		}
	}
	pTextBitMap->uwActualHeight = uwY + pFont->uwHeight;
}

void fontDestroyTextBitMap(tTextBitMap *pTextBitMap) {
	bitmapDestroy(pTextBitMap->pBitMap);
	memFree(pTextBitMap, sizeof(tTextBitMap));
}

void fontDrawTextBitMap(
	tBitMap *pDest, tTextBitMap *pTextBitMap,
	UWORD uwX, UWORD uwY, UBYTE ubColor, UBYTE ubFlags
) {
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
	tBitMap sTmpDest;
#if defined(AMIGA)
	InitBitMap(&sTmpDest, 1, pDest->BytesPerRow<<3, pDest->Rows);
#else
#error "Something is missing here!"
#endif

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
				ubMinterm = 0xC0;
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
		sTmpDest.Planes[0] = pDest->Planes[i];
		blitCopy(
			pTextBitMap->pBitMap, 0, 0, &sTmpDest, uwX, uwY,
			pTextBitMap->uwActualWidth, pTextBitMap->uwActualHeight,
			ubMinterm, 0x01
		);
		ubColor >>= 1;
	}
}

void fontDrawStr(
	tBitMap *pDest, const tFont *pFont, UWORD uwX, UWORD uwY,
	const char *szText, UBYTE ubColor, UBYTE ubFlags
) {
	logBlockBegin(
		"fontDrawStr(pDest: %p, pFont: %p, uwX: %hu, uwY: %hu, szText: '%s', "
		"ubColor: %hhu, ubFlags: %hhu)",
		pDest, pFont, uwX, uwY, szText, ubColor, ubFlags
	);
	tTextBitMap *pTextBitMap = fontCreateTextBitMapFromStr(pFont, szText);
	fontFillTextBitMap(pFont, pTextBitMap, szText);
	fontDrawTextBitMap(pDest, pTextBitMap, uwX, uwY, ubColor, ubFlags);
	fontDestroyTextBitMap(pTextBitMap);
	logBlockEnd("fontDrawStr()");
}
