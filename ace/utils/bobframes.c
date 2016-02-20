#include <ace/utils/bobframes.h>

// TODO: read bpp from file
tBobFrameset *bobFramesCreate(char *szFileName) {
	tBobFrameset *pFrameset;
	tBobFrame *pFrame;
	FILE *pFile;
	UBYTE ubAnim;
	UBYTE ubDir;	
	UBYTE ubPlane;
	logBlockBegin("bobFramesCreate(szFileName: %s)", szFileName);
	
	pFrameset = memAllocFast(sizeof(tBobFrameset)); // TODO: assert czy cuœ
	logWrite("destination addr: %p\n", pFrameset);

	pFile = fopen(szFileName, "rb");
	
	// Header
	fread(&pFrameset->ubFrameWidth, 1, 1, pFile);
	fread(&pFrameset->ubFrameHeight, 1, 1, pFile);
	fread(&pFrameset->ubAnimCount, 1, 1, pFile);
	logWrite("Header: frameWidth: %u, frameHeight: %u, animCount: %u\n", pFrameset->ubFrameWidth, pFrameset->ubFrameHeight, pFrameset->ubAnimCount);
	
	pFrameset->pData = memAllocFast(sizeof(tBobFrame**) * pFrameset->ubAnimCount);
	for(ubAnim = 0; ubAnim != pFrameset->ubAnimCount; ++ubAnim) {
		pFrameset->pData[ubAnim] = memAllocFast(sizeof(tBobFrame*) * BOB_DIRS);
		for(ubDir = 0; ubDir != BOB_DIRS; ++ubDir) {
			
			// Frame struct
			pFrameset->pData[ubAnim][ubDir] = memAllocFast(sizeof(tBobFrame));
			pFrame = pFrameset->pData[ubAnim][ubDir];
			
			// logBlockBegin("Reading bob raster %u-%u @%p", ubAnim, ubDir, pFrame);
			g_sLogManager.ubShutUp = 1;
			// Bitmap
			pFrame->pBitMap = bitmapCreate(pFrameset->ubFrameWidth, pFrameset->ubFrameHeight, BOBFRAMES_BPP, 0);
			for(ubPlane = 0; ubPlane != BOBFRAMES_BPP; ++ubPlane)
				fread(pFrame->pBitMap->Planes[ubPlane], (pFrameset->ubFrameWidth >> 3) * pFrameset->ubFrameHeight, 1, pFile);
			
			// Mask
			pFrame->pMask = memAllocChip((pFrameset->ubFrameWidth >> 3) * pFrameset->ubFrameHeight);
			fread(pFrame->pMask, (pFrameset->ubFrameWidth >> 3) * pFrameset->ubFrameHeight, 1, pFile);
			g_sLogManager.ubShutUp = 0;
			// logBlockEnd("Reading bob raster");
		}
	}
	
	logBlockEnd("bobFramesCreate()");
	fclose(pFile);
	return pFrameset;
}

void bobFramesDestroy(tBobFrameset *pFrameset) {
	UBYTE ubAnim;
	UBYTE ubDir;
	tBobFrame ***pImageDatas;
	tBobFrame *pFrame;
	
	logBlockBegin("bobFramesDestroy(pFrameset: %p)", pFrameset);
	
#ifdef GAME_DEBUG
	if(pFrameset->ubFrameWidth == 0 || pFrameset->ubFrameHeight == 0)
		logWrite(" ERR: WRONG pFrameset POINTER (frame width %u and height %u)... \n", pFrameset->ubFrameWidth, pFrameset->ubFrameHeight);
#endif
	pImageDatas = pFrameset->pData;
	
	for(ubAnim = 0; ubAnim != pFrameset->ubAnimCount; ++ubAnim) {
		for(ubDir = 0; ubDir != BOB_DIRS; ++ubDir) {
			pFrame = pImageDatas[ubAnim][ubDir];
			// Zwolnij surówkê bitplane'ów + maskê
			g_sLogManager.ubShutUp = 1;
			bitmapDestroy(pFrame->pBitMap);
			g_sLogManager.ubShutUp = 0;
			memFree(pFrame->pMask, (pFrameset->ubFrameWidth>>3) * pFrameset->ubFrameHeight);
			memFree(pFrame, sizeof(tBobFrame));
		}
		memFree(pImageDatas[ubAnim], sizeof(tBobFrame*) * BOB_DIRS);
	}
	memFree(pImageDatas, sizeof(tBobFrame**) * pFrameset->ubAnimCount);
	memFree(pFrameset, sizeof(tBobFrameset));
	
	logBlockEnd("bobFramesDestroy()");
}
