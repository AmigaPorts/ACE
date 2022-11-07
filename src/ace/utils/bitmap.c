/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/bitmap.h>
#include <ace/managers/blit.h>
#include <ace/managers/log.h>
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/utils/endian.h>
#include <ace/utils/chunky.h>
#include <ace/utils/custom.h>

/* Globals */

/* Functions */

tBitMap *bitmapCreate(
	UWORD uwWidth, UWORD uwHeight, UBYTE ubDepth, UBYTE ubFlags
) {
#ifdef AMIGA
	tBitMap *pBitMap;
	UBYTE i;

	systemUse();
	logBlockBegin(
		"bitmapCreate(uwWidth: %hu, uwHeight: %hu, ubDepth: %hhu, ubFlags: %hhu)",
		uwWidth, uwHeight, ubDepth, ubFlags
	);

	if(uwWidth == 0 || uwHeight == 0) {
		logWrite("ERR: invalid bitmap dimensions\n");
		return 0;
	}

	if((uwWidth & 0xF) != 0) {
		// Needed for blitter!
		logWrite("ERR: bitmap width is not multiple of 16\n");
		return 0;
	}

	pBitMap = (tBitMap*) memAllocFastClear(sizeof(tBitMap));
	logWrite("addr: %p\n", pBitMap);

	pBitMap->BytesPerRow = uwWidth / 8;
	pBitMap->Rows = uwHeight;
	pBitMap->Flags = 0;
	pBitMap->Depth = ubDepth;

	if(ubFlags & BMF_INTERLEAVED) {
		pBitMap->Flags |= BMF_INTERLEAVED;
		UWORD uwRealWidth;
		uwRealWidth = pBitMap->BytesPerRow;
		pBitMap->BytesPerRow *= ubDepth;

		pBitMap->Planes[0] = (PLANEPTR) memAlloc(
			pBitMap->BytesPerRow*uwHeight,
			(ubFlags & BMF_FASTMEM) ? MEMF_ANY : MEMF_CHIP
		);
		if(!pBitMap->Planes[0]) {
			logWrite("ERR: Can't alloc interleaved bitplanes\n");
			goto fail;
		}
		for(i = 1; i != ubDepth; ++i) {
			pBitMap->Planes[i] = pBitMap->Planes[i-1] + uwRealWidth;
		}

		if (ubFlags & BMF_CLEAR) {
			memset(pBitMap->Planes[0], 0, pBitMap->Rows * pBitMap->BytesPerRow);
		}
	}
	else if(ubFlags & BMF_CONTIGUOUS) {
		pBitMap->Flags |= BMF_CONTIGUOUS;
		ULONG ulPlaneSize = pBitMap->BytesPerRow * uwHeight;
		pBitMap->Planes[0] = (PLANEPTR) memAllocChip(ulPlaneSize * ubDepth);
		if(!pBitMap->Planes[0]) {
				logWrite("ERR: Can't alloc contiguous bitplanes\n");
				goto fail;
		}
		for(i = 1; i < ubDepth; ++i) {
				pBitMap->Planes[i] = &pBitMap->Planes[i - 1][ulPlaneSize];
		}
		if (ubFlags & BMF_CLEAR) {
				memset(pBitMap->Planes[0], 0, ulPlaneSize * ubDepth);
		}
	}
	else {
		for(i = ubDepth; i--;) {
			pBitMap->Planes[i] = (PLANEPTR) memAllocChip(pBitMap->BytesPerRow * uwHeight);
			if(!pBitMap->Planes[i]) {
				logWrite("ERR: Can't alloc bitplane %hu/%hu\n", ubDepth - i + 1,ubDepth);
				while(++i != ubDepth) {
					memFree(pBitMap->Planes[i], pBitMap->BytesPerRow*uwHeight);
				}
				goto fail;
			}
			if (ubFlags & BMF_CLEAR) {
				memset(pBitMap->Planes[i], 0, pBitMap->Rows * pBitMap->BytesPerRow);
			}
		}
	}

	logBlockEnd("bitmapCreate()");
	systemUnuse();
	return pBitMap;

fail:
	if(pBitMap) {
		memFree(pBitMap, sizeof(tBitMap));
	}
	logBlockEnd("bitmapCreate()");
	systemUnuse();
	return 0;

#else
	return 0;
#endif // AMIGA
}

