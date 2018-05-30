/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/tag.h>

#ifdef AMIGA

// Flags for internal usage.
#define SIMPLEBUFFER_FLAG_X_SCROLLABLE 1
#define SIMPLEBUFFER_FLAG_COPLIST_RAW  2

static void simpleBufferSetBack(tSimpleBufferManager *pManager, tBitMap *pBack) {
#if defined(ACE_DEBUG)
	if(pManager->pBack && pManager->pBack->Depth != pBack->Depth) {
		logWrite("ERR: buffer bitmaps differ in BPP!\n");
		return;
	}
#endif
	pManager->pBack = pBack;
}

void simpleBufferSetFront(tSimpleBufferManager *pManager, tBitMap *pFront) {
	logBlockBegin(
		"simpleBufferSetFront(pManager: %p, pFront: %p)",
		pManager, pFront
	);
#if defined(ACE_DEBUG)
	if(pManager->pFront && pManager->pFront->Depth != pFront->Depth) {
		logWrite("ERR: buffer bitmaps differ in BPP!\n");
		return;
	}
#endif

	pManager->uBfrBounds.sUwCoord.uwX = bitmapGetByteWidth(pFront) << 3;
	pManager->uBfrBounds.sUwCoord.uwY = pFront->Rows;
	pManager->pFront = pFront;
	UWORD uwModulo = pFront->BytesPerRow - (pManager->sCommon.pVPort->uwWidth >> 3);
	UWORD uwDDfStrt;
	if(pManager->uBfrBounds.sUwCoord.uwX <= pManager->sCommon.pVPort->uwWidth) {
		uwDDfStrt = 0x0038;
		pManager->ubFlags &= ~SIMPLEBUFFER_FLAG_X_SCROLLABLE;
	}
	else {
		pManager->ubFlags |= SIMPLEBUFFER_FLAG_X_SCROLLABLE;
		uwDDfStrt = 0x0030;
		uwModulo -= 1;
	}
	logWrite("Modulo: %u\n", uwModulo);

	// Update (rewrite) copperlist
	// TODO this could be unified with copBlock being set with copSetMove too
	tCopList *pCopList = pManager->sCommon.pVPort->pView->pCopList;
	if(pManager->ubFlags & SIMPLEBUFFER_FLAG_COPLIST_RAW) {
		// Since simpleBufferProcess only updates bitplane ptrs and shift,
		// copperlist must be shaped here.
		// WAIT is calc'd in same way as in copBlockCreate in simpleBufferCreate().
		tCopCmd *pCmdList = &pCopList->pBackBfr->pList[pManager->uwCopperOffset];
		logWrite(
			"Setting copperlist %p at offs %u\n",
			pCopList->pBackBfr, pManager->uwCopperOffset
		);
		copSetWait(&pCmdList[0].sWait, 0xE2-7*4, pManager->sCommon.pVPort->uwOffsY + 0x2C-1);
		copSetMove(&pCmdList[1].sMove, &g_pCustom->ddfstop, 0x00D0);    // Data fetch
		copSetMove(&pCmdList[2].sMove, &g_pCustom->ddfstrt, uwDDfStrt);
		copSetMove(&pCmdList[3].sMove, &g_pCustom->bpl1mod, uwModulo);  // Bitplane modulo
		copSetMove(&pCmdList[4].sMove, &g_pCustom->bpl2mod, uwModulo);
		copSetMove(&pCmdList[5].sMove, &g_pCustom->bplcon1, 0);         // Shift: 0
		UBYTE i;
		ULONG ulPlaneAddr;
		for (i = 0; i < pManager->sCommon.pVPort->ubBPP; ++i) {
			ulPlaneAddr = (ULONG)pManager->pFront->Planes[i];
			copSetMove(&pCmdList[6 + i*2 + 0].sMove, &g_pBplFetch[i].uwHi, ulPlaneAddr >> 16);
			copSetMove(&pCmdList[6 + i*2 + 1].sMove, &g_pBplFetch[i].uwLo, ulPlaneAddr & 0xFFFF);
		}
		// Copy to front buffer since it needs initialization there too
		CopyMem(
			&pCopList->pBackBfr->pList[pManager->uwCopperOffset],
			&pCopList->pFrontBfr->pList[pManager->uwCopperOffset],
			(6+2*pManager->sCommon.pVPort->ubBPP)*sizeof(tCopCmd)
		);
	}
	else {
		tCopBlock *pBlock = pManager->pCopBlock;
		pBlock->uwCurrCount = 0; // Rewind to beginning
		copMove(pCopList, pBlock, &g_pCustom->ddfstop, 0x00D0);     // Data fetch
		copMove(pCopList, pBlock, &g_pCustom->ddfstrt, uwDDfStrt);
		copMove(pCopList, pBlock, &g_pCustom->bpl1mod, uwModulo);   // Bitplane modulo
		copMove(pCopList, pBlock, &g_pCustom->bpl2mod, uwModulo);
		copMove(pCopList, pBlock, &g_pCustom->bplcon1, 0);          // Shift: 0
		for (UBYTE i = 0; i < pManager->sCommon.pVPort->ubBPP; ++i) {
			ULONG ulPlaneAddr = (ULONG)pManager->pFront->Planes[i];
			copMove(pCopList, pBlock, &g_pBplFetch[i].uwHi, ulPlaneAddr >> 16);
			copMove(pCopList, pBlock, &g_pBplFetch[i].uwLo, ulPlaneAddr & 0xFFFF);
		}
	}
	logBlockEnd("simplebufferSetFront()");
}

tSimpleBufferManager *simpleBufferCreate(void *pTags, ...) {
	va_list vaTags;
	tCopList *pCopList;
	tSimpleBufferManager *pManager;
	UWORD uwBoundWidth, uwBoundHeight;
	UBYTE ubBitmapFlags;
	tBitMap *pFront = 0, *pBack = 0;
	UBYTE isCameraCreated = 0;

	logBlockBegin("simpleBufferCreate(pTags: %p, ...)", pTags);
	va_start(vaTags, pTags);

	// Init manager
	pManager = memAllocFastClear(sizeof(tSimpleBufferManager));
	pManager->sCommon.process = (tVpManagerFn)simpleBufferProcess;
	pManager->sCommon.destroy = (tVpManagerFn)simpleBufferDestroy;
	pManager->sCommon.ubId = VPM_SCROLL;
	logWrite("Addr: %p\n", pManager);

	tVPort *pVPort = (tVPort*)tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_VPORT, 0);
	if(!pVPort) {
		logWrite("ERR: No parent viewport (TAG_SIMPLEBUFFER_VPORT) specified!\n");
		goto fail;
	}
	pManager->sCommon.pVPort = pVPort;
	logWrite("Parent VPort: %p\n", pVPort);

	// Buffer bitmap
	uwBoundWidth = tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_BOUND_WIDTH, pVPort->uwWidth);
	uwBoundHeight = tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_BOUND_HEIGHT, pVPort->uwHeight);
	ubBitmapFlags = tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR);
	logWrite("Bounds: %ux%u\n", uwBoundWidth, uwBoundHeight);
	pFront = bitmapCreate(
		uwBoundWidth, uwBoundHeight, pVPort->ubBPP, ubBitmapFlags
	);
	if(!pFront) {
		logWrite("ERR: Can't alloc buffer bitmap!\n");
		goto fail;
	}

	UBYTE isDblBfr = tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_IS_DBLBUF, 0);
	if(isDblBfr) {
		pBack = bitmapCreate(
			uwBoundWidth, uwBoundHeight, pVPort->ubBPP, ubBitmapFlags
		);
		if(!pBack) {
			logWrite("ERR: Can't alloc buffer bitmap!\n");
			goto fail;
		}
	}

	// Find camera manager, create if not exists
	pManager->pCameraManager = (tCameraManager*)vPortGetManager(pVPort, VPM_CAMERA);
	if(!pManager->pCameraManager) {
		isCameraCreated = 1;
		pManager->pCameraManager = cameraCreate(pVPort, 0, 0, uwBoundWidth, uwBoundHeight);
	}

	pCopList = pVPort->pView->pCopList;
	if(pCopList->ubMode == COPPER_MODE_BLOCK) {
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
				"ERR: Copperlist offset (TAG_SIMPLEBUFFER_COPLIST_OFFSET) not specified!\n"
			);
			goto fail;
		}
		logWrite("Copperlist offset: %u\n", pManager->uwCopperOffset);
	}

	simpleBufferSetFront(pManager, pFront);
	simpleBufferSetBack(pManager, pBack ? pBack : pFront);

	// Add manager to VPort
	vPortAddManager(pVPort, (tVpManager*)pManager);
	logBlockEnd("simpleBufferCreate()");
	va_end(vaTags);
	return pManager;

