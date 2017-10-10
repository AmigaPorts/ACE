#include <ace/utils/bitmapmask.h>
#include <ace/utils/bitmap.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>

tBitmapMask *bitmapMaskCreate(UWORD uwWidth, UWORD uwHeight) {
	tBitmapMask *pMask = 0;
	ULONG ulDataSize;

	logBlockBegin(
		"bitmapMaskCreate(uwWidth: %u, uwHeight: %u)", uwWidth, uwHeight
	);

	// Allocate struct
	pMask = memAllocFast(sizeof(tBitmapMask));
	if(!pMask) {
		logWrite("ERR: Couldn't alloc RAM for mask struct\n");
		goto fail;
	}
	logWrite("Addr: %p\n", pMask);
	pMask->uwWidth = uwWidth;
	pMask->uwHeight = uwHeight;

	// Allocate data
	ulDataSize = (pMask->uwWidth>>3) * pMask->uwHeight;
	pMask->pData = memAllocChip(ulDataSize);
	if(!pMask->pData) {
		logWrite("ERR: Couldn't alloc RAM for mask data\n");
		goto fail;
	}
	logWrite("Data addr: %p\n", pMask->pData);

	logBlockEnd("bitmapMaskCreate()");
	return pMask;

fail:
	if(pMask)
		bitmapMaskDestroy(pMask);
	logBlockEnd("bitmapMaskCreate()");
	return 0;
}

tBitmapMask *bitmapMaskCreateFromFile(char *szFile) {
	FILE *pMaskFile = 0;
	tBitmapMask *pMask = 0;
	ULONG ulDataSize;
	UWORD uwWidth, uwHeight;

	logBlockBegin("bitmapMaskCreateFromFile(szFile: %s)", szFile);

	// Read width & height from file
	pMaskFile = fopen(szFile, "rb");
	if(!pMaskFile) {
		logWrite("ERR: File doesn't exist: %s\n", pMask);
		goto fail;
	}
	fread(&uwWidth, sizeof(UWORD), 1, pMaskFile);
	fread(&uwHeight, sizeof(UWORD), 1, pMaskFile);

	// Create mask of given size
	pMask = bitmapMaskCreate(uwWidth, uwHeight);
	if(!pMask) {
		logWrite("ERR: Couldn't alloc mask!\n");
		goto fail;
	}

	// Fill with data
	ulDataSize = (uwWidth>>3) * uwHeight;
	fread(pMask->pData, ulDataSize, 1, pMaskFile);
	fclose(pMaskFile);
	pMaskFile = 0;

	logBlockEnd("bitmapMaskCreateFromFile()");
	return pMask;

fail:
	if(pMask)
		bitmapMaskDestroy(pMask);
	if(pMaskFile)
		fclose(pMaskFile);
	logBlockEnd("bitmapMaskCreateFromFile()");
	return 0;
}

void bitmapMaskDestroy(tBitmapMask *pMask) {
	logBlockBegin("bitmapMaskDestroy(pMask: %p)");
	if(pMask->pData)
		memFree(pMask->pData, (pMask->uwWidth>>3) * pMask->uwHeight);
	memFree(pMask, sizeof(tBitmapMask));
	logBlockEnd("bitmapMaskDestroy()");
}

void bitmapMaskSave(tBitmapMask *pMask, char *szPath) {
	logBlockBegin("bitmapMaskSave(pMask: %p, szPath: %s)", pMask, szPath);
	FILE *pFile = fopen(szPath, "wb");
	if(!pFile) {
		logWrite("ERR: Couldn't save file at: %s\n", szPath);
		logBlockEnd("bitmapMaskSave()");
		return;
	}
	fwrite(&pMask->uwWidth, sizeof(UWORD), 1, pFile);
	fwrite(&pMask->uwHeight, sizeof(UWORD), 1, pFile);

	ULONG ulDataSize = (pMask->uwWidth>>3) * pMask->uwHeight;
	fwrite(pMask->pData, ulDataSize, 1, pFile);
	fclose(pFile);
	logBlockEnd("bitmapMaskSave()");
}

void bitmapMaskSaveBmp(tBitmapMask *pMask, char *szPath) {
	tBitMap sBitmap;
	const UWORD pPalette[2] = {0x0000, 0x0fff};

	InitBitMap(&sBitmap, 1, pMask->uwWidth, pMask->uwHeight);
	sBitmap.Planes[0] = (APTR)pMask->pData;
	bitmapSaveBmp(&sBitmap, (UWORD*)pPalette, szPath);
}
