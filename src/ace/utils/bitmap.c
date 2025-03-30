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
#include <ace/utils/disk_file.h>

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
		systemUnuse();
		return 0;
	}

	if((uwWidth & 0xF) != 0) {
		// Needed for blitter!
		logWrite("ERR: bitmap width is not multiple of 16\n");
		systemUnuse();
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

void bitmapLoadFromPath(tBitMap *pBitMap, const char *szPath, UWORD uwStartX, UWORD uwStartY) {
	return bitmapLoadFromFd(pBitMap, diskFileOpen(szPath, DISK_FILE_MODE_READ, 1), uwStartX, uwStartY);
}

void bitmapLoadFromFd(
		tBitMap *pBitMap, tFile *pFile, UWORD uwStartX, UWORD uwStartY)
{
	UWORD uwSrcWidth, uwDstWidth, uwSrcHeight;
	UBYTE ubSrcFlags, ubSrcBpp, ubSrcVersion;
	UWORD y;
	UWORD uwWidth;
	UBYTE ubPlane;

	systemUse();
	logBlockBegin(
		"bitmapLoadFromFd(pBitMap: %p, pFile: %p, uwStartX: %u, uwStartY: %u)",
		pBitMap, pFile, uwStartX, uwStartY
	);

	if(!pBitMap) {
		logWrite("ERR: pBitMap is 0\n");
		systemUnuse();
		return;
	}

	// Open source bitmap
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
		logBlockEnd("bitmapLoadFromFd()");
		systemUnuse();
		return;
	}

	// Read header
	fileRead(pFile, &uwSrcWidth, sizeof(UWORD));
	fileRead(pFile, &uwSrcHeight, sizeof(UWORD));
	fileRead(pFile, &ubSrcBpp, sizeof(UBYTE));
	fileRead(pFile, &ubSrcVersion, sizeof(UBYTE));
	fileRead(pFile, &ubSrcFlags, sizeof(UBYTE));
	fileSeek(pFile, 2 * sizeof(UBYTE), FILE_SEEK_CURRENT); // Skip unused 2 bytes
	if(ubSrcVersion != 0) {
		fileClose(pFile);
		logWrite("ERR: Unknown file version: %hu\n", ubSrcVersion);
		logBlockEnd("bitmapLoadFromFd()");
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
		logBlockEnd("bitmapLoadFromFd()");
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
		logBlockEnd("bitmapLoadFromFd()");
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
		logBlockEnd("bitmapLoadFromFd()");
		systemUnuse();
		return;
	}

	// Read data
	uwWidth = bitmapGetByteWidth(pBitMap);
	UWORD uwReadBytesPerRow = (uwSrcWidth + 7) / 8;
	if(bitmapIsInterleaved(pBitMap)) {
		UWORD uwDestOffs = uwWidth * (uwStartY * pBitMap->Depth) + (uwStartX / 8);
		if(uwStartX == 0 && uwSrcWidth == uwDstWidth) {
			fileRead(
				pFile,
				&pBitMap->Planes[0][uwDestOffs],
				pBitMap->BytesPerRow * uwSrcHeight
			);
		}
		else {
			for(y = 0; y < uwSrcHeight; ++y) {
				for(ubPlane = 0; ubPlane != pBitMap->Depth; ++ubPlane) {
					fileRead(
						pFile,
						&pBitMap->Planes[0][uwDestOffs],
						uwReadBytesPerRow
					);
					uwDestOffs += uwWidth;
				}
			}
		}
	}
	else {
		for(ubPlane = 0; ubPlane != pBitMap->Depth; ++ubPlane) {
			for(y = 0; y != uwSrcHeight; ++y) {
				UWORD uwDestOffs = uwWidth * uwStartY + (uwStartX / 8);
				fileRead(
					pFile,
					&pBitMap->Planes[ubPlane][uwDestOffs],
					uwReadBytesPerRow
				);
				uwDestOffs += uwWidth;
			}
		}
	}
	fileClose(pFile);
	logBlockEnd("bitmapLoadFromFd()");
	systemUnuse();
}

tBitMap *bitmapCreateFromPath(const char *szPath, UBYTE isFast) {
	return bitmapCreateFromFd(diskFileOpen(szPath, DISK_FILE_MODE_READ, 1), isFast);
}

tBitMap *bitmapCreateFromFd(tFile *pFile, UBYTE isFast) {
	tBitMap *pBitMap;
	UWORD uwWidth, uwHeight;  // Image dimensions
	UBYTE ubVersion, ubFlags; // Format version & flags
	UBYTE ubPlaneCount;       // Bitplane count
	UBYTE i;

	systemUse();
	logBlockBegin("bitmapCreateFromFd(pFile: %p)", pFile);
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
		logBlockEnd("bitmapCreateFromFd()");
		systemUnuse();
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
		logWrite("ERR: Unknown file version: %hu\n", ubVersion);
		fileClose(pFile);
		logBlockEnd("bitmapCreateFromFd()");
		systemUnuse();
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
	logBlockEnd("bitmapCreateFromFd()");
	systemUnuse();
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
	// The check for depth is because of how bitmapGetByteWidth() works
	// if byte width were to be stored in bitmap struct, the depth check can be skipped.
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

	tFile *pFile = diskFileOpen(szPath, DISK_FILE_MODE_WRITE, 1);
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
	tFile *pOut = diskFileOpen(szFilePath, DISK_FILE_MODE_WRITE, 1);
	if(!pOut) {
		logWrite("ERR: Couldn't save bmp at '%s'\n", szFilePath);
		logBlockEnd("bitmapSaveBmp()");
		return;
	}

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
