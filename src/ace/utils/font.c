#include <ace/utils/font.h>
#include <ace/utils/file.h>

/* Globals */

/* Functions */

tFont *fontCreate(const char *szFontName) {
	tFile *pFontFile;
	tFont *pFont;
	logBlockBegin("fontCreate(szFontName: %s)", szFontName);

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

	fileRead(pFontFile, pFont, 2 * sizeof(UWORD) + sizeof(UBYTE));
	logWrite(
		"Addr: %p, data width: %upx, chars: %u, font height: %upx\n",
		pFont, pFont->uwWidth, pFont->ubChars, pFont->uwHeight
	);

	pFont->pCharOffsets = memAllocFast(sizeof(UWORD) * pFont->ubChars);
	fileRead(pFontFile, pFont->pCharOffsets, sizeof(UWORD) * pFont->ubChars);

	pFont->pRawData = bitmapCreate(pFont->uwWidth, pFont->uwHeight, 1, 0);
#ifdef AMIGA
	UWORD uwPlaneByteSize = (pFont->uwWidth+15)/8  * pFont->uwHeight;
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
		pFont = 0;
	}
	logBlockEnd("fontDestroy()");
}

tTextBitMap *fontCreateTextBitMap(tFont *pFont, const char *szText) {
	tTextBitMap *pTextBitMap;
	UBYTE *p;
	UWORD uwX;
	UWORD uwY = pFont->uwHeight;

	// Init text bitmap struct - also setting uwActualWidth to zero.
	pTextBitMap = memAllocFastClear(sizeof(tTextBitMap));

	// Text width measurement
	for (p = (UBYTE*)szText; *(p); ++p) {
		if(*p == '\n') {
			uwY += pFont->uwHeight;
		}
		else {
			pTextBitMap->uwActualWidth += (pFont->pCharOffsets[(*p) + 1] - pFont->pCharOffsets[*p]) + 1;
		}
	}

	// Bitmap init
	pTextBitMap->pBitMap = bitmapCreate(pTextBitMap->uwActualWidth, uwY, 1, BMF_CLEAR);

	// Draw text on bitmap buffer
	for (p = (UBYTE*)szText, uwX = 0, uwY = 0; *(p); ++p) {
		if(*p == '\n') {
			uwX = 0;
			uwY += pFont->uwHeight;
		}
		else {
			blitCopy(
				pFont->pRawData, pFont->pCharOffsets[*p], 0,
				pTextBitMap->pBitMap, uwX, uwY,
				pFont->pCharOffsets[(*p) + 1] - pFont->pCharOffsets[*p], pFont->uwHeight,
				MINTERM_COOKIE, 0x01
			);
			uwX += (pFont->pCharOffsets[(*p) + 1] - pFont->pCharOffsets[*p]) + 1;
		}
	}
	return pTextBitMap;
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
		uwY -= pTextBitMap->pBitMap->Rows;
	}
	else if(ubFlags & FONT_VCENTER) {
		uwY -= pTextBitMap->pBitMap->Rows>>1;
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
			pTextBitMap->uwActualWidth, pTextBitMap->pBitMap->Rows,
			ubMinterm, 0x01
		);
		ubColor >>= 1;
	}
}

void fontDrawStr(
	tBitMap *pDest, tFont *pFont, UWORD uwX, UWORD uwY,
	const char *szText, UBYTE ubColor, UBYTE ubFlags
) {
	logBlockBegin(
		"fontDrawStr(pDest: %p, pFont: %p, uwX: %hu, uwY: %hu, szText: '%s', "
		"ubColor: %hhu, ubFlags: %hhu)",
		pDest, pFont, uwX, uwY, szText, ubColor, ubFlags
	);
	tTextBitMap *pTextBitMap = fontCreateTextBitMap(pFont, szText);
	fontDrawTextBitMap(pDest, pTextBitMap, uwX, uwY, ubColor, ubFlags);
	fontDestroyTextBitMap(pTextBitMap);
	logBlockEnd("fontDrawStr()");
}
