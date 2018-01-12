#include <ace/utils/font.h>

/* Globals */

/* Functions */

tFont *fontCreate(char *szFontName) {
	FILE *pFontFile;
	tFont *pFont;
	logBlockBegin("fontCreate(szFontName: %s)", szFontName);

	pFontFile = fopen(szFontName, "r");
	if (!pFontFile) {
		logBlockEnd("fontCreate()");
		return 0;
	}

	pFont = (tFont *) memAllocFast(sizeof(tFont));
	if (!pFont) {
		logBlockEnd("fontCreate()");
		fclose(pFontFile);
		return 0;
	}

	fread(pFont, 2 * sizeof(UWORD) + sizeof(UBYTE), 1, pFontFile);
	logWrite("Addr: %p, data width: %upx, chars: %u, font height: %upx\n", pFont, pFont->uwWidth, pFont->ubChars, pFont->uwHeight);

	pFont->pCharOffsets = memAllocFast(sizeof(UWORD) * pFont->ubChars);
	fread(pFont->pCharOffsets, sizeof(UWORD), pFont->ubChars, pFontFile);

	pFont->pRawData = memAllocChip(sizeof(tBitMap));
#ifdef AMIGA
	InitBitMap(pFont->pRawData, 1, pFont->uwWidth, pFont->uwHeight);

	pFont->pRawData->Planes[0] = AllocRaster(pFont->uwWidth, pFont->uwHeight);
	fread(pFont->pRawData->Planes[0], 1, (pFont->uwWidth >> 3) * pFont->uwHeight, pFontFile);
#else
	logWrite("ERR: Unimplemented\n");
	memFree(pFont, sizeof(tFont));
	fclose(pFontFile);
	logBlockEnd("fontCreate()");
	return 0;
#endif // AMIGA

	fclose(pFontFile);
	logBlockEnd("fontCreate()");
	return pFont;
}

void fontDestroy(tFont *pFont) {
	logBlockBegin("fontDestroy(pFont: %p)", pFont);
	if (pFont) {
#ifdef AMIGA
		FreeRaster(pFont->pRawData->Planes[0], pFont->pRawData->BytesPerRow << 3, pFont->pRawData->Rows);
#endif // AMIGA
		memFree(pFont->pRawData, sizeof(tBitMap));
		memFree(pFont->pCharOffsets, sizeof(UWORD) * pFont->ubChars);
		memFree(pFont, sizeof(tFont));
		pFont = 0;
	}
	logBlockEnd("fontDestroy()");
}

tTextBitMap *fontCreateTextBitMap(tFont *pFont, char *szText) {
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
		} else {
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
		} else {
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

void fontDrawTextBitMap(tBitMap *pDest, tTextBitMap *pTextBitMap, UWORD uwX, UWORD uwY, UBYTE ubColor, UBYTE ubFlags) {
	UBYTE i;
	UBYTE ubMinterm;
	tBitMap sTmpDest;

	// Alignment flags
	if (ubFlags & FONT_RIGHT)
		uwX -= pTextBitMap->uwActualWidth;
	else if (ubFlags & FONT_HCENTER)
		uwX -= pTextBitMap->uwActualWidth>>1;
	if(ubFlags & FONT_BOTTOM)
		uwY -= pTextBitMap->pBitMap->Rows;
	else if(ubFlags & FONT_VCENTER)
		uwY -= pTextBitMap->pBitMap->Rows>>1;

	if(ubFlags & FONT_SHADOW)
		fontDrawTextBitMap(pDest, pTextBitMap, uwX, uwY+1, 0, FONT_COOKIE);

	// Helper destination bitmap
#ifdef AMIGA
	InitBitMap(&sTmpDest, 1, pDest->BytesPerRow<<3, pDest->Rows);
#endif

	// Text-drawing loop
	for (i = 0; i != pDest->Depth; ++i) {
		// Determine minterm for given bitplane
		if(ubFlags & FONT_COOKIE) {
			if(ubColor & 1)
				ubMinterm = 0xEA;
			else
				ubMinterm = 0x2A;
		}
		else {
			if(ubColor & 1)
				ubMinterm = 0xC0;
			else {
				if(ubFlags & FONT_LAZY) {
					ubColor >>= 1;
					continue;
				}
				ubMinterm = 0x00;
			}
		}
		// Blit on given bitplane
		sTmpDest.Planes[0] = pDest->Planes[i];
		blitCopy(
			pTextBitMap->pBitMap, 0, 0,
			&sTmpDest, uwX, uwY,
			pTextBitMap->uwActualWidth, pTextBitMap->pBitMap->Rows,
			ubMinterm, 0x01
		);
		ubColor >>= 1;
	}
}

void fontDrawStr(
	tBitMap *pDest, tFont *pFont, UWORD uwX, UWORD uwY,
	char *szText, UBYTE ubColor, UBYTE ubFlags
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