void bitmapLoadFromFile(
	tBitMap *pBitMap, char *szFilePath, UWORD uwStartX, UWORD uwStartY
) {
	UWORD uwSrcWidth, uwDstWidth, uwSrcHeight;
	UBYTE ubSrcFlags, ubSrcBpp, ubSrcVersion;
	UWORD y;
	UWORD uwWidth;
	UBYTE ubPlane;

	systemUse();
	logBlockBegin(
		"bitmapLoadFromFile(pBitMap: %p, szFilePath: '%s', uwStartX: %u, uwStartY: %u)",
		pBitMap, szFilePath, uwStartX, uwStartY
	);
	// Open source bitmap
	tFile *pFile = fileOpen(szFilePath, "r");
	if(!pFile) {
		logWrite("ERR: File does not exist\n");
		logBlockEnd("bitmapLoadFromFile()");
		systemUnuse();
		return;
	}

	// Read header
	fileRead(pFile, &uwSrcWidth, sizeof(UWORD));
	fileRead(pFile, &uwSrcHeight, sizeof(UWORD));
	fileRead(pFile, &ubSrcBpp, sizeof(UBYTE));
	fileRead(pFile, &ubSrcVersion, sizeof(UBYTE));
	fileRead(pFile, &ubSrcFlags, sizeof(UBYTE));
	fileSeek(pFile, 2*sizeof(UBYTE), FILE_SEEK_CURRENT); // Skip unused 2 bytes
	if(ubSrcVersion != 0) {
		fileClose(pFile);
		logWrite("ERR: Unknown file version: %hu\n", ubSrcVersion);
		logBlockEnd("bitmapLoadFromFile()");
		systemUnuse();
		return;
	}
	logWrite("Source dimensions: %ux%u\n", uwSrcWidth, uwSrcHeight);

	// Interleaved check
	if(!!(ubSrcFlags & BITMAP_INTERLEAVED) != bitmapIsInterleaved(pBitMap)) {
		logWrite(
			"ERR: Interleaved flag conflict (file: %d, bm: %hhu)\n",
			!!(ubSrcFlags & BITMAP_INTERLEAVED), bitmapIsInterleaved(pBitMap)
		);
		fileClose(pFile);
		logBlockEnd("bitmapLoadFromFile()");
		systemUnuse();
		return;
	}

	// Depth check
	if(ubSrcBpp > pBitMap->Depth) {
		logWrite(
			"ERR: Source has greater BPP than destination: %hu > %hu\n",
			ubSrcBpp, pBitMap->Depth
		);
		fileClose(pFile);
		logBlockEnd("bitmapLoadFromFile()");
		systemUnuse();
		return;
	}

	// Check bitmap dimensions
	uwDstWidth = bitmapGetByteWidth(pBitMap) << 3;
	if(uwStartX + uwSrcWidth > uwDstWidth || uwStartY + uwSrcHeight > (pBitMap->Rows)) {
		logWrite(
			"ERR: Source doesn't fit on dest: %ux%u @%u,%u > %ux%u\n",
			uwSrcWidth, uwSrcHeight,
			uwStartX, uwStartY,
			uwDstWidth, pBitMap->Rows
		);
		fileClose(pFile);
		logBlockEnd("bitmapLoadFromFile()");
		systemUnuse();
		return;
	}

	// Read data
	uwWidth = bitmapGetByteWidth(pBitMap);
	if(bitmapIsInterleaved(pBitMap)) {
		for(y = 0; y != uwSrcHeight; ++y) {
			for(ubPlane = 0; ubPlane != pBitMap->Depth; ++ubPlane) {
				fileRead(
					pFile,
					&pBitMap->Planes[0][uwWidth*(((uwStartY + y)*pBitMap->Depth)+ubPlane)+(uwStartX>>3)],
					((uwSrcWidth+7)>>3)
				);
			}
		}
	}
	else {
		for(ubPlane = 0; ubPlane != pBitMap->Depth; ++ubPlane) {
			for(y = 0; y != uwSrcHeight; ++y) {
				fileRead(
					pFile,
					&pBitMap->Planes[ubPlane][uwWidth*(uwStartY+y) + (uwStartX>>3)],
					((uwSrcWidth+7)>>3)
				);
			}
		}
	}
	fileClose(pFile);
	logBlockEnd("bitmapLoadFromFile()");
	systemUnuse();
}