fail:
	if(pBack && pBack != pFront) {
		bitmapDestroy(pBack);
	}
	if(pFront) {
		bitmapDestroy(pFront);
	}
	if(pManager) {
		if(pManager->pCameraManager && isCameraCreated) {
			cameraDestroy(pManager->pCameraManager);
		}
		memFree(pManager, sizeof(tSimpleBufferManager));
	}
	logBlockEnd("simpleBufferCreate()");
	va_end(vaTags);
	return 0;
}

void simpleBufferDestroy(tSimpleBufferManager *pManager) {
	logBlockBegin("simpleBufferDestroy()");
	logWrite("Destroying bitmap...\n");
	if(pManager->pBack != pManager->pFront) {
		bitmapDestroy(pManager->pBack);
	}
	bitmapDestroy(pManager->pFront);
	logWrite("Freeing mem...\n");
	memFree(pManager, sizeof(tSimpleBufferManager));
	logBlockEnd("simpleBufferDestroy()");
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
	else {
		uwShift = 0;
	}

	// X offset on bitplane
	ulBplOffs = (pCameraManager->uPos.sUwCoord.uwX >> 4) << 1;

	// Calculate Y movement
	ulBplOffs += pManager->pBack->BytesPerRow*pCameraManager->uPos.sUwCoord.uwY;

	// Copperlist - regen bitplane ptrs, update shift
	// TODO could be unified by using copSetMove in copBlock
	if(pManager->ubFlags & SIMPLEBUFFER_FLAG_COPLIST_RAW) {
		tCopCmd *pCmdList = &pCopList->pBackBfr->pList[pManager->uwCopperOffset];
		copSetMove(&pCmdList[5].sMove, &g_pCustom->bplcon1, uwShift);
		for (i = 0; i < pManager->sCommon.pVPort->ubBPP; ++i) {
			ulPlaneAddr = ((ULONG)pManager->pBack->Planes[i]) + ulBplOffs;
			copSetMove(&pCmdList[6 + i*2 + 0].sMove, &g_pBplFetch[i].uwHi, ulPlaneAddr >> 16);
			copSetMove(&pCmdList[6 + i*2 + 1].sMove, &g_pBplFetch[i].uwLo, ulPlaneAddr & 0xFFFF);
		}
	}
	else {
		pManager->pCopBlock->uwCurrCount = 4; // Rewind to shift cmd pos
		copMove(pCopList, pManager->pCopBlock, &g_pCustom->bplcon1, uwShift);
		for(i = 0; i < pManager->pBack->Depth; ++i) {
			ulPlaneAddr = ((ULONG)pManager->pBack->Planes[i]) + ulBplOffs;
			copMove(pCopList, pManager->pCopBlock, &g_pBplFetch[i].uwHi, ulPlaneAddr >> 16);
			copMove(pCopList, pManager->pCopBlock, &g_pBplFetch[i].uwLo, ulPlaneAddr & 0xFFFF);
		}
	}

	// Swap buffers if needed
	if(pManager->pBack != pManager->pFront) {
		tBitMap *pTmp = pManager->pBack;
		pManager->pBack = pManager->pFront;
		pManager->pFront = pTmp;
	}
}

UBYTE simpleBufferIsRectVisible(
	tSimpleBufferManager *pManager,
	UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight
) {
	return (
		uwX >= pManager->pCameraManager->uPos.sUwCoord.uwX - uwWidth &&
		uwX <= pManager->pCameraManager->uPos.sUwCoord.uwX + pManager->sCommon.pVPort->uwWidth &&
		uwY >= pManager->pCameraManager->uPos.sUwCoord.uwY - uwHeight &&
		uwY <= pManager->pCameraManager->uPos.sUwCoord.uwY + pManager->sCommon.pVPort->uwHeight
	);
}

#endif // AMIGA
