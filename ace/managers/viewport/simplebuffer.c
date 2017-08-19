#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/tag.h>

tSimpleBufferManager *simpleBufferCreate(
	void *pTags, ...
) {
	va_list vaTags;
	tCopList *pCopList;
	tSimpleBufferManager *pManager;
	UWORD uwBoundWidth, uwBoundHeight;
	UBYTE ubBitmapFlags;

	logBlockBegin("simpleBufferManagerCreate(pTags: %p, ...)", pTags);
	
	// Init manager
	pManager = memAllocFast(sizeof(tSimpleBufferManager));
	pManager->sCommon.pNext = 0;
	pManager->sCommon.process = (tVpManagerFn)simpleBufferProcess;
	pManager->sCommon.destroy = (tVpManagerFn)simpleBufferDestroy;
	pManager->sCommon.ubId = VPM_SCROLL;
	logWrite("Addr: %p\n", pManager);

	va_start(vaTags, pTags);
	tVPort *pVPort = (tVPort*)tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_VPORT, 0);
	if(!pVPort) {
		logWrite("ERR: No parent viewport (TAG_SIMPLEBUFFER_VPORT) specified!");
		goto fail;
	}
	pManager->sCommon.pVPort = pVPort;

	// Buffer bitmap
	uwBoundWidth = tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_BOUND_WIDTH, pVPort->uwWidth);
	uwBoundHeight = tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_BOUND_HEIGHT, pVPort->uwHeight);
	ubBitmapFlags = tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR);
	logWrite("Bounds: %ux%u\n", uwBoundWidth, uwBoundHeight);
	tBitMap *pBuffer = bitmapCreate(
		uwBoundWidth, uwBoundHeight,
		pVPort->ubBPP, ubBitmapFlags
	);
	if(!pBuffer) {
		logWrite("ERR: Can't alloc buffer bitmap!\n");
		goto fail;
	}
	
	// Find camera manager, create if not exists
	pManager->pCameraManager = (tCameraManager*)vPortGetManager(pVPort, VPM_CAMERA);
	if(!pManager->pCameraManager)
		pManager->pCameraManager = cameraCreate(pVPort, 0, 0, uwBoundWidth, uwBoundHeight);
	
	pCopList = pVPort->pView->pCopList;
	if(
		tagGet(
			pTags, vaTags,
			TAG_SIMPLEBUFFER_COPLIST_MODE, SIMPLEBUFFER_COPLIST_MODE_BLOCK
		) == SIMPLEBUFFER_COPLIST_MODE_BLOCK
	) {
		// CopBlock contains: bitplanes + shiftX
		pManager->pCopBlock = copBlockCreate(
			pCopList,
			// Shift +2 ddf +2 modulos +2*bpp bpladdr
			2*pVPort->ubBPP + 5,
			// Vertically addition from DiWStrt, horizontally a bit before last fetch.
			// First to set are ddf, modulos & shift so they are changed during fetch.
			0xE2-7*4, pVPort->uwOffsY + 0x2C-1
		);
	}
	else {
		const UWORD uwInvalidCopOffs = -1;
		pManager->ubFlags |= SIMPLEBUFFER_FLAG_COPLIST_RAW;
		pManager->uwCopperOffset = tagGet(
			pTags, vaTags, TAG_SIMPLEBUFFER_COPLIST_OFFSET, uwInvalidCopOffs
		);
		if(pManager->uwCopperOffset == uwInvalidCopOffs) {
			logWrite(
				"ERR: Copperlist offset (TAG_SIMPLEBUFFER_COPLIST_OFFSET) not specified"
			);
			goto fail;
		}
	}
	
	pManager->pBuffer = 0;
	simpleBufferSetBitmap(pManager, pBuffer);
	
	// Add manager to VPort
	vPortAddManager(pVPort, (tVpManager*)pManager);
	logBlockEnd("simpleBufferManagerCreate()");
	va_end(vaTags);	
	return pManager;

fail:
	logBlockEnd("simpleBufferManagerCreate()");			
	va_end(vaTags);
	return 0;
}

