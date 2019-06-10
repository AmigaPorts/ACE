/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/viewport/scrollbuffer.h>
#include <ace/utils/tag.h>
#include <limits.h>

#ifdef AMIGA

static UWORD nearestPowerOf2(UWORD uwVal) {
	// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
	// Decrease by one and fill result with ones, then increase by one
	--uwVal;
	uwVal |= uwVal >> 1;
	uwVal |= uwVal >> 2;
	uwVal |= uwVal >> 4;
	uwVal |= uwVal >> 8;
	++uwVal;
	return uwVal;
}

tScrollBufferManager *scrollBufferCreate(void *pTags, ...) {
	va_list vaTags;
	tCopList *pCopList = 0;
	tScrollBufferManager *pManager;
	UBYTE ubMarginWidth;
	UWORD uwBoundWidth, uwBoundHeight;
	UBYTE ubBitmapFlags;
	UBYTE isCameraCreated = 0;
	UBYTE isDblBuf;

	logBlockBegin("scrollBufferCreate(pTags: %p, ...)", pTags);
	va_start(vaTags, pTags);

	// Init manager
	pManager = memAllocFastClear(sizeof(tScrollBufferManager));
	pManager->sCommon.process = (tVpManagerFn)scrollBufferProcess;
	pManager->sCommon.destroy = (tVpManagerFn)scrollBufferDestroy;
	pManager->sCommon.ubId = VPM_SCROLL;
	logWrite("Addr: %p\n", pManager);

	tVPort *pVPort = (tVPort*)tagGet(pTags, vaTags, TAG_SCROLLBUFFER_VPORT, 0);
	if(!pVPort) {
		logWrite("ERR: No parent viewport (TAG_SCROLLBUFFER_VPORT) specified!\n");
		goto fail;
	}
	pManager->sCommon.pVPort = pVPort;
	logWrite("Parent VPort: %p\n", pVPort);

	ubMarginWidth = tagGet(
		pTags, vaTags, TAG_SCROLLBUFFER_MARGIN_WIDTH, UCHAR_MAX
	);
	if(ubMarginWidth == UCHAR_MAX) {
		logWrite(
			"ERR: No margin width (TAG_SCROLLBUFFER_MARGIN_WIDTH) specified!\n"
		);
		goto fail;
	}

	// Buffer bitmap
	uwBoundWidth = tagGet(
		pTags, vaTags, TAG_SCROLLBUFFER_BOUND_WIDTH, pVPort->uwWidth
	);
	uwBoundHeight = tagGet(
		pTags, vaTags, TAG_SCROLLBUFFER_BOUND_HEIGHT, pVPort->uwHeight
	);
	ubBitmapFlags = tagGet(
		pTags, vaTags, TAG_SCROLLBUFFER_BITMAP_FLAGS, BMF_CLEAR
	);
	logWrite("Bounds: %ux%u\n", uwBoundWidth, uwBoundHeight);

	isDblBuf = tagGet(pTags, vaTags, TAG_SCROLLBUFFER_IS_DBLBUF, 0);

	// Create copperlist entries
	pCopList = pVPort->pView->pCopList;
	if(pCopList->ubMode == COPPER_MODE_BLOCK) {
		pManager->pStartBlock = copBlockCreate(
			pVPort->pView->pCopList, 2 * pVPort->ubBPP + 8,
			// Vertically addition from DiWStrt, horizontally a bit before last fetch.
			// First to set are ddf, modulos & shift so they are changed during fetch.
			0xE2-7*4, pVPort->uwOffsY + 0x2C-1
		);
		pManager->pBreakBlock = copBlockCreate(
			pVPort->pView->pCopList, 2 * pVPort->ubBPP + 2,
			// Dummy position - will be updated
			0x7F, 0xFF
		);
	}
	else {
		// TODO Raw mode
		logWrite("ERR: Unimplemented %s:%d\n", __FILE__, __LINE__);
		goto fail;
	}

	scrollBufferReset(
		pManager, ubMarginWidth, uwBoundWidth, uwBoundHeight,
		ubBitmapFlags, isDblBuf
	);

	// Must be before camera? Shouldn't be as there are priorities on manager list
	vPortAddManager(pVPort, (tVpManager*)pManager);

	// Find camera manager, create if not exists
	pManager->pCamera = (tCameraManager*)vPortGetManager(pVPort, VPM_CAMERA);
	if(!pManager->pCamera) {
		pManager->pCamera = cameraCreate(
			pVPort, 0, 0, uwBoundWidth, uwBoundHeight, isDblBuf
		);
		isCameraCreated = 1;
	}
	else {
		cameraReset(pManager->pCamera, 0,0, uwBoundWidth, uwBoundHeight, isDblBuf);
	}

	// TODO: Update copperlist with current camera pos?

	va_end(vaTags);
	logBlockEnd("scrollBufferCreate");
	return pManager;
fail:
	if(isCameraCreated) {
		cameraDestroy(pManager->pCamera);
	}
	if(pCopList && pManager->pStartBlock) {
		copBlockDestroy(pCopList, pManager->pStartBlock);
		if(pManager->pBreakBlock) {
			copBlockDestroy(pCopList, pManager->pBreakBlock);
		}
	}

	memFree(pManager, sizeof(tScrollBufferManager));
	va_end(vaTags);
	logBlockEnd("scrollBufferCreate");
	return 0;
}