void bitmapLoadFromMem(
	tBitMap *pBitMap, const UBYTE* pData, UWORD uwStartX, UWORD uwStartY
) {
	UWORD uwSrcWidth, uwDstWidth, uwSrcHeight;
	UBYTE ubSrcFlags, ubSrcBpp, ubSrcVersion;
	UWORD y;
	UWORD uwWidth;
	UBYTE ubPlane;
	ULONG ulCurByte = 0;

	logBlockBegin(
		"bitmapLoadFromMem(pBitMap: %p, uwStartX: %u, uwStartY: %u)",
		pBitMap, uwStartX, uwStartY
	);


	// Read header
	memcpy(&uwSrcWidth,&pData[ulCurByte],sizeof(UWORD));
	ulCurByte+=sizeof(UWORD);

	memcpy(&uwSrcHeight,&pData[ulCurByte],sizeof(UWORD));
	ulCurByte+=sizeof(UWORD);

	memcpy(&ubSrcBpp,&pData[ulCurByte],sizeof(UBYTE));
	ulCurByte+=sizeof(UBYTE);

	memcpy(&ubSrcVersion,&pData[ulCurByte],sizeof(UBYTE));
	ulCurByte+=sizeof(UBYTE);

	memcpy(&ubSrcFlags,&pData[ulCurByte],sizeof(UBYTE));
	ulCurByte+=sizeof(UBYTE);

	ulCurByte+=2*sizeof(UBYTE);

	if(ubSrcVersion != 0) {
		logWrite("ERR: Unknown file version: %hu\n", ubSrcVersion);
		logBlockEnd("bitmapLoadFromMem()");
		return;
	}
	logWrite("Source dimensions: %ux%u\n", uwSrcWidth, uwSrcHeight);

	// Interleaved check
	if(!!(ubSrcFlags & BITMAP_INTERLEAVED) != bitmapIsInterleaved(pBitMap)) {
		logWrite("ERR: Interleaved flag conflict\n");
		logBlockEnd("bitmapLoadFromMem()");
		return;
	}

	// Depth check
	if(ubSrcBpp > pBitMap->Depth) {
		logWrite(
			"ERR: Source has greater BPP than destination: %hu > %hu\n",
			ubSrcBpp, pBitMap->Depth
		);
		logBlockEnd("bitmapLoadFromMem()");
		return;
	}

	// Check bitmap dimensions
	uwDstWidth = bitmapGetByteWidth(pBitMap) << 3;
	if(uwStartX + uwSrcWidth > uwDstWidth || uwStartY + uwSrcHeight > (pBitMap->Rows)) {
		logWrite(
			"ERR: Source doesn't fit on dest: %ux%u @%u,%u > %ux%u\n",
			uwSrcWidth, uwSrcHeight,
			uwStartX, uwStartY,
			uwDstWidth, pBitMap->Rows
		);
		logBlockEnd("bitmapLoadFromMem()");
		return;
	}

	// Read data
	uwWidth = bitmapGetByteWidth(pBitMap);
	if(bitmapIsInterleaved(pBitMap)) {
		for(y = 0; y != uwSrcHeight; ++y) {
			for(ubPlane = 0; ubPlane != pBitMap->Depth; ++ubPlane) {
				memcpy(&pBitMap->Planes[0][uwWidth*(((uwStartY + y)*pBitMap->Depth)+ubPlane)+(uwStartX>>3)],&pData[ulCurByte],((uwSrcWidth+7)>>3));
				ulCurByte+=((uwSrcWidth+7)>>3);
			}
		}
	}
	else {
		for(ubPlane = 0; ubPlane != pBitMap->Depth; ++ubPlane) {
			for(y = 0; y != uwSrcHeight; ++y) {
				memcpy(&pBitMap->Planes[ubPlane][uwWidth*(uwStartY+y) + (uwStartX>>3)],&pData[ulCurByte],((uwSrcWidth+7)>>3));
				ulCurByte+=((uwSrcWidth+7)>>3);
			}
		}
	}
	logBlockEnd("bitmapLoadFromMem()");
}