void simpleBufferSetBitmap(tSimpleBufferManager *pManager, tBitMap *pBitMap) {
	UWORD uwModulo, uwDDfStrt;
	tCopList *pCopList;
	tCopBlock *pBlock;
	UBYTE i;
	ULONG ulPlaneAddr;
	
	logBlockBegin(
		"simpleBufferSetBitmap(pManager: %p, pBitMap: %p)",
		pManager, pBitMap
	);
	if(pManager->pBuffer && pManager->pBuffer->Depth != pBitMap->Depth) {
		logWrite("ERR: buffer bitmaps differ in BPP!\n");
		return;
	}
	
	pManager->uBfrBounds.sUwCoord.uwX = bitmapGetByteWidth(pBitMap) << 3;
	pManager->uBfrBounds.sUwCoord.uwY = pBitMap->Rows;
	pManager->pBuffer = pBitMap;
	uwModulo = pBitMap->BytesPerRow - (pManager->sCommon.pVPort->uwWidth >> 3);
	logWrite("Modulo: %u\n", uwModulo);
	if(pManager->uBfrBounds.sUwCoord.uwX <= pManager->sCommon.pVPort->uwWidth) {
		uwDDfStrt = 0x0038;
		pManager->ubFlags &= ~SIMPLEBUFFER_FLAG_X_SCROLLABLE;
	}
	else {
		pManager->ubFlags |= SIMPLEBUFFER_FLAG_X_SCROLLABLE;
		uwDDfStrt = 0x0030;
		uwModulo -= 1;
	}
	
	// Update (rewrite) copperlist
	// TODO this could be unified with copBlock being set with copSetMove too
	if(pManager->ubFlags & SIMPLEBUFFER_FLAG_COPLIST_RAW) {
		// Since simpleBufferProcess only updates bitplane ptrs and shift,
		// copperlist must be shaped here.
		// WAIT is calc'd in same way as in copBlockCreate in simpleBufferCreate().
		tCopCmd *pCmdList = &pCopList->pBackBfr->pList[pManager->uwCopperOffset];
		copSetWait(&pCmdList[0].sWait, 0xE2-7*4, pManager->sCommon.pVPort->uwOffsY + 0x2C-1);
		copSetMove(&pCmdList[1].sMove, &custom.ddfstop, 0x00D0);    // Data fetch
		copSetMove(&pCmdList[2].sMove, &custom.ddfstrt, uwDDfStrt);
		copSetMove(&pCmdList[3].sMove, &custom.bpl1mod, uwModulo);  // Bitplane modulo
		copSetMove(&pCmdList[4].sMove, &custom.bpl2mod, uwModulo);
		copSetMove(&pCmdList[5].sMove, &custom.bplcon1, 0);         // Shift: 0
		for (i = 0; i != pManager->sCommon.pVPort->ubBPP; ++i) {
			copSetMove(&pCmdList[6 + i*2 + 0].sMove, &pBplPtrs[i].uwHi, ulPlaneAddr >> 16);
			copSetMove(&pCmdList[6 + i*2 + 1].sMove, &pBplPtrs[i].uwLo, ulPlaneAddr & 0xFFFF);
		}
		// Copy to front buffer since it needs initialization there too
		CopyMem(
			pCmdList, &pCopList->pFrontBfr->pList[pManager->uwCopperOffset],
			6+2*pManager->sCommon.pVPort->ubBPP*sizeof(tCopCmd)
		);
	}
	else {
		pBlock = pManager->pCopBlock;
		pCopList = pManager->sCommon.pVPort->pView->pCopList;
		pBlock->uwCurrCount = 0; // Rewind to beginning
		copMove(pCopList, pBlock, &custom.ddfstop, 0x00D0);     // Data fetch
		copMove(pCopList, pBlock, &custom.ddfstrt, uwDDfStrt);
		copMove(pCopList, pBlock, &custom.bpl1mod, uwModulo);   // Bitplane modulo
		copMove(pCopList, pBlock, &custom.bpl2mod, uwModulo);
		copMove(pCopList, pBlock, &custom.bplcon1, 0);          // Shift: 0
		for (i = 0; i != pManager->sCommon.pVPort->ubBPP; ++i) {
			ulPlaneAddr = (ULONG)pManager->pBuffer->Planes[i];
			copMove(pCopList, pBlock, &pBplPtrs[i].uwHi, ulPlaneAddr >> 16);
			copMove(pCopList, pBlock, &pBplPtrs[i].uwLo, ulPlaneAddr & 0xFFFF);
		}
	}
	logBlockEnd("simplebufferSetBitmap()");
}

void simpleBufferDestroy(tSimpleBufferManager *pManager) {
	logWrite("Destroying bitmap...\n");
	bitmapDestroy(pManager->pBuffer);
	logWrite("Freeing mem...\n");
	memFree(pManager, sizeof(tSimpleBufferManager));
	logWrite("Done\n");
}

void simpleBufferProcess(tSimpleBufferManager *pManager) {
	UBYTE i;
	UWORD uwShift;
	ULONG ulBplOffs;
	ULONG ulPlaneAddr;
	tCameraManager *pCameraManager;
	tCopList *pCopList;
	
	pCameraManager = pManager->pCameraManager;
	pCopList = pManager->sCommon.pVPort->pView->pCopList;
	
	// Calculate X movement
	if(pManager->ubFlags & SIMPLEBUFFER_FLAG_X_SCROLLABLE) {
		uwShift = 15-(pCameraManager->uPos.sUwCoord.uwX & 0xF);
		uwShift = (uwShift << 4) | uwShift;
	}
	else
		uwShift = 0;
	
	// X offset on bitplane
	ulBplOffs = (pCameraManager->uPos.sUwCoord.uwX >> 4) << 1;
	
	// Calculate Y movement
	ulBplOffs += pManager->pBuffer->BytesPerRow*pCameraManager->uPos.sUwCoord.uwY;
	
	// Copperlist - regen bitplane ptrs, update shift
	// TODO could be unified by using copSetMove in copBlock
	if(pManager->ubFlags & SIMPLEBUFFER_FLAG_COPLIST_RAW) {
		tCopCmd *pCmdList = &pCopList->pBackBfr->pList[pManager->uwCopperOffset];
		copSetMove(&pCmdList[5].sMove, &custom.bplcon1, uwShift);
		for (i = 0; i != pManager->sCommon.pVPort->ubBPP; ++i) {
			copSetMove(&pCmdList[6 + i*2 + 0].sMove, &pBplPtrs[i].uwHi, ulPlaneAddr >> 16);
			copSetMove(&pCmdList[6 + i*2 + 1].sMove, &pBplPtrs[i].uwLo, ulPlaneAddr & 0xFFFF);
		}
	}
	else {
		pManager->pCopBlock->uwCurrCount = 4; // Rewind to shift cmd pos
		copMove(pCopList, pManager->pCopBlock, &custom.bplcon1, uwShift);
		for(i = 0; i != pManager->pBuffer->Depth; ++i) {
			ulPlaneAddr = ((ULONG)pManager->pBuffer->Planes[i]) + ulBplOffs;
			copMove(pCopList, pManager->pCopBlock, &pBplPtrs[i].uwHi, ulPlaneAddr >> 16);
			copMove(pCopList, pManager->pCopBlock, &pBplPtrs[i].uwLo, ulPlaneAddr & 0xFFFF);
		}
	}
}