void scrollBufferDestroy(tScrollBufferManager *pManager) {
	logBlockBegin("scrollBufferDestroy(pManager: %p)", pManager);

	copBlockDestroy(pManager->sCommon.pVPort->pView->pCopList, pManager->pStartBlock);
	copBlockDestroy(pManager->sCommon.pVPort->pView->pCopList, pManager->pBreakBlock);

	if(pManager->pFront && pManager->pFront != pManager->pBack) {
		bitmapDestroy(pManager->pFront);
	}
	if(pManager->pBack) {
		bitmapDestroy(pManager->pBack);
	}
	memFree(pManager, sizeof(tScrollBufferManager));

	logBlockEnd("scrollBufferDestroy()");
}

FN_HOTSPOT
void scrollBufferProcess(tScrollBufferManager *pManager) {
	UWORD uwVpHeight = pManager->sCommon.pVPort->uwHeight;

	// convert camera pos to scroll pos
	UWORD uwScrollX = pManager->pCamera->uPos.uwX;
	UWORD uwScrollY = pManager->pCamera->uPos.uwY & (pManager->uwBmAvailHeight - 1);

	// preparations for new copperlist
	UWORD uwShift = (16 - (uwScrollX & 0xF)) & 0xF; // Bitplane shift - single
	uwShift = (uwShift << 4) | uwShift;             // Bitplane shift - PF1 | PF2
	ULONG ulBplAddX = ((uwScrollX - 1) >> 4) << 1;  // must be ULONG!

	tCopList *pCopList = pManager->sCommon.pVPort->pView->pCopList;

	if(pManager->ubFlags & SCROLLBUFFER_FLAG_COPLIST_RAW) {
		// TODO: Raw mode
		logWrite("ERR: Unimplemented %s:%d\n", __FILE__, __LINE__);
	}
	else {
		// Initial copper block
		tCopBlock *pBlock = pManager->pStartBlock;
		pBlock->uwCurrCount = 0; // Rewind copBlock
		copMove(pCopList, pBlock, &g_pCustom->bplcon1, uwShift);
		ULONG ulPlaneOffs = ulBplAddX + (pManager->pBack->BytesPerRow*uwScrollY);
		for(UBYTE i = pManager->sCommon.pVPort->ubBPP; i--;) {
			ULONG ulPlaneAddr = (ULONG)(pManager->pBack->Planes[i]) + ulPlaneOffs;
			copMove(pCopList, pBlock, &g_pBplFetch[i].uwLo, ulPlaneAddr & 0xFFFF);
			copMove(pCopList, pBlock, &g_pBplFetch[i].uwHi, ulPlaneAddr >> 16);
		}
		// TODO setting colors before and after copper instructions moved viewport
		// one line lower on 4bpp - there will be problem on 5 & 6bpp
		pBlock->uwCurrCount += 4; // Add constant part

		// Copper block after Y-break
		pBlock = pManager->pBreakBlock;

		pBlock->uwCurrCount = 0; // Rewind copBlock
		if(pManager->uwBmAvailHeight - uwScrollY <= uwVpHeight) {
			if(pBlock->ubDisabled) {
				copBlockEnable(pCopList, pBlock);
			}
			copBlockWait(pCopList, pBlock, 0, 0x2C + pManager->sCommon.pVPort->uwOffsY + pManager->uwBmAvailHeight - uwScrollY);
			for(UBYTE i = pManager->sCommon.pVPort->ubBPP; i--;) {
				ULONG ulPlaneAddr = (ULONG)(pManager->pBack->Planes[i]) + ulBplAddX;
				copMove(pCopList, pBlock, &g_pBplFetch[i].uwHi, ulPlaneAddr >> 16);
				copMove(pCopList, pBlock, &g_pBplFetch[i].uwLo, ulPlaneAddr & 0xFFFF);
			}
		}
		else {
			copBlockDisable(pCopList, pBlock);
		}
	}

	pManager->uwVpHeightPrev = uwVpHeight;

	// Swap buffers if needed
	if(pManager->pBack != pManager->pFront) {
		tBitMap *pTmp = pManager->pBack;
		pManager->pBack = pManager->pFront;
		pManager->pFront = pTmp;
	}
}