tBitMap *bitmapCreateFromFile(const char *szFilePath, UBYTE isFast) {
	tBitMap *pBitMap;
	tFile *pFile;
	UWORD uwWidth, uwHeight;  // Image dimensions
	UBYTE ubVersion, ubFlags; // Format version & flags
	UBYTE ubPlaneCount;       // Bitplane count
	UBYTE i;

	logBlockBegin("bitmapCreateFromFile(szFilePath: '%s')", szFilePath);
	pFile = fileOpen(szFilePath, "r");
	if(!pFile) {
		fileClose(pFile);
		logWrite("ERR: File does not exist\n");
		logBlockEnd("bitmapCreateFromFile()");
		return 0;
	}

	// Read header
	fileRead(pFile, &uwWidth, sizeof(UWORD));
	fileRead(pFile, &uwHeight, sizeof(UWORD));
	fileRead(pFile, &ubPlaneCount, sizeof(UBYTE));
	fileRead(pFile, &ubVersion, sizeof(UBYTE));
	fileRead(pFile, &ubFlags, sizeof(UBYTE));
	fileSeek(pFile, 2 * sizeof(UBYTE), SEEK_CUR); // Skip unused 2 bytes
	if(ubVersion != 0) {
		fileClose(pFile);
		logWrite("ERR: Unknown file version: %hu\n", ubVersion);
		logBlockEnd("bitmapCreateFromFile()");
		return 0;
	}

	// Init bitmap
	UBYTE ubBitmapFlags = 0;
	if(isFast) {
		ubBitmapFlags |= BMF_FASTMEM;
	}
	if(ubFlags & BITMAP_INTERLEAVED) {
		pBitMap = bitmapCreate(
			uwWidth, uwHeight, ubPlaneCount, ubBitmapFlags | BMF_INTERLEAVED
		);
		fileRead(pFile, pBitMap->Planes[0], (uwWidth >> 3) * uwHeight * ubPlaneCount);
	}
	else {
		pBitMap = bitmapCreate(uwWidth, uwHeight, ubPlaneCount, ubBitmapFlags);
		for (i = 0; i != ubPlaneCount; ++i) {
			fileRead(pFile, pBitMap->Planes[i], (uwWidth >> 3) * uwHeight);
		}
	}
	fileClose(pFile);

	logWrite(
		"Dimensions: %ux%u@%uBPP, version: %hu, flags: %hu\n",
		uwWidth, uwHeight, ubPlaneCount, ubVersion, ubFlags
	);
	logBlockEnd("bitmapCreateFromFile()");
	return pBitMap;
}

