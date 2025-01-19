/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/viewport/simplebuffer.h>
#include <proto/exec.h>
#include <ace/utils/tag.h>
#include <ace/utils/extview.h>
#include <ace/generic/screen.h> // Has the look up table for the COPPER_X_WAIT values.
#ifdef AMIGA


// Flags for internal usage.
#define SIMPLEBUFFER_FLAG_X_SCROLLABLE 1
#define SIMPLEBUFFER_FLAG_COPLIST_RAW  2

static void setBitplanePtrs(tCopCmd *pCmds, const tBitMap *pBitmap, LONG lBplOffs) {
	for(UBYTE i = 0; i < pBitmap->Depth; ++i) {
		ULONG ulPlaneAddr = (ULONG)pBitmap->Planes[i] + lBplOffs;
		copSetMove(&(pCmds++)->sMove, &g_pBplFetch[i].uwHi, ulPlaneAddr >> 16);
		copSetMove(&(pCmds++)->sMove, &g_pBplFetch[i].uwLo, ulPlaneAddr & 0xFFFF);
	}
}

static void updateBitplanePtrs(
	tCopCmd *pCmds, const tBitMap *pBitmap, LONG lBplOffs
) {
	for(UBYTE i = 0; i < pBitmap->Depth; ++i) {
		ULONG ulPlaneAddr = ((ULONG)pBitmap->Planes[i]) + lBplOffs;
		copSetMoveVal(&(pCmds++)->sMove, ulPlaneAddr >> 16);
		copSetMoveVal(&(pCmds++)->sMove, ulPlaneAddr & 0xFFFF);
	}
}

static void simpleBufferInitializeCopperList(
	tSimpleBufferManager *pManager, UBYTE isScrollX
) {
	pManager->uBfrBounds.uwX = bitmapGetByteWidth(pManager->pFront) << 3;
	pManager->uBfrBounds.uwY = pManager->pFront->Rows;
	UWORD uwModulo = pManager->pFront->BytesPerRow - (pManager->sCommon.pVPort->uwWidth >> 3);

	// http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0085.html
	UWORD uwDDfStrt = (pManager->sCommon.pVPort->pView->ubPosX + 15) / 2 - 16;
	UWORD uwDDfStop = uwDDfStrt + ((pManager->sCommon.pVPort->pView->uwWidth / 16) - 1) * 8;
	if(pManager->sCommon.pVPort->eFlags & VP_FLAG_HIRES) {
		uwDDfStrt += 4;
		uwDDfStop += 4;
	}

	if(
		!isScrollX || pManager->uBfrBounds.uwX <= pManager->sCommon.pVPort->uwWidth
	) {
		pManager->ubFlags &= ~SIMPLEBUFFER_FLAG_X_SCROLLABLE;
	}
	else {
		pManager->ubFlags |= SIMPLEBUFFER_FLAG_X_SCROLLABLE;
		if(pManager->sCommon.pVPort->eFlags & VP_FLAG_HIRES) {
			uwDDfStrt -= 8; // two more hires 4-part bitplane fetch pattern: 3120
			uwModulo -= 4;
		}
		else {
			uwDDfStrt -= 8; // one more lores 8-part bitplane fetch pattern: x351x240
			uwModulo -= 2;
		}
	}
	logWrite("DDFSTRT: %04X, DDFSTOP: %04X, Modulo: %u\n", uwDDfStrt, uwDDfStop, uwModulo);

	// if X scroll is enabled then it needs to start one word early
	ULONG ulBplOffs = 0;
	if(pManager->ubFlags & SIMPLEBUFFER_FLAG_X_SCROLLABLE) {
		if(pManager->sCommon.pVPort->eFlags & VP_FLAG_HIRES) {
			ulBplOffs = -4;
		}
		else {
			ulBplOffs = -2;
		}
	}

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
		copSetWait(&pCmdList[0].sWait, s_pCopperWaitXByBitplanes[pManager->sCommon.pVPort->ubBpp], (
			pManager->sCommon.pVPort->uwOffsY +
			pManager->sCommon.pVPort->pView->ubPosY -1
		));
		copSetMove(&pCmdList[1].sMove, &g_pCustom->ddfstop, uwDDfStop); // Data fetch
		copSetMove(&pCmdList[2].sMove, &g_pCustom->ddfstrt, uwDDfStrt);
		copSetMove(&pCmdList[3].sMove, &g_pCustom->bpl1mod, uwModulo); // Bitplane modulo
		copSetMove(&pCmdList[4].sMove, &g_pCustom->bpl2mod, uwModulo);
		copSetMove(&pCmdList[5].sMove, &g_pCustom->bplcon1, 0); // Shift: 0

		// Copy to front buffer since it needs initialization there too
		for(UWORD i = pManager->uwCopperOffset; i < pManager->uwCopperOffset + 6; ++i) {
			pCopList->pFrontBfr->pList[i].ulCode = pCopList->pBackBfr->pList[i].ulCode;
		}

		// Proper back buffer pointers
		setBitplanePtrs(&pCmdList[6], pManager->pFront, ulBplOffs);

		// Proper front buffer pointers
		pCmdList = &pCopList->pFrontBfr->pList[pManager->uwCopperOffset];
		setBitplanePtrs(&pCmdList[6], pManager->pBack, ulBplOffs);
	}
	else {
		tCopBlock *pBlock = pManager->pCopBlock;
		pBlock->uwCurrCount = 0; // Rewind to beginning
		copMove(pCopList, pBlock, &g_pCustom->ddfstop, uwDDfStop); // Data fetch
		copMove(pCopList, pBlock, &g_pCustom->ddfstrt, uwDDfStrt);
		copMove(pCopList, pBlock, &g_pCustom->bpl1mod, uwModulo); // Bitplane modulo
		copMove(pCopList, pBlock, &g_pCustom->bpl2mod, uwModulo);
		copMove(pCopList, pBlock, &g_pCustom->bplcon1, 0); // Shift: 0
		for (UBYTE i = 0; i < pManager->sCommon.pVPort->ubBpp; ++i) {
			ULONG ulPlaneAddr = (ULONG)pManager->pBack->Planes[i] + ulBplOffs;
			copMove(pCopList, pBlock, &g_pBplFetch[i].uwHi, ulPlaneAddr >> 16);
			copMove(pCopList, pBlock, &g_pBplFetch[i].uwLo, ulPlaneAddr & 0xFFFF);
		}
	}
}

static void simpleBufferSetBack(tSimpleBufferManager *pManager, tBitMap *pBack) {
#if defined(ACE_DEBUG)
	if(pManager->pBack && pManager->pBack->Depth != pBack->Depth) {
		logWrite("ERR: buffer bitmaps differ in BPP\n");
		return;
	}
#endif
	pManager->pBack = pBack;
}

static UWORD simpleBufferCalcBplOffsAndShift(tSimpleBufferManager *pManager, ULONG *pBplOffs) {
	// Calculate X movement: bitplane shift, starting word to fetch
	UWORD uwShift;
	if(pManager->ubFlags & SIMPLEBUFFER_FLAG_X_SCROLLABLE) {
		uwShift = (16 - (pManager->pCamera->uPos.uwX & 0xF)) & 0xF; // Bitplane shift - single
		*pBplOffs = ((pManager->pCamera->uPos.uwX - 1) >> 4) << 1;  // Must be ULONG!
		if(pManager->sCommon.pVPort->eFlags & VP_FLAG_HIRES) {
			uwShift >>= 1; // Usable scroll values are 0..7, shifts 2 pixels per value
			*pBplOffs -= 2; // Fetch 4 bytes (2 words) in scrolling instead of 2 (4)
		}
		uwShift = (uwShift << 4) | uwShift; // Convert to bplcon format - PF1 | PF2
	}
	else {
		uwShift = 0;
		*pBplOffs = (pManager->pCamera->uPos.uwX >> 4) << 1;
	}

	// Calculate Y movement
	*pBplOffs += pManager->pBack->BytesPerRow * pManager->pCamera->uPos.uwY;
	return uwShift;
}

void simpleBufferSetFront(tSimpleBufferManager *pManager, tBitMap *pFront) {
	logBlockBegin(
		"simpleBufferSetFront(pManager: %p, pFront: %p)",
		pManager, pFront
	);
#if defined(ACE_DEBUG)
	if(pManager->pFront && pManager->pFront->Depth != pFront->Depth) {
		logWrite("ERR: buffer bitmaps differ in BPP\n");
		return;
	}
#endif
	pManager->pFront = pFront;
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
		logWrite("ERR: No parent viewport (TAG_SIMPLEBUFFER_VPORT) specified\n");
		goto fail;
	}
	pManager->sCommon.pVPort = pVPort;
	logWrite("Parent VPort: %p\n", pVPort);

	// Buffer bitmap
	uwBoundWidth = tagGet(
		pTags, vaTags, TAG_SIMPLEBUFFER_BOUND_WIDTH, pVPort->uwWidth
	);
	uwBoundHeight = tagGet(
		pTags, vaTags, TAG_SIMPLEBUFFER_BOUND_HEIGHT, pVPort->uwHeight
	);
	ubBitmapFlags = tagGet(
		pTags, vaTags, TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR
	);
	logWrite("Bounds: %ux%u\n", uwBoundWidth, uwBoundHeight);
	pFront = bitmapCreate(
		uwBoundWidth, uwBoundHeight, pVPort->ubBpp, ubBitmapFlags
	);
	if(!pFront) {
		logWrite("ERR: Can't alloc buffer bitmap\n");
		goto fail;
	}

	UBYTE isDblBfr = tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_IS_DBLBUF, 0);
	if(isDblBfr) {
		pBack = bitmapCreate(
			uwBoundWidth, uwBoundHeight, pVPort->ubBpp, ubBitmapFlags
		);
		if(!pBack) {
			logWrite("ERR: Can't alloc buffer bitmap\n");
			goto fail;
		}
	}

	// Find camera manager, create if not exists
	pManager->pCamera = (tCameraManager*)vPortGetManager(pVPort, VPM_CAMERA);
	if(!pManager->pCamera) {
		isCameraCreated = 1;
		pManager->pCamera = cameraCreate(
			pVPort, 0, 0, uwBoundWidth, uwBoundHeight, isDblBfr
		);
	}

	pCopList = pVPort->pView->pCopList;
	if(pCopList->ubMode == COPPER_MODE_BLOCK) {
		// CopBlock contains: bitplanes + shiftX
		pManager->pCopBlock = copBlockCreate(
			// WAIT is already in copBlock so 1 instruction less
			pCopList, simpleBufferGetRawCopperlistInstructionCount(pVPort->ubBpp) - 1,
			// Vertically addition from DiWStrt, horizontally just so that 6bpp can be set up.
			// First to set are ddf, modulos & shift so they are changed during fetch.
			s_pCopperWaitXByBitplanes[pVPort->ubBpp],
			 pVPort->uwOffsY + pVPort->pView->ubPosY - 1
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
				"ERR: Copperlist offset (TAG_SIMPLEBUFFER_COPLIST_OFFSET) not specified\n"
			);
			goto fail;
		}
		logWrite("Copperlist offset: %u\n", pManager->uwCopperOffset);
	}

	UBYTE isScrollX = tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_USE_X_SCROLLING, 1);
	simpleBufferSetFront(pManager, pFront);
	simpleBufferSetBack(pManager, pBack ? pBack : pFront);
	simpleBufferInitializeCopperList(pManager, isScrollX);

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
		if(pManager->pCamera && isCameraCreated) {
			cameraDestroy(pManager->pCamera);
		}
		memFree(pManager, sizeof(tSimpleBufferManager));
	}
	logBlockEnd("simpleBufferCreate()");
	va_end(vaTags);
	return 0;
}

