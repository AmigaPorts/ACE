/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/palette.h>
#include <ace/managers/blit.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/disk_file.h>

void paletteLoadFromPath(const char *szPath, UWORD *pPalette, UBYTE ubMaxLength) {
	return paletteLoadFromFd(diskFileOpen(szPath, DISK_FILE_MODE_READ, 1), pPalette, ubMaxLength);
}

void paletteLoadFromFd(tFile *pFile, UWORD *pPalette, UBYTE ubMaxLength) {
	logBlockBegin(
		"paletteLoadFromFd(pFile: %p, pPalette: %p, ubMaxLength: %hu)",
		pFile, pPalette, ubMaxLength
	);

	if(!pFile) {
		logWrite("ERR: Null file handle\n");
		logBlockEnd("paletteLoadFromFd()");
		return;
	}

	UBYTE ubPaletteLength;
	fileRead(pFile, &ubPaletteLength, sizeof(UBYTE));
	UBYTE ubColorsRead = MIN(ubPaletteLength, ubMaxLength);
	logWrite("Color count: %hhu, reading: %hhu\n", ubPaletteLength, ubColorsRead);
	fileRead(pFile, pPalette, sizeof(UWORD) * ubColorsRead);
	fileClose(pFile);

	logBlockEnd("paletteLoadFromFd()");
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

UWORD paletteColorMix(
	UWORD uwColorPrimary, UWORD uwColorSecondary, UBYTE ubLevel
) {
	UBYTE r1,g1,b1;
	UBYTE r2,g2,b2;

	r1 = (uwColorPrimary >> 8) & 0xF;
	g1 = (uwColorPrimary >> 4) & 0xF;
	b1 = (uwColorPrimary)      & 0xF;
	r2 = (uwColorSecondary >> 8) & 0xF;
	g2 = (uwColorSecondary >> 4) & 0xF;
	b2 = (uwColorSecondary)      & 0xF;

	// Dim color
	r1 = ((r1 * ubLevel + (r2 * (0xF - ubLevel)))/15) & 0xF;
	g1 = ((g1 * ubLevel + (g2 * (0xF - ubLevel)))/15) & 0xF;
	b1 = ((b1 * ubLevel + (b2 * (0xF - ubLevel)))/15) & 0xF;

	// Output
	return (r1 << 8) | (g1 << 4) | b1;
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

void paletteSave(UWORD *pPalette, UBYTE ubColorCnt, char *szPath) {
	logBlockBegin(
		"paletteSave(pPalette: %p, ubColorCnt: %hu, szPath: '%s')",
		pPalette, ubColorCnt, szPath
	);

	tFile *pFile = diskFileOpen(szPath, DISK_FILE_MODE_WRITE, 1);
	if(!pFile) {
		logWrite("ERR: Can't write file\n");
		logBlockEnd("paletteSave()");
		return;
	}
	else {
		fileWrite(pFile, &ubColorCnt, sizeof(UBYTE));
		fileWrite(pFile, pPalette, sizeof(UWORD) * ubColorCnt);
		fileClose(pFile);
	}

	logBlockEnd("paletteSave()");
}
