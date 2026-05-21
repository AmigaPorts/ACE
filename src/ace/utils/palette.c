/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/palette.h>
#include <ace/macros.h>
#include <ace/managers/blit.h>
#include <ace/managers/log.h>
#include <ace/managers/memory.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/disk_file.h>
#include <ace/utils/endian.h>

/** .plt v2 first-byte values (wire format; not part of public API). */
enum { PLT_V2_ECS = 0, PLT_V2_AGA = 1 };

void paletteLoadFromPath(const char *szPath, UWORD *pPalette, UWORD uwMaxLength) {
	return paletteLoadFromFd(diskFileOpen(szPath, DISK_FILE_MODE_READ, 1), pPalette, uwMaxLength);
}

void paletteLoadFromFd(tFile *pFile, UWORD *pPalette, UWORD uwMaxLength) {
	logBlockBegin(
		"paletteLoadFromFd(pFile: %p, pPalette: %p, uwMaxLength: %hu)",
		pFile, pPalette, uwMaxLength
	);

	if(!pFile) {
		logWrite("ERR: Null file handle\n");
		logBlockEnd("paletteLoadFromFd()");
		return;
	}

	UBYTE ubFirst;
	fileRead(pFile, &ubFirst, sizeof(UBYTE));

	if(ubFirst > 1) {
		logWrite(
			"ERR: Legacy .plt (v1) is not supported; re-export with palette_conv to v2.\n"
		);
		fileClose(pFile);
		logBlockEnd("paletteLoadFromFd()");
		return;
	}

	UWORD uwNumInFile;
	fileRead(pFile, &uwNumInFile, sizeof(UWORD));
	uwNumInFile = endianBig16(uwNumInFile);
	UWORD uwColorsRead = MIN(uwNumInFile, uwMaxLength);

	logWrite(
		".plt v2 mode %hhu, file colors: %hu, reading: %hu\n",
		ubFirst, uwNumInFile, uwColorsRead
	);

	if(ubFirst == PLT_V2_ECS) {
		fileRead(pFile, pPalette, sizeof(UWORD) * uwColorsRead);
	}
	else {
		fileRead(pFile, pPalette, sizeof(ULONG) * uwColorsRead);
	}

	fileClose(pFile);

	logBlockEnd("paletteLoadFromFd()");
}

/**
 * Writes ECS/OCS v2 .plt (e.g. from in-game tooling or external conversion pipelines
 * such as the Lands of Lore asset converter).
 */
void paletteSaveOcs(const UWORD *pPalette, UWORD uwColorCnt, char *szPath) {
	logBlockBegin(
		"paletteSaveOcs(pPalette: %p, uwColorCnt: %hu, szPath: '%s')",
		pPalette, uwColorCnt, szPath
	);

	tFile *pFile = diskFileOpen(szPath, DISK_FILE_MODE_WRITE, 1);
	if(!pFile) {
		logWrite("ERR: Can't write file\n");
		logBlockEnd("paletteSaveOcs()");
		return;
	}

	UBYTE ubSentinel = PLT_V2_ECS;

	fileWrite(pFile, &ubSentinel, sizeof(UBYTE));
	{
		UWORD uwWire = endianBig16(uwColorCnt);
		fileWrite(pFile, &uwWire, sizeof(UWORD));
	}
	fileWrite(pFile, pPalette, sizeof(UWORD) * uwColorCnt);
	fileClose(pFile);

	logBlockEnd("paletteSaveOcs()");
}

#ifdef ACE_USE_AGA_FEATURES
void paletteSaveAga(const ULONG *pPalette, UWORD uwColorCnt, char *szPath) {
	logBlockBegin(
		"paletteSaveAga(pPalette: %p, uwColorCnt: %hu, szPath: '%s')",
		pPalette, uwColorCnt, szPath
	);

	tFile *pFile = diskFileOpen(szPath, DISK_FILE_MODE_WRITE, 1);
	if(!pFile) {
		logWrite("ERR: Can't write file\n");
		logBlockEnd("paletteSaveAga()");
		return;
	}

	UBYTE ubSentinel = PLT_V2_AGA;

	fileWrite(pFile, &ubSentinel, sizeof(UBYTE));
	{
		UWORD uwWire = endianBig16(uwColorCnt);
		fileWrite(pFile, &uwWire, sizeof(UWORD));
	}

	for(UWORD i = 0; i < uwColorCnt; ++i) {
		ULONG ul = pPalette[i];
		UBYTE ubA = 0;
		UBYTE ubR = (ul >> 16) & 0xFF;
		UBYTE ubG = (ul >> 8) & 0xFF;
		UBYTE ubB = ul & 0xFF;

		fileWrite(pFile, &ubA, sizeof(UBYTE));
		fileWrite(pFile, &ubR, sizeof(UBYTE));
		fileWrite(pFile, &ubG, sizeof(UBYTE));
		fileWrite(pFile, &ubB, sizeof(UBYTE));
	}

	fileClose(pFile);

	logBlockEnd("paletteSaveAga()");
}
#endif

void paletteDimOcs(
	UWORD *pSource, volatile UWORD *pDest, UWORD uwColorCount, UBYTE ubLevel
) {
	for(UWORD c = 0; c < uwColorCount; ++c) {
		pDest[c] = paletteColorDimOcs(pSource[c], ubLevel);
	}
}

#ifdef ACE_USE_AGA_FEATURES
void paletteDimAga(ULONG *pSource, volatile ULONG *pDest, UWORD uwColorCount, UBYTE ubLevel) {
	for(UWORD c = 0; c < uwColorCount; ++c) {
		pDest[c] = paletteColorDimAga(pSource[c], ubLevel);
	}
}
#endif

UWORD paletteColorDimOcs(UWORD uwFullColor, UBYTE ubLevel) {
	UBYTE r, g, b;

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

#ifdef ACE_USE_AGA_FEATURES
ULONG paletteColorDimAga(ULONG ulFullColor, UBYTE ubLevel) {
	UBYTE r, g, b;

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
#endif

UWORD paletteColorMixOcs(
	UWORD uwColorPrimary, UWORD uwColorSecondary, UBYTE ubLevel
) {
	UBYTE r1, g1, b1;
	UBYTE r2, g2, b2;

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

#ifdef ACE_USE_AGA_FEATURES
ULONG paletteColorMixAga(ULONG ulColorPrimary, ULONG ulColorSecondary, UBYTE ubLevel) {
	ULONG r1, g1, b1;
	ULONG r2, g2, b2;

	r1 = (ulColorPrimary >> 16) & 0xFF;
	g1 = (ulColorPrimary >> 8) & 0xFF;
	b1 = (ulColorPrimary) & 0xFF;
	r2 = (ulColorSecondary >> 16) & 0xFF;
	g2 = (ulColorSecondary >> 8) & 0xFF;
	b2 = (ulColorSecondary) & 0xFF;

	r1 = ((r1 * ubLevel + (r2 * (255 - ubLevel)))/255) & 0xFF;
	g1 = ((g1 * ubLevel + (g2 * (255 - ubLevel)))/255) & 0xFF;
	b1 = ((b1 * ubLevel + (b2 * (255 - ubLevel)))/255) & 0xFF;

	return (r1 << 16) | (g1 << 8) | b1;
}

void paletteDumpAga(ULONG *pPalette, UWORD uwColorCnt, char *szPath) {
	UWORD uwLastColor = uwColorCnt - 1;
	UBYTE ubBpp = 0;

	while(uwLastColor) {
		uwLastColor >>= 1;
		++ubBpp;
	}

	tBitMap *pBm = bitmapCreate(
		CEIL_TO_FACTOR((1+8)*uwColorCnt + 1, 16), 10, ubBpp, BMF_CLEAR
	);

	UWORD *pUwTmp = memAllocFast(sizeof(UWORD) * uwColorCnt);

	for(UWORD i = 0; i < uwColorCnt; ++i) {
		ULONG ul = pPalette[i];

		UBYTE r = (ul >> 16) & 0xFF;
		UBYTE g = (ul >> 8) & 0xFF;
		UBYTE b = ul & 0xFF;

		pUwTmp[i] =
			(((UWORD)(r >> 4)) << 8) |
			(((UWORD)(g >> 4)) << 4) |
			((UWORD)(b >> 4));
	}

	for(UWORD i = 0; i < uwColorCnt; ++i) {
		blitRect(pBm, 1+(8+1)*i, 1, 8, 8, i);
	}

	bitmapSaveBmp(pBm, pUwTmp, szPath);
	memFree(pUwTmp, sizeof(UWORD) * uwColorCnt);
	bitmapDestroy(pBm);
}
#endif

void paletteDumpOcs(UWORD *pPalette, UWORD uwColorCnt, char *szPath) {
	UWORD uwLastColor = uwColorCnt - 1;
	UBYTE ubBpp = 0;
	while(uwLastColor) {
		uwLastColor >>= 1;
		++ubBpp;
	}
	tBitMap *pBm = bitmapCreate(
		CEIL_TO_FACTOR((1+8)*uwColorCnt + 1, 16), 10, ubBpp, BMF_CLEAR
	);
	for(UWORD i = 0; i < uwColorCnt; ++i) {
		blitRect(pBm, 1+(8+1)*i, 1, 8, 8, i);
	}
	bitmapSaveBmp(pBm, pPalette, szPath);
	bitmapDestroy(pBm);
}