void simpleBufferDestroy(tSimpleBufferManager *pManager) {
	logBlockBegin("simpleBufferDestroy()");
	if(!(pManager->ubFlags & SIMPLEBUFFER_FLAG_COPLIST_RAW)) {
		copBlockDestroy(
			pManager->sCommon.pVPort->pView->pCopList, pManager->pCopBlock
		);
	}
	if(pManager->pBack != pManager->pFront) {
		bitmapDestroy(pManager->pBack);
	}
	bitmapDestroy(pManager->pFront);
	memFree(pManager, sizeof(tSimpleBufferManager));
	logBlockEnd("simpleBufferDestroy()");
}

void simpleBufferProcess(tSimpleBufferManager *pManager) {
	const tCameraManager *pCamera = pManager->pCamera;
	tCopList *pCopList = pManager->sCommon.pVPort->pView->pCopList;

	// Copperlist - regen bitplane ptrs, update shift
	// TODO could be unified by using copSetMove in copBlock
	if((pManager->ubFlags & SIMPLEBUFFER_FLAG_COPLIST_RAW)) {
		if(cameraIsMoved(pCamera)) {
			// At least two updates in the row - double-buffered copperlist
			// must update two buffers with state of single-buffered vport manager.
			pManager->ubDirtyCounter = 2;
		}
		if(pManager->ubDirtyCounter) {
			ULONG ulBplOffs;
			UWORD uwShift = simpleBufferCalcBplOffsAndShift(pManager, &ulBplOffs);
			tCopCmd *pCmdList = &pCopList->pBackBfr->pList[pManager->uwCopperOffset];
			copSetMoveVal(&pCmdList[5].sMove, uwShift);
			updateBitplanePtrs(&pCmdList[6], pManager->pBack, ulBplOffs);
			--pManager->ubDirtyCounter;
		}
	}
	else if(pManager->pBack != pManager->pFront || cameraIsMoved(pCamera)) {
		// In double buffering, we can't really check for camera being moved, since
		// copBlock needs to change its bitplane pointers value each frame and
		// copperlist needs refreshing.
		ULONG ulBplOffs;
		UWORD uwShift = simpleBufferCalcBplOffsAndShift(pManager, &ulBplOffs);
		pManager->pCopBlock->uwCurrCount = 4; // Rewind to shift cmd pos
		copMove(pCopList, pManager->pCopBlock, &g_pCustom->bplcon1, uwShift);
		for(UBYTE i = 0; i < pManager->pBack->Depth; ++i) {
			ULONG ulPlaneAddr = ((ULONG)pManager->pBack->Planes[i]) + ulBplOffs;
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
		uwX >= pManager->pCamera->uPos.uwX - uwWidth &&
		uwX <= pManager->pCamera->uPos.uwX + pManager->sCommon.pVPort->uwWidth &&
		uwY >= pManager->pCamera->uPos.uwY - uwHeight &&
		uwY <= pManager->pCamera->uPos.uwY + pManager->sCommon.pVPort->uwHeight
	);
}

UBYTE simpleBufferGetRawCopperlistInstructionCount(UBYTE ubBpp) {
	UBYTE ubInstructionCount = (
		1 +       // WAIT cmd
		2 +       // DDFSTOP / DDFSTART setup
		2 +       // Odd / even modulos
		1 +       // X-shift setup in bplcon
		2 * ubBpp // 2 * 16-bit MOVE for each bitplane
	);
	return ubInstructionCount;
}

#endif // AMIGA
