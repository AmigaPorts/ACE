/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/palette.h>
#include <ace/managers/blit.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/file.h>

void paletteLoad(const char *szFileName, UWORD *pPalette, UBYTE ubMaxLength) {
	tFile *pFile;
	UBYTE ubPaletteLength;

	logBlockBegin(
		"paletteLoad(szFileName: '%s', pPalette: %p, ubMaxLength: %hu)",
		szFileName, pPalette, ubMaxLength
	);

	pFile = fileOpen(szFileName, "r");
	if(!pFile) {
		logWrite("ERR: File doesn't exist!\n");
		logBlockEnd("paletteLoad()");
		return;
	}
	else {
		fileRead(pFile, &ubPaletteLength, sizeof(UBYTE));
		UWORD uwColorsRead = MIN(ubPaletteLength, ubMaxLength);
		if(uwColorsRead ==255)
			uwColorsRead = 256;
		logWrite("Color count: %hhu, reading: %hhu\n", ubPaletteLength, uwColorsRead);
		if (uwColorsRead > 32)
		{
			// for(int c=0; c<ubColorsRead; ++c) {
			// 	UBYTE ubR, ubG, ubB;
			// 	fileRead(pFile, &ubR, sizeof(UBYTE));
			// 	fileRead(pFile, &ubG, sizeof(UBYTE));
			// 	fileRead(pFile, &ubB, sizeof(UBYTE));

			// 	pPalette[c] = (ubR << 16) | (ubG << 8) | ubB;
			// }
			fileRead(pFile, pPalette, sizeof(ULONG) * uwColorsRead);
		}
		else
		{
			fileRead(pFile, pPalette, sizeof(UWORD) * uwColorsRead);
		}
		fileClose(pFile);
	}

	logBlockEnd("paletteLoad()");
}

void paletteLoadFromMem(const UBYTE* pData, UWORD *pPalette, UBYTE ubMaxLength) {
	logBlockBegin(
		"paletteLoadFromMem(pPalette: %p, ubMaxLength: %hu)", pPalette, ubMaxLength
	);

	UBYTE ubPaletteLength = pData[0];
	memcpy(pPalette, &pData[1], sizeof(UWORD) * MIN(ubMaxLength, ubPaletteLength));

	logBlockEnd("paletteLoadFromMem()");
}

void paletteDim(
	UWORD *pSource, volatile UWORD *pDest, UBYTE ubColorCount, UBYTE ubLevel
) {
	for(UBYTE c = 0; c != ubColorCount; ++c) {
		pDest[c] = paletteColorDim(pSource[c],  ubLevel) ;
	}
}

void paletteDimAGA(ULONG *pSource, volatile ULONG *pDest, UBYTE ubColorCount, UBYTE ubLevel) {
	for(UWORD c = 0; c <= ubColorCount; ++c) {
		pDest[c] = paletteColorDimAGA(pSource[c],  ubLevel) ;
	}
}

UWORD paletteColorDim(UWORD uwFullColor, UBYTE ubLevel) {
	UBYTE r,g,b;

	r = (uwFullColor >> 8) & 0xF;
	g = (uwFullColor >> 4) & 0xF;
	b = (uwFullColor)      & 0xF;

	// Dim color
	r = ((r * ubLevel)/15) & 0xF;
	g = ((g * ubLevel)/15) & 0xF;
	b = ((b * ubLevel)/15) & 0xF;

	// Output
	return (r << 8) | (g << 4) | b;
}

ULONG paletteColorDimAGA(ULONG ulFullColor, UBYTE ubLevel) {
	UBYTE r,g,b;

	r = (ulFullColor >> 16) & 0xFF;
	g = (ulFullColor >> 8) & 0xFF;
	b = (ulFullColor)      & 0xFF;

	// Dim color
	r = ((r * ubLevel)/255) & 0xFF;
	g = ((g * ubLevel)/255) & 0xFF;
	b = ((b * ubLevel)/255) & 0xFF;

	// Output
	return (r << 16) | (g << 8) | b;
}

void paletteDump(UWORD *pPalette, UBYTE ubColorCnt, char *szPath) {
	UBYTE ubLastColor = ubColorCnt - 1;
	UBYTE ubBpp = 0;
	while(ubLastColor) {
		ubLastColor >>= 1;
		++ubBpp;
	}
	tBitMap *pBm = bitmapCreate(
		CEIL_TO_FACTOR((1+8)*ubColorCnt + 1, 16), 10, ubBpp, BMF_CLEAR
	);
	for(UBYTE i = 0; i <= ubColorCnt; ++i) {
		blitRect(pBm, 1+(8+1)*i, 1, 8, 8, i);
	}
	bitmapSaveBmp(pBm, pPalette, szPath);
	bitmapDestroy(pBm);
}
