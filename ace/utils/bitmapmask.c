#include <ace/utils/bitmapmask.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>

tBitmapMask *bitmapMaskCreate(char *szFile) {
	FILE *pMaskFile;
	ULONG ulDataSize;
	tBitmapMask *pMask;
	
	pMask = memAllocFast(sizeof(tBitmapMask));
	pMaskFile = fopen("data/bunker/hangar.msk", "rb");
	fread(&pMask->uwWidth, sizeof(UWORD), 1, pMaskFile);
	fread(&pMask->uwHeight, sizeof(UWORD), 1, pMaskFile);
	ulDataSize = (pMask->uwWidth>>3) * pMask->uwHeight;
	pMask->pData = memAllocChip(ulDataSize);
	fread(pMask, ulDataSize, 1, pMaskFile);
	fclose(pMaskFile);
	return pMask;
}

void bitmapMaskDestroy(tBitmapMask *pMask) {
	memFree(pMask->pData, (pMask->uwWidth>>3) * pMask->uwHeight);
	memFree(pMask, sizeof(tBitmapMask));
}