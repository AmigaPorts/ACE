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
	else {
		// Read first byte - could be header or palette count for old format
		UBYTE ubFirstByte;
		fileRead(pFile, &ubFirstByte, sizeof(UBYTE));
		
		UBYTE ubIsAGA = 0;
		UWORD uwPaletteLength;
		
		// Check if this is the new format with header
		if (ubFirstByte == 0 || ubFirstByte == 1) {
			// New format: first byte is AGA flag, followed by 2-byte palette count
			ubIsAGA = ubFirstByte;
			fileRead(pFile, &uwPaletteLength, sizeof(UWORD));
			logWrite("New format - AGA: %s, color count: %hu\n", 
				ubIsAGA ? "YES" : "NO", uwPaletteLength);
		} else {
			// Old format: first byte is palette count
			uwPaletteLength = ubFirstByte;
			// Use old heuristic for AGA detection
			ubIsAGA = (uwPaletteLength > 32) ? 1 : 0;
			logWrite("Legacy format - color count: %hu, detected AGA: %s\n", 
				uwPaletteLength, ubIsAGA ? "YES" : "NO");
		}
		
		// Handle 256 color case for old format compatibility
		UWORD uwColorsRead = MIN(uwPaletteLength, ubMaxLength);
		if(uwColorsRead == 255 && !ubFirstByte) // Only for old format
			uwColorsRead = 256;
		
		logWrite("Reading %hu colors\n", uwColorsRead);
		
		if (ubIsAGA) {
			// AGA format: 4 bytes per color (alpha, R, G, B)
			for(UWORD c = 0; c <= uwColorsRead; ++c) {
				UBYTE ubA, ubR, ubG, ubB;
				fileRead(pFile, &ubA, sizeof(UBYTE)); // Skip alpha
				fileRead(pFile, &ubR, sizeof(UBYTE));
				fileRead(pFile, &ubG, sizeof(UBYTE));
				fileRead(pFile, &ubB, sizeof(UBYTE));
				
				#ifdef ACE_USE_AGA_FEATURES
				// Store as 32-bit AGA color
				((ULONG*)pPalette)[c] = (ubR << 16) | (ubG << 8) | ubB;
				#else
				// Convert to OCS format for non-AGA builds
				pPalette[c] = ((ubR >> 4) << 8) | ((ubG >> 4) << 4) | (ubB >> 4);
				#endif
			}
		} else {
			// OCS/ECS format: 2 bytes per color (packed RGB)
			fileRead(pFile, pPalette, sizeof(UWORD) * uwColorsRead);
		}
		fileClose(pFile);
	}

	// UBYTE ubPaletteLength;
	// fileRead(pFile, &ubPaletteLength, sizeof(UBYTE));
	// UBYTE ubColorsRead = MIN(ubPaletteLength, ubMaxLength);
	// logWrite("Color count: %hhu, reading: %hhu\n", ubPaletteLength, ubColorsRead);
	// fileRead(pFile, pPalette, sizeof(UWORD) * ubColorsRead);
	// fileClose(pFile);

	logBlockEnd("paletteLoadFromFd()");
}

void paletteDim(
	UWORD *pSource, volatile UWORD *pDest, UBYTE ubColorCount, UBYTE ubLevel
) {
	for(UBYTE c = 0; c != ubColorCount; ++c) {
		pDest[c] = paletteColorDim(pSource[c],  ubLevel) ;
	}
}

#ifdef ACE_USE_AGA_FEATURES
void paletteDimAGA(ULONG *pSource, volatile ULONG *pDest, UBYTE ubColorCount, UBYTE ubLevel) {
	for(UWORD c = 0; c <= ubColorCount; ++c) {
		pDest[c] = paletteColorDimAGA(pSource[c],  ubLevel) ;
	}
}
#endif

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

#ifdef ACE_USE_AGA_FEATURES
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
#endif

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

#ifdef ACE_USE_AGA_FEATURES
ULONG paletteColorMixAGA(
	ULONG ulColorPrimary, ULONG ulColorSecondary, UBYTE ubLevel
) {
	UBYTE r1,g1,b1;
	UBYTE r2,g2,b2;

	r1 = (ulColorPrimary >> 16) & 0xFF;
	g1 = (ulColorPrimary >> 8) & 0xFF;
	b1 = (ulColorPrimary)      & 0xFF;
	r2 = (ulColorSecondary >> 16) & 0xFF;
	g2 = (ulColorSecondary >> 8) & 0xFF;
	b2 = (ulColorSecondary)      & 0xFF;

	// Mix colors
	r1 = ((r1 * ubLevel + (r2 * (0xFF - ubLevel)))/255) & 0xFF;
	g1 = ((g1 * ubLevel + (g2 * (0xFF - ubLevel)))/255) & 0xFF;
	b1 = ((b1 * ubLevel + (b2 * (0xFF - ubLevel)))/255) & 0xFF;

	// Output
	return (r1 << 16) | (g1 << 8) | b1;
}
#endif

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

#ifdef ACE_USE_AGA_FEATURES
void paletteDumpAGA(ULONG *pPalette, UWORD uwColorCnt, char *szPath) {
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
#endif

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
		// Write new format: header byte (0 = OCS/ECS) + 2-byte color count + colors
		UBYTE ubHeader = 0; // OCS/ECS format
		UWORD uwColorCnt = ubColorCnt;
		
		fileWrite(pFile, &ubHeader, sizeof(UBYTE));
		fileWrite(pFile, &uwColorCnt, sizeof(UWORD));
		fileWrite(pFile, pPalette, sizeof(UWORD) * ubColorCnt);
		fileClose(pFile);
		
		logWrite("Saved %hu colors in OCS/ECS format\n", ubColorCnt);
	}

	logBlockEnd("paletteSave()");
}