void bitmapDestroy(tBitMap *pBitMap) {
	logBlockBegin("bitmapDestroy(pBitMap: %p)", pBitMap);
	if(pBitMap) {
#ifdef AMIGA
		blitWait();
#endif
		systemUse();
		if(bitmapIsInterleaved(pBitMap)) {
			memFree(pBitMap->Planes[0], pBitMap->BytesPerRow * pBitMap->Rows);
		}
		else if(pBitMap->Flags & BMF_CONTIGUOUS) {
			memFree(pBitMap->Planes[0], pBitMap->BytesPerRow * pBitMap->Rows * pBitMap->Depth);
		}
		else {
			for(UBYTE i = pBitMap->Depth; i--;) {
				memFree(pBitMap->Planes[i], pBitMap->BytesPerRow * pBitMap->Rows);
			}
		}
		memFree(pBitMap, sizeof(tBitMap));
		systemUnuse();
	}
	logBlockEnd("bitmapDestroy()");
}

UBYTE bitmapIsInterleaved(const tBitMap *pBitMap) {
	return (pBitMap->Depth > 1) && (pBitMap->Flags & BMF_INTERLEAVED);
}

UBYTE bitmapIsChip(const tBitMap *pBitMap) {
	return memType(pBitMap->Planes[0]) == MEMF_CHIP;
}

void bitmapDump(const tBitMap *pBitMap) {
	UBYTE i;

	logBlockBegin("bitmapDump(pBitMap: %p)", pBitMap);

	logWrite(
		"BytesPerRow: %u, Rows: %u, Flags: %hu, Depth: %hu, pad: %u\n",
		pBitMap->BytesPerRow, pBitMap->Rows, pBitMap->Flags,
		pBitMap->Depth, pBitMap->pad
	);
	// since Planes is always 8-long, dump all its entries
	for(i = 0; i != pBitMap->Depth; ++i) {
		logWrite("Bitplane %hu addr: %p\n", i, pBitMap->Planes[i]);
	}

	logBlockEnd("bitmapDump()");
}

void bitmapSave(const tBitMap *pBitMap, const char *szPath) {
	systemUse();
	logBlockBegin("bitmapSave(pBitMap: %p, szPath: '%s')", pBitMap, szPath);

	tFile *pFile = fileOpen(szPath, "wb");
	if(!pFile) {
		logWrite("ERR: Couldn't save bitmap at '%s'\n", szPath);
		logBlockEnd("bitmapSave()");
		return;
	}

	// TODO check free space on disk

	// Header
	UWORD uwWidth = bitmapGetByteWidth(pBitMap) << 3;
	UWORD uwHeight = pBitMap->Rows;
	UBYTE ubPlaneCount = pBitMap->Depth;
	UBYTE ubVersion = 0;
	UBYTE ubFlags = bitmapIsInterleaved(pBitMap) ? BITMAP_INTERLEAVED : 0;
	UWORD uwUnused = 0;
	fileWrite(pFile, &uwWidth, sizeof(UWORD));
	fileWrite(pFile, &uwHeight, sizeof(UWORD));
	fileWrite(pFile, &ubPlaneCount, sizeof(UBYTE));
	fileWrite(pFile, &ubVersion, sizeof(UBYTE));
	fileWrite(pFile, &ubFlags, sizeof(UBYTE));
	fileWrite(pFile, &uwUnused, sizeof(UWORD)); // Unused 2 bytes

	// Data
	if(ubFlags & BITMAP_INTERLEAVED) {
		fileWrite(pFile, pBitMap->Planes[0], (uwWidth >> 3) * uwHeight * ubPlaneCount);
	}
	else {
		for (FUBYTE i = 0; i != ubPlaneCount; ++i) {
			fileWrite(pFile, pBitMap->Planes[i], (uwWidth >> 3) * uwHeight);
		}
	}

	fileClose(pFile);
	logBlockEnd("bitmapSave()");
	systemUnuse();
}

