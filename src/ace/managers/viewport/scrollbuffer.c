#include <ace/managers/viewport/scrollbuffer.h>

#ifdef AMIGA

/**
 * Creates scroll manager
 */
tScrollBufferManager *scrollBufferCreate(tVPort *pVPort, UBYTE ubMarginWidth, UWORD uwBoundWidth, UWORD uwBoundHeight) {
	logBlockBegin("scrollBufferCreate(pVPort: %p, ubMarginWidth: %hu, uwBoundWidth: %u, uwBoundHeight: %u)", pVPort, ubMarginWidth, uwBoundWidth, uwBoundHeight);
	tScrollBufferManager *pManager;

	// Wype�nienie struktury mened�era
	pManager = memAllocFast(sizeof(tScrollBufferManager));
	logWrite("Addr: %p\n", pManager);
	pManager->sCommon.pNext = 0;
	pManager->sCommon.process = (tVpManagerFn)scrollBufferProcess;
	pManager->sCommon.destroy = (tVpManagerFn)scrollBufferDestroy;
	pManager->sCommon.ubId = VPM_SCROLL;
	pManager->sCommon.pVPort = pVPort;
	pManager->pBuffer = 0;
	pManager->uwModulo = 0;
	pManager->pAvg = logAvgCreate("scrollBufferProcess()", 100);

	scrollBufferReset(pManager, ubMarginWidth, uwBoundWidth, uwBoundHeight);

	// Create copperlist
	pManager->pStartBlock = copBlockCreate(pVPort->pView->pCopList, 2*pVPort->ubBPP + 8, 0, 0);
	pManager->pBreakBlock = copBlockCreate(pVPort->pView->pCopList, 2*pVPort->ubBPP + 2, 0x7F, 0xFF);

	// Must be before camera? Shouldn't be as there are priorities on manager list
	vPortAddManager(pVPort, (tVpManager*)pManager);

	// Find camera manager, create if not exists
	if(!(pManager->pCameraManager = (tCameraManager*)vPortGetManager(pVPort, VPM_CAMERA)))
		pManager->pCameraManager = cameraCreate(pVPort, 0, 0, uwBoundWidth, uwBoundHeight);
	else
		cameraReset(pManager->pCameraManager, 0,0, uwBoundWidth, uwBoundHeight);

	// TODO: Set copperlist to current camera pos?

	logBlockEnd("scrollBufferCreate");
	return pManager;
}

void scrollBufferDestroy(tScrollBufferManager *pManager) {
	logBlockBegin("scrollBufferDestroy(pManager: %p)", pManager);

	logAvgDestroy(pManager->pAvg);
	copBlockDestroy(pManager->sCommon.pVPort->pView->pCopList, pManager->pStartBlock);
	copBlockDestroy(pManager->sCommon.pVPort->pView->pCopList, pManager->pBreakBlock);

	bitmapDestroy(pManager->pBuffer);
	memFree(pManager, sizeof(tScrollBufferManager));

	logBlockEnd("scrollBufferDestroy()");
}

/**
 * Scroll buffer process function
 */
void scrollBufferProcess(tScrollBufferManager *pManager) {
	UWORD uwVpHeight;

	uwVpHeight = pManager->sCommon.pVPort->uwHeight;
	logAvgBegin(pManager->pAvg);
	if (cameraIsMoved(pManager->pCameraManager) || pManager->uwVpHeightPrev != uwVpHeight) {
		UBYTE i;
		UWORD uwScrollX, uwScrollY;
		ULONG ulPlaneOffs;
		ULONG ulPlaneAddr;
		UWORD uwOffsX;

		// convert camera pos to scroll pos
		uwScrollX = pManager->pCameraManager->uPos.sUwCoord.uwX;
		uwScrollY = pManager->pCameraManager->uPos.sUwCoord.uwY % pManager->uwBmAvailHeight;

		// preparations for new copperlist
		uwOffsX = 15 - (uwScrollX & 0xF);         // Bitplane shift - single
		uwOffsX = (uwOffsX << 4) | uwOffsX;       // Bitplane shift - PF1 | PF2

		uwScrollX >>= 3;

		// Initial copper block
		tCopList *pCopList = pManager->sCommon.pVPort->pView->pCopList;
		tCopBlock *pBlock = pManager->pStartBlock;
		pBlock->uwCurrCount = 0; // Rewind copBlock
		copBlockWait(pCopList, pBlock, 0, 0x2C + pManager->sCommon.pVPort->uwOffsY);
		copMove(pCopList, pBlock, &custom.color[0], 0x0F0);
		copMove(pCopList, pBlock, &custom.bplcon1, uwOffsX);            // Bitplane shift
		ulPlaneOffs = uwScrollX + (pManager->pBuffer->BytesPerRow*uwScrollY);
		for (i = pManager->sCommon.pVPort->ubBPP; i--;) {
			ulPlaneAddr = (ULONG)(pManager->pBuffer->Planes[i]) + ulPlaneOffs;
			copMove(pCopList, pBlock, &pBplPtrs[i].uwLo, ulPlaneAddr & 0xFFFF);
			copMove(pCopList, pBlock, &pBplPtrs[i].uwHi, ulPlaneAddr >> 16);
		}
		copMove(pCopList, pBlock, &custom.ddfstrt, 0x30);               // Fetch start
		copMove(pCopList, pBlock, &custom.bpl1mod, pManager->uwModulo); // Odd planes modulo
		copMove(pCopList, pBlock, &custom.bpl2mod, pManager->uwModulo); // Even planes modulo
		copMove(pCopList, pBlock, &custom.ddfstop, 0x00D0);             // Fetch stop
		copMove(pCopList, pBlock, &custom.color[0], 0x000);

		// Copper block after Y-break
		pBlock = pManager->pBreakBlock;

		pBlock->uwCurrCount = 0; // Rewind copBlock
		if (pManager->uwBmAvailHeight - uwScrollY <= uwVpHeight) {
			// logWrite("Break calc: %u - %u == %u, vpHeight: %u\n", pManager->uwBmAvailHeight, uwScrollY, pManager->uwBmAvailHeight - uwScrollY, uwVpHeight);
			copBlockWait(pCopList, pBlock, 0, 0x2C + pManager->sCommon.pVPort->uwOffsY + pManager->uwBmAvailHeight - uwScrollY);
			// copMove(pCopList, pBlock, &custom.bplcon1, uwOffsX); // potrzebne?
			copMove(pCopList, pBlock, &custom.color[0], 0x0F00);
			for (i = pManager->sCommon.pVPort->ubBPP; i--;) {
				ulPlaneAddr = (ULONG)(pManager->pBuffer->Planes[i]) + uwScrollX;
				copMove(pCopList, pBlock, &pBplPtrs[i].uwHi, ulPlaneAddr >> 16);
				copMove(pCopList, pBlock, &pBplPtrs[i].uwLo, ulPlaneAddr & 0xFFFF);
			}
			copMove(pCopList, pBlock, &custom.color[0], 0x0000);
		}
		else
			copBlockWait(pCopList, pBlock, 0x7F, 0xFF);


		pManager->uwVpHeightPrev = uwVpHeight;
		copProcessBlocks();
	}

	logAvgEnd(pManager->pAvg);
	WaitTOF();
}

