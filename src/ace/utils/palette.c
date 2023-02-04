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
		logWrite("Color count: %u\n", ubPaletteLength);
		fileRead(pFile, pPalette, sizeof(UWORD) * MIN(ubPaletteLength, ubMaxLength));
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

void paletteDump(UWORD *pPalette, UBYTE ubColorCnt, char *szPath) {
	UBYTE fubLastColor = ubColorCnt -1;
	UBYTE fubBpp = 0;
	while(fubLastColor) {
		fubLastColor >>= 1;
		++fubBpp;
	}
	tBitMap *pBm = bitmapCreate((1+8)*ubColorCnt + 1, 10, fubBpp, BMF_CLEAR);
	for(UBYTE i = 0; i <= ubColorCnt; ++i) {
		blitRect(pBm, 1+(8+1)*i, 1, 8, 8, i);
	}
	bitmapSaveBmp(pBm, pPalette, szPath);
	bitmapDestroy(pBm);
}