void bitmapSaveBmp(
	const tBitMap *pBitMap, const UWORD *pPalette, const char *szFilePath
) {
	// TODO: EHB support

	systemUse();
	UWORD uwWidth = bitmapGetByteWidth(pBitMap) << 3;
	tFile *pOut = fileOpen(szFilePath, "w");

	// BMP header
	fileWrite(pOut, "BM", 2);

	ULONG ulOut = endianIntel32((pBitMap->BytesPerRow<<3) * pBitMap->Rows + 14+40+256*4);
	fileWrite(pOut, &ulOut, sizeof(ULONG)); // BMP file size

	ulOut = 0;
	fileWrite(pOut, &ulOut, sizeof(ULONG)); // Reserved

	ulOut = endianIntel32(14+40+256*4);
	fileWrite(pOut, &ulOut, sizeof(ULONG)); // Bitmap data starting addr


	// Bitmap info header
	ulOut = endianIntel32(40);
	fileWrite(pOut, &ulOut, sizeof(ULONG)); // Core header size

	ulOut = endianIntel32(uwWidth);
	fileWrite(pOut, &ulOut, sizeof(ULONG)); // Image width

	ulOut = endianIntel32(pBitMap->Rows);
	fileWrite(pOut, &ulOut, sizeof(ULONG)); // Image height

	UWORD uwOut = endianIntel16(1);
	fileWrite(pOut, &uwOut, sizeof(UWORD)); // Color plane count

	uwOut = endianIntel16(8);
	fileWrite(pOut, &uwOut, sizeof(UWORD)); // Image BPP - 8bit indexed

	ulOut = endianIntel32(0);
	fileWrite(pOut, &ulOut, sizeof(ULONG)); // Compression method - none

	ulOut = endianIntel32(uwWidth * pBitMap->Rows);
	fileWrite(pOut, &ulOut, sizeof(ULONG)); // Image size

	ulOut = endianIntel32(100);
	fileWrite(pOut, &ulOut, sizeof(ULONG)); // Horizontal resolution - px/m

	ulOut = endianIntel32(100);
	fileWrite(pOut, &ulOut, sizeof(ULONG)); // Vertical resolution - px/m

	ulOut = endianIntel32(0);
	fileWrite(pOut, &ulOut, sizeof(ULONG)); // Palette length

	ulOut = endianIntel32(0);
	fileWrite(pOut, &ulOut, sizeof(ULONG)); // Number of important colors - all

	// Global palette
	UWORD c;
	for(c = 0; c != (1 << pBitMap->Depth); ++c) {
		UBYTE ubOut = pPalette[c] & 0xF;
		ubOut |= ubOut << 4;
		fileWrite(pOut, &ubOut, sizeof(UBYTE)); // B

		ubOut = (pPalette[c] >> 4) & 0xF;
		ubOut |= ubOut << 4;
		fileWrite(pOut, &ubOut, sizeof(UBYTE)); // G

		ubOut = pPalette[c] >> 8;
		ubOut |= ubOut << 4;
		fileWrite(pOut, &ubOut, sizeof(UBYTE)); // R

		ubOut = 0;
		fileWrite(pOut, &ubOut, sizeof(UBYTE)); // 0
	}
	// Dummy fill up to 255 indices
	ulOut = 0;
	while(c < 256) {
		fileWrite(pOut, &ulOut, sizeof(ULONG));
		++c;
	}

	// Image data
	UBYTE pIndicesChunk[16];
	for(UWORD uwY = pBitMap->Rows; uwY--;) {
		UWORD uwX;
		for(uwX = 0; uwX < uwWidth; uwX += 16) {
			chunkyFromPlanar16(pBitMap, uwX, uwY, pIndicesChunk);
			fileWrite(pOut, pIndicesChunk, 16*sizeof(UBYTE));
		}
		UBYTE ubOut = 0;
		while(uwX & 0x3) {// 4-byte row padding
			fileWrite(pOut, &ubOut, sizeof(UBYTE));
			++uwX;
		}
	}

	fileClose(pOut);
	systemUnuse();
}

UWORD bitmapGetByteWidth(const tBitMap *pBitMap) {
	if(bitmapIsInterleaved(pBitMap)) {
		return ((ULONG)pBitMap->Planes[1] - (ULONG)pBitMap->Planes[0]);
	}
	return pBitMap->BytesPerRow;
}