void scrollBufferReset(tScrollBufferManager *pManager, UBYTE ubMarginWidth, UWORD uwBoundWidth, UWORD uwBoundHeight) {
	UWORD uwVpWidth, uwVpHeight;
	UWORD uwCalcWidth, uwCalcHeight;
	logBlockBegin("scrollBufferReset(pManager: %p, ubMarginWidth: %hu, uwBoundWidth: %u, uwBoundHeight: %u)", pManager, ubMarginWidth, uwBoundWidth, uwBoundHeight);
	// Helper vars
	uwVpWidth = pManager->sCommon.pVPort->uwWidth;
	uwVpHeight = pManager->sCommon.pVPort->uwHeight;

	// Reset manager fields
	pManager->uwVpHeightPrev = 0;
	pManager->uBfrBounds.sUwCoord.uwX = uwBoundWidth;
	pManager->uBfrBounds.sUwCoord.uwY = uwBoundHeight;
	pManager->uwBmAvailHeight = ubMarginWidth * blockCountCeil(uwVpHeight, ubMarginWidth) + ubMarginWidth*4;

	// Destroy old buffer bitmap
	if(pManager->pBuffer)
		bitmapDestroy(pManager->pBuffer);

	// Create new buffer bitmap
	uwCalcWidth = uwVpWidth + ubMarginWidth*4;
	uwCalcHeight = pManager->uwBmAvailHeight + blockCountCeil(uwBoundWidth, uwVpWidth) - 1;
	pManager->pBuffer = bitmapCreate(uwCalcWidth, uwCalcHeight, pManager->sCommon.pVPort->ubBPP, BMF_CLEAR);
	pManager->uwModulo = pManager->pBuffer->BytesPerRow - (uwVpWidth >> 3) - 2;

	logBlockEnd("scrollBufferReset()");
}

/**
 * Uses unsafe blit copy for using out-of-bound X ccord
 */
void scrollBufferBlitMask(
	tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tScrollBufferManager *pDstManager, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, UWORD *pMsk
) {
	// TODO: if area is visible
	wDstY %= pDstManager->uwBmAvailHeight;
	// UBYTE ubAddY = wDstX/(pDstManager->pBuffer->BytesPerRow<<3);
	wDstY %= (pDstManager->pBuffer->BytesPerRow<<3);

	if(wDstY + wHeight <= pDstManager->uwBmAvailHeight) {
		// single blit
		blitUnsafeCopyMask(
			pSrc, wSrcX, wSrcY,
			pDstManager->pBuffer, wDstX, wDstY,
			wWidth, wHeight, pMsk
		);
	}
	else {
		// split blit in two
		WORD wPartHeight;
		wPartHeight = pDstManager->uwBmAvailHeight - wDstY;
		blitUnsafeCopyMask(
			pSrc, wSrcX, wSrcY,
			pDstManager->pBuffer, wDstX, wDstY,
			wWidth, wPartHeight, pMsk
		);
		blitUnsafeCopyMask(
			pSrc, wSrcX, wSrcY + wPartHeight,
			pDstManager->pBuffer, wDstX, 0,
			wWidth, wHeight - wPartHeight, pMsk
		);
	}
}

#endif // AMIGA
