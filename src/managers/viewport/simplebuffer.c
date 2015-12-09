#include "simplebuffer.h"

tSimpleBufferManager *simpleBufferCreate(tVPort *pVPort, UWORD uwBoundWidth, UWORD uwBoundHeight) {
	UBYTE i;
	UWORD uwScrollX, uwScrollY, uwOffsX, uwModulo;
	ULONG ulPlaneAddr;
	tCopList *pCopList;
	tCopBlock *pBlock;
	tSimpleBufferManager *pManager;

	logBlockBegin("simpleBufferManagerCreate(pVPort: %p, uwBoundWidth: %u, uwBoundHeight: %u)", pVPort, uwBoundWidth, uwBoundHeight);
	
	pManager = memAllocFast(sizeof(tSimpleBufferManager));
	logWrite("Addr: %p\n", pManager);
	logWrite("Bounds: %ux%u\n", uwBoundWidth, uwBoundHeight);
	
	// Struct init
	pManager->sCommon.pNext = 0;
	pManager->sCommon.process = (tVpManagerFn)simpleBufferProcess;
	pManager->sCommon.destroy = (tVpManagerFn)simpleBufferDestroy;
	pManager->sCommon.pVPort = pVPort;
	pManager->sCommon.ubId = VPM_SCROLL;
	pManager->uBfrBounds.sUwCoord.uwX = uwBoundWidth;
	pManager->uBfrBounds.sUwCoord.uwX = uwBoundHeight;
		
	// Buffer bitmap
	pManager->pBuffer = bitmapCreate(uwBoundWidth, uwBoundHeight, pVPort->ubBPP, BMF_CLEAR);
	
	// Find camera manager, create if not exists
	if(!(pManager->pCameraManager = (tCameraManager*)vPortGetManager(pVPort, VPM_CAMERA)))
		pManager->pCameraManager = cameraCreate(pVPort, 0, 0, uwBoundWidth, uwBoundHeight);
	
	uwOffsX = 0;
	uwModulo = pManager->pBuffer->BytesPerRow - (pManager->sCommon.pVPort->uwHeight >> 3);

	// Form display - set registers
	custom.ddfstop = 0x00D0;
	custom.ddfstrt = 0x0038;
	custom.bpl1mod = uwModulo;
	custom.bpl2mod = uwModulo;
	
	pCopList = pVPort->pView->pCopList;
	// Bitplanes + shift X
	pBlock = copBlockCreate(pCopList, 2*pVPort->ubBPP + 1, 0, pManager->sCommon.pVPort->uwOffsY);
	pManager->pCopBlock = pBlock;
	
	// Copperlist - regen bitplane ptrs, update shift
	copMove(pCopList, pBlock, &custom.bplcon1, 0); // shift: 0
	for (i = 0; i != pVPort->ubBPP; ++i) {
		ulPlaneAddr = (ULONG)pManager->pBuffer->Planes[i];
		copMove(pCopList, pBlock, &pBplPtrs[i].uwHi, ulPlaneAddr >> 16);
		copMove(pCopList, pBlock, &pBplPtrs[i].uwLo, ulPlaneAddr & 0xFFFF);
	}
	
	// podpiêcie pod VPort
	vPortAddManager(pVPort, (tVpManager*)pManager);
	logBlockEnd("simpleBufferManagerCreate()");
	return pManager;
}

void simpleBufferDestroy(tSimpleBufferManager *pManager) {
	bitmapDestroy(pManager->pBuffer);
	memFree(pManager, sizeof(tSimpleBufferManager));
}

void simpleBufferProcess(tSimpleBufferManager *pManager) {
	// TODO: scrolling
}
