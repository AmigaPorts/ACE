#include <ace/utils/bitmapmask.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>

tBitmapMask *bitmapMaskCreate(char *szFile) {
	FILE *pMaskFile;
	ULONG ulDataSize;
	tBitmapMask *pMask;
	
	logBlockBegin("bitmapMaskCreate(szFile: %s)", szFile);
	
	pMask = memAllocFast(sizeof(tBitmapMask));
	if(!pMask) {
		logWrite("Couldn't alloc RAM for mask struct\n");
		logBlockEnd("bitmapMaskCreate()");
		return 0;
	}
	pMaskFile = fopen(szFile, "rb");
	if(!pMaskFile) {
		logWrite("File doesn't exist: %s\n", pMask);
		memFree(pMask->pData, (pMask->uwWidth>>3) * pMask->uwHeight);
		fclose(pMaskFile);
		logBlockEnd("bitmapMaskCreate()");
		return 0;
	}
	fread(&pMask->uwWidth, sizeof(UWORD), 1, pMaskFile);
	fread(&pMask->uwHeight, sizeof(UWORD), 1, pMaskFile);
	ulDataSize = (pMask->uwWidth>>3) * pMask->uwHeight;
	pMask->pData = memAllocChip(ulDataSize);
	if(!pMask->pData) {
		logWrite("Couldn't alloc RAM for mask data\n");
		memFree(pMask->pData, (pMask->uwWidth>>3) * pMask->uwHeight);
		fclose(pMaskFile);
		logBlockEnd("bitmapMaskCreate()");
		return 0;
	}
	fread(pMask->pData, ulDataSize, 1, pMaskFile);
	fclose(pMaskFile);
	
	logBlockEnd("bitmapMaskCreate()");
	return pMask;
}

void bitmapMaskDestroy(tBitmapMask *pMask) {
	logBlockBegin("bitmapMaskDestroy(pMask: %p)");
	memFree(pMask->pData, (pMask->uwWidth>>3) * pMask->uwHeight);
	memFree(pMask, sizeof(tBitmapMask));
	logBlockEnd("bitmapMaskDestroy()");
}