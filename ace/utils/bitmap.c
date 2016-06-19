#include <ace/utils/bitmap.h>

/* Globals */

/* Functions */

// AllocBitMap nie dzia³a na kick 1.3
tBitMap *bitmapCreate(UWORD uwWidth, UWORD uwHeight, UBYTE ubDepth, UBYTE ubFlags) {
	tBitMap *pBitMap;
	UBYTE i;
	
	logBlockBegin("bitmapCreate(uwWidth: %u, uwHeight: %u, ubDepth: %hu, ubFlags: %hu)", uwWidth, uwHeight, ubDepth, ubFlags);
	pBitMap = (tBitMap*) memAllocFastClear(sizeof(tBitMap));
	logWrite("addr: %p\n", pBitMap);

	InitBitMap(pBitMap, ubDepth, uwWidth, uwHeight);
	
	if(ubFlags & BMF_INTERLEAVED) {
		UWORD uwRealWidth;
		uwRealWidth = pBitMap->BytesPerRow;
		pBitMap->BytesPerRow *= ubDepth;
		
		pBitMap->Planes[0] = (PLANEPTR) memAllocChip(pBitMap->BytesPerRow*uwHeight);
		if(!pBitMap->Planes[0]) {
			logWrite("ERR: Can't alloc interleaved bitplane\n");
			memFree(pBitMap, sizeof(tBitMap));
			logBlockEnd("bitmapCreate()");
			return 0;			
		}
		for(i = 1; i != ubDepth; ++i)
			pBitMap->Planes[i] = pBitMap->Planes[i-1] + uwRealWidth;
		
		if (ubFlags & BMF_CLEAR)
			BltClear(pBitMap->Planes[0], pBitMap->Rows * pBitMap->BytesPerRow, 1);
	}
	else
		for(i = ubDepth; i--;) {
			pBitMap->Planes[i] = (PLANEPTR) memAllocChip(pBitMap->BytesPerRow*uwHeight);
			if(!pBitMap->Planes[i]) {
				logWrite("ERR: Can't alloc bitplane %hu/%hu\n", ubDepth-i+1,ubDepth);
				while(i) {
					memFree(pBitMap->Planes[i], pBitMap->BytesPerRow*uwHeight);
					--i;
				}
				memFree(pBitMap, sizeof(tBitMap));
				logBlockEnd("bitmapCreate()");
				return 0;
			}
			if (ubFlags & BMF_CLEAR)
				BltClear(pBitMap->Planes[i], pBitMap->Rows * pBitMap->BytesPerRow, 1);
		}

	if (ubFlags & BMF_CLEAR)
		WaitBlit();

	logBlockEnd("bitmapCreate()");
	return pBitMap;
}

tBitMap *bitmapCreateFromFile(char *szFileName) {
	tBitMap *pBitMap;
	FILE *pFile;
	UWORD uwWidth, uwHeight;            // wymiary obrazka
	UWORD uwCopperLength, uwCopperSize; // copperlista - raczej niepotrzebne, usun¹æ póŸniej ze specyfikacji pliku
	UBYTE ubPlaneCount;                 // liczba bitplanesów
	UBYTE i;
	
	logBlockBegin("bitmapCreateFromFile(szFileName: %s)", szFileName);
	pFile = fopen(szFileName, "r");
	if(!pFile) {
		logWrite("File does not exist: %s\n", szFileName);
		logBlockEnd("bitmapCreateFromFile()");
		return 0;
	}
	logWrite("Addr: %p\n",pBitMap);
	
	// Nag³ówek
	fread(&uwWidth, 2, 1, pFile);
	fread(&uwHeight, 2, 1, pFile);
	fread(&ubPlaneCount, 1, 1, pFile);
	fread(&uwCopperLength, 2, 1, pFile);
	fread(&uwCopperSize, 2, 1, pFile);
	
	// Init bitmapy
	pBitMap = bitmapCreate(uwWidth, uwHeight, ubPlaneCount, 0);
	for (i = 0; i != ubPlaneCount; ++i)
		fread(pBitMap->Planes[i], 1, (uwWidth >> 3) * uwHeight, pFile);
	fclose(pFile);
	
	logWrite("Dimensions: %ux%u@%uBPP, unused: %u %u\n",uwWidth,uwHeight,ubPlaneCount,uwCopperLength,uwCopperSize);
	logBlockEnd("bitmapCreateFromFile()");
	return pBitMap;
}

void bitmapDestroy(tBitMap *pBitMap) {
	UBYTE i;
	logBlockBegin("bitmapDestroy(pBitMap: %p)", pBitMap);
	if (pBitMap) {
		WaitBlit();
		if(bitmapIsInterleaved(pBitMap))
			pBitMap->Depth = 1;
		for (i = pBitMap->Depth; i--;)
			memFree(pBitMap->Planes[i], pBitMap->BytesPerRow*pBitMap->Rows);
		memFree(pBitMap, sizeof(tBitMap));
	}
	logBlockEnd("bitmapDestroy()");
}

inline BYTE bitmapIsInterleaved(tBitMap *pBitMap) {
	return (pBitMap->Depth > 1 && ((ULONG)pBitMap->Planes[1] - (ULONG)pBitMap->Planes[0])*pBitMap->Depth == pBitMap->BytesPerRow);
}

void bitmapDump(tBitMap *pBitMap) {
	UBYTE i;
	
	logBlockBegin("bitmapDump(pBitMap: %p)", pBitMap);
	
	logWrite(
		"BytesPerRow: %u, Rows: %u, Flags: %hu, Depth: %hu, pad: %u\n",
		pBitMap->BytesPerRow, pBitMap->Rows, pBitMap->Flags,
		pBitMap->Depth, pBitMap->pad
	);
	for(i = 0; i != pBitMap->Depth; ++i)
		logWrite("Bitplane %hu addr: %p\n", i, pBitMap->Planes[i]);
	
	logBlockEnd("bitmapDump()");
}