#include <ace/utils/font.h>

/**
 * Font & text drawing utils
 * Maintainer: KaiN
 *
 * TODO: Rozwazyc prefix txt, lub rozdzielenie na dwa pliki: font, txt
 */
 
/* Globals */

/* Functions */

/**
 * Creates font instance from given path
 */
tFont *fontCreate(char *szFontName) {
	FILE *pFontFile;
	tFont *pFont;
	logBlockBegin("fontCreate(szFontName: %s)", szFontName);
	
	pFontFile = fopen(szFontName, "r");
	if (!pFontFile)
		return 0;
	
	pFont = (tFont *) memAllocFast(sizeof(tFont));
	if (!pFont)
		return 0;
	
	fread(pFont, 2 * sizeof(UWORD) + sizeof(UBYTE), 1, pFontFile);
	logWrite("Addr: %p, data width: %upx, chars: %u, font height: %upx\n", pFont, pFont->uwWidth, pFont->ubChars, pFont->uwHeight);
	
	pFont->pCharOffsets = (UWORD *) memAllocFast(sizeof(UWORD) * pFont->ubChars);
	fread(pFont->pCharOffsets, sizeof(UWORD), pFont->ubChars, pFontFile);
	
	pFont->pRawData = (struct BitMap *) memAllocChip(sizeof(struct BitMap));
	InitBitMap(pFont->pRawData, 1, pFont->uwWidth, pFont->uwHeight);
	
	pFont->pRawData->Planes[0] = AllocRaster(pFont->uwWidth, pFont->uwHeight);
	fread(pFont->pRawData->Planes[0], 1, (pFont->uwWidth >> 3) * pFont->uwHeight, pFontFile);
	
	fclose(pFontFile);
	logBlockEnd("fontCreate()");
	return pFont;
}

/**
 * Destroys given font instance
 */
void fontDestroy(tFont *pFont) {
	logBlockBegin("fontDestroy(pFont: %p)", pFont);
	if (pFont) {
		FreeRaster(pFont->pRawData->Planes[0], pFont->pRawData->BytesPerRow << 3, pFont->pRawData->Rows);
		memFree(pFont->pRawData, sizeof(struct BitMap));
		memFree(pFont->pCharOffsets, sizeof(UWORD) * pFont->ubChars);
		memFree(pFont, sizeof(tFont));
		pFont = 0;
	}
	logBlockEnd("fontDestroy()");
}

/**
 * Prepares buffered text bitmap written with supplied font
 * Treat as cache - allows faster reblit of text without need of assembling it again
 */
tTextBitMap *fontCreateTextBitMap(tFont *pFont, char *szText) {
	tTextBitMap *pTextBitMap;
	char *p;
	UWORD uwX;
	UWORD uwY = pFont->uwHeight;

	// Init struktury bitmapy napisu - przy okazji wyzerowanie uwActualWidth
	pTextBitMap = memAllocFastClear(sizeof(tTextBitMap));

	// Zmierzenie d³ugoœci napisu
	for (p = szText; *(p); ++p) {
		if(*p == '\n') {
			uwY += pFont->uwHeight;
		} else {
			pTextBitMap->uwActualWidth += (pFont->pCharOffsets[(*p) + 1] - pFont->pCharOffsets[*p]) + 1;
		}
	}
	
	// Init bitmapy
	pTextBitMap->pBitMap = bitmapCreate(pTextBitMap->uwActualWidth, uwY, 1, BMF_CLEAR);

	// Odrysowanie napisu na bitmapie tekstu
	for (p = szText, uwX = 0, uwY = 0; *(p); ++p) {
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

/**
 * Destroys buffered text bitmap
 */
void fontDestroyTextBitMap(tTextBitMap *pTextBitMap) {
	bitmapDestroy(pTextBitMap->pBitMap);
	memFree(pTextBitMap, sizeof(tTextBitMap));
}

/**
 * Draws prepared text to destination
 * Each bitplane has its own minterm related to color code bits
 * Useful links:
 * 	About minterms:       http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node011D.html
 * 	Creating own minterm: http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node011E.html
 */
void fontDrawTextBitMap(struct BitMap *pDest, tTextBitMap *pTextBitMap, UWORD uwX, UWORD uwY, UBYTE ubColor, UBYTE ubFlags) {
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
	InitBitMap(&sTmpDest, 1, pDest->BytesPerRow<<3, pDest->Rows);
	
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

/**
 * Writes one-time texts
 * Should be used very carefully, as text assembling is time-consuming
 * If text is going to be redrawn in game loop, its bitmap buffer should
 * be stored and used for redraw
 */
void fontDrawStr(struct BitMap *pDest, tFont *pFont, UWORD uwX, UWORD uwY, char *szText, UBYTE ubColor, UBYTE ubFlags) {
	tTextBitMap *pTextBitMap = fontCreateTextBitMap(pFont, szText);
	fontDrawTextBitMap(pDest, pTextBitMap, uwX, uwY, ubColor, ubFlags);
	fontDestroyTextBitMap(pTextBitMap);
}