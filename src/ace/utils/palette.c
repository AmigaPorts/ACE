/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/palette.h>
#include <ace/managers/blit.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/file.h>

void paletteLoad(char *szFileName, UWORD *pPalette, UBYTE ubMaxLength) {
	tFile *pFile;
	UBYTE ubPaletteLength;

	logBlockBegin(
		"paletteLoad(szFileName: '%s', pPalette: %p, ubMaxLength: %hu)",
		szFileName, pPalette, ubMaxLength
	);

	pFile = fileOpen(szFileName, "r");
	fileRead(pFile, &ubPaletteLength, sizeof(UBYTE));
	logWrite(" Color count: %u\n", ubPaletteLength);
	if(ubPaletteLength > ubMaxLength) {
		ubPaletteLength = ubMaxLength;
	}
	fileRead(pFile, pPalette, sizeof(UWORD) * ubPaletteLength);
	fileClose(pFile);

	logBlockEnd("paletteLoad()");
}

void paletteLoadFromMem(const UBYTE* pData, UWORD *pPalette, UBYTE ubMaxLength) {
	logBlockBegin(
		"paletteLoadFromMem(pPalette: %p, ubMaxLength: %hu)", pPalette, ubMaxLength
	);

	UBYTE ubPaletteLength = pData[0];
	memcpy(pPalette, &pData[1], MIN( sizeof(UWORD) * ubMaxLength, sizeof(UWORD) * ubPaletteLength));

	logBlockEnd("paletteLoadFromMem()");
}

void paletteDim(UWORD *pSource, UWORD *pDest, UBYTE ubColorCount, UBYTE ubLevel) {
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

void paletteDump(UWORD *pPalette, FUBYTE fubColorCnt, char *szPath) {
	FUBYTE fubLastColor = fubColorCnt -1;
	FUBYTE fubBpp = 0;
	while(fubLastColor) {
		fubLastColor >>= 1;
		++fubBpp;
	}
	tBitMap *pBm = bitmapCreate((1+8)*fubColorCnt + 1, 10, fubBpp, BMF_CLEAR);
	for(FUBYTE i = 0; i <= fubColorCnt; ++i) {
		blitRect(pBm, 1+(8+1)*i, 1, 8, 8, i);
	}
	bitmapSaveBmp(pBm, pPalette, szPath);
	bitmapDestroy(pBm);
}