void scrollBufferReset(
	tScrollBufferManager *pManager, UBYTE ubMarginWidth,
	UWORD uwBoundWidth, UWORD uwBoundHeight, UBYTE ubBitmapFlags, UBYTE isDblBuf
) {
	UWORD uwVpWidth, uwVpHeight;
	UWORD uwCalcWidth, uwCalcHeight;
	logBlockBegin(
		"scrollBufferReset(pManager: %p, ubMarginWidth: %hu, uwBoundWidth: %u, uwBoundHeight: %u)",
		pManager, ubMarginWidth, uwBoundWidth, uwBoundHeight
	);
	// Helper vars
	uwVpWidth = pManager->sCommon.pVPort->uwWidth;
	uwVpHeight = pManager->sCommon.pVPort->uwHeight;

	// Reset manager fields
	pManager->uwVpHeightPrev = 0;
	pManager->uBfrBounds.uwX = uwBoundWidth;
	pManager->uBfrBounds.uwY = uwBoundHeight;
	// Optimize avail height to power of two so that modulo can be an AND
	pManager->uwBmAvailHeight = nearestPowerOf2(ubMarginWidth * (blockCountCeil(uwVpHeight, ubMarginWidth) + 4));

	// Destroy old buffer bitmap
	if(pManager->pFront && pManager->pFront != pManager->pBack) {
		bitmapDestroy(pManager->pFront);
	}
	if(pManager->pBack) {
		bitmapDestroy(pManager->pBack);
	}

	// Create new buffer bitmap
	uwCalcWidth = nearestPowerOf2(uwVpWidth + ubMarginWidth*4);
	uwCalcHeight = pManager->uwBmAvailHeight + blockCountCeil(uwBoundWidth, uwVpWidth) - 1;
	pManager->pBack = bitmapCreate(
		uwCalcWidth, uwCalcHeight, pManager->sCommon.pVPort->ubBPP, ubBitmapFlags
	);
	if(isDblBuf) {
		pManager->pFront = bitmapCreate(
			uwCalcWidth, uwCalcHeight, pManager->sCommon.pVPort->ubBPP, ubBitmapFlags
		);
	}
	else {
		pManager->pFront = pManager->pBack;
	}
	pManager->uwModulo = pManager->pBack->BytesPerRow - (uwVpWidth >> 3) - 2;

	// Constant stuff in copperlist
	tCopList *pCopList = pManager->sCommon.pVPort->pView->pCopList;
	if(pManager->ubFlags & SCROLLBUFFER_FLAG_COPLIST_RAW) {
	}
	else {
		tCopBlock *pBlock = pManager->pStartBlock;
		// Set initial WAIT
		copBlockWait(pCopList, pBlock, 0, 0x2C + pManager->sCommon.pVPort->uwOffsY);
		// After bitplane ptrs & bplcon
		pBlock->uwCurrCount = 2 * pManager->sCommon.pVPort->ubBPP + 1;
		copMove(pCopList, pBlock, &g_pCustom->ddfstrt, 0x0030);             // Fetch start
		copMove(pCopList, pBlock, &g_pCustom->bpl1mod, pManager->uwModulo); // Odd planes modulo
		copMove(pCopList, pBlock, &g_pCustom->bpl2mod, pManager->uwModulo); // Even planes modulo
		copMove(pCopList, pBlock, &g_pCustom->ddfstop, 0x00D0);             // Fetch stop
	}

	// Refresh bitplane pointers in copperlist - 2x for double buffered
	scrollBufferProcess(pManager);
	scrollBufferProcess(pManager);

	logBlockEnd("scrollBufferReset()");
}

void scrollBufferBlitMask(
	tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tScrollBufferManager *pDstManager, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, UWORD *pMsk
) {
	// TODO: if area is visible
	wDstY %= pDstManager->uwBmAvailHeight;
	wDstY %= (pDstManager->pBack->BytesPerRow<<3);

	if(wDstY + wHeight <= pDstManager->uwBmAvailHeight) {
		// single blit
		blitUnsafeCopyMask(
			pSrc, wSrcX, wSrcY,
			pDstManager->pBack, wDstX, wDstY,
			wWidth, wHeight, pMsk
		);
	}
	else {
		// split blit in two
		WORD wPartHeight;
		wPartHeight = pDstManager->uwBmAvailHeight - wDstY;
		blitUnsafeCopyMask(
			pSrc, wSrcX, wSrcY,
			pDstManager->pBack, wDstX, wDstY,
			wWidth, wPartHeight, pMsk
		);
		blitUnsafeCopyMask(
			pSrc, wSrcX, wSrcY + wPartHeight,
			pDstManager->pBack, wDstX, 0,
			wWidth, wHeight - wPartHeight, pMsk
		);
	}
}

#endif // AMIGA
