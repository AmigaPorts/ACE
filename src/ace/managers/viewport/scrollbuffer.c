/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/viewport/scrollbuffer.h>
#include <ace/utils/tag.h>
#include <limits.h>


#ifdef AMIGA

tScrollBufferManager *scrollBufferCreate(void *pTags, ...) {
	va_list vaTags;
	tCopList *pCopList;
	tScrollBufferManager *pManager;
	UBYTE ubMarginWidth;
	UWORD uwBoundWidth, uwBoundHeight;
	UBYTE ubBitmapFlags;
	UBYTE isCameraCreated = 0;
	UBYTE isDblBfr;

	logBlockBegin("scrollBufferCreate(pTags: %p, ...)", pTags);
	va_start(vaTags, pTags);

	// Init manager
	pManager = memAllocFastClear(
		sizeof(tScrollBufferManager)
	);
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

	isDblBfr = tagGet(pTags, vaTags, TAG_SCROLLBUFFER_IS_DBLBUF, 0);
	scrollBufferReset(pManager, ubMarginWidth, uwBoundWidth, uwBoundHeight, ubBitmapFlags, isDblBfr);

	// Must be before camera? Shouldn't be as there are priorities on manager list
	vPortAddManager(pVPort, (tVpManager*)pManager);

	// Find camera manager, create if not exists
	pManager->pCameraManager = (tCameraManager*)vPortGetManager(pVPort, VPM_CAMERA);
	if(!pManager->pCameraManager) {
		pManager->pCameraManager = cameraCreate(
			pVPort, 0, 0, uwBoundWidth, uwBoundHeight
		);
	}
	else {
		cameraReset(pManager->pCameraManager, 0,0, uwBoundWidth, uwBoundHeight);
	}

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
		goto fail;
	}

	// TODO: Update copperlist with current camera pos?

	va_end(vaTags);
	logBlockEnd("scrollBufferCreate");
	return pManager;
fail:
	va_end(vaTags);
	logBlockEnd("scrollBufferCreate");
	return 0;
}

void scrollBufferDestroy(tScrollBufferManager *pManager) {
	logBlockBegin("scrollBufferDestroy(pManager: %p)", pManager);

	copBlockDestroy(pManager->sCommon.pVPort->pView->pCopList, pManager->pStartBlock);
	copBlockDestroy(pManager->sCommon.pVPort->pView->pCopList, pManager->pBreakBlock);

	bitmapDestroy(pManager->pBack);
	memFree(pManager, sizeof(tScrollBufferManager));

	logBlockEnd("scrollBufferDestroy()");
}

void scrollBufferProcess(tScrollBufferManager *pManager) {
	UWORD uwVpHeight;

	uwVpHeight = pManager->sCommon.pVPort->uwHeight;
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

	if(pManager->ubFlags & SCROLLBUFFER_FLAG_COPLIST_RAW) {
		// TODO: Raw mode
	}
	else {
		tCopBlock *pBlock = pManager->pStartBlock;
		pBlock->uwCurrCount = 0; // Rewind copBlock
		copBlockWait(pCopList, pBlock, 0, 0x2C + pManager->sCommon.pVPort->uwOffsY);
		copMove(pCopList, pBlock, &g_pCustom->color[0], 0x0F0);
		copMove(pCopList, pBlock, &g_pCustom->bplcon1, uwOffsX);            // Bitplane shift
		ulPlaneOffs = uwScrollX + (pManager->pBack->BytesPerRow*uwScrollY);
		for (i = pManager->sCommon.pVPort->ubBPP; i--;) {
			ulPlaneAddr = (ULONG)(pManager->pBack->Planes[i]) + ulPlaneOffs;
			copMove(pCopList, pBlock, &g_pBplFetch[i].uwLo, ulPlaneAddr & 0xFFFF);
			copMove(pCopList, pBlock, &g_pBplFetch[i].uwHi, ulPlaneAddr >> 16);
		}
		copMove(pCopList, pBlock, &g_pCustom->ddfstrt, 0x30);               // Fetch start
		copMove(pCopList, pBlock, &g_pCustom->bpl1mod, pManager->uwModulo); // Odd planes modulo
		copMove(pCopList, pBlock, &g_pCustom->bpl2mod, pManager->uwModulo); // Even planes modulo
		copMove(pCopList, pBlock, &g_pCustom->ddfstop, 0x00D0);             // Fetch stop
		copMove(pCopList, pBlock, &g_pCustom->color[0], 0x000);

		// Copper block after Y-break
		pBlock = pManager->pBreakBlock;

		pBlock->uwCurrCount = 0; // Rewind copBlock
		if (pManager->uwBmAvailHeight - uwScrollY <= uwVpHeight) {
			// logWrite("Break calc: %u - %u == %u, vpHeight: %u\n", pManager->uwBmAvailHeight, uwScrollY, pManager->uwBmAvailHeight - uwScrollY, uwVpHeight);
			copBlockWait(pCopList, pBlock, 0, 0x2C + pManager->sCommon.pVPort->uwOffsY + pManager->uwBmAvailHeight - uwScrollY);
			// copMove(pCopList, pBlock, &g_pCustom->bplcon1, uwOffsX); // potrzebne?
			copMove(pCopList, pBlock, &g_pCustom->color[0], 0x0F00);
			for (i = pManager->sCommon.pVPort->ubBPP; i--;) {
				ulPlaneAddr = (ULONG)(pManager->pBack->Planes[i]) + uwScrollX;
				copMove(pCopList, pBlock, &g_pBplFetch[i].uwHi, ulPlaneAddr >> 16);
				copMove(pCopList, pBlock, &g_pBplFetch[i].uwLo, ulPlaneAddr & 0xFFFF);
			}
			copMove(pCopList, pBlock, &g_pCustom->color[0], 0x0000);
		}
		else {
			copBlockWait(pCopList, pBlock, 0x7F, 0xFF);
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
	UWORD uwBoundWidth, UWORD uwBoundHeight, UBYTE ubBitmapFlags, UBYTE isDblBfr
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
	pManager->uBfrBounds.sUwCoord.uwX = uwBoundWidth;
	pManager->uBfrBounds.sUwCoord.uwY = uwBoundHeight;
	// TODO optimize avail height to power of two so that modulo can be an AND
	pManager->uwBmAvailHeight = ubMarginWidth * (blockCountCeil(uwVpHeight, ubMarginWidth) + 4);

	// Destroy old buffer bitmap
	if(pManager->pFront != pManager->pBack) {
		bitmapDestroy(pManager->pFront);
	}
	if(pManager->pBack) {
		bitmapDestroy(pManager->pBack);
	}

	// Create new buffer bitmap
	uwCalcWidth = uwVpWidth + ubMarginWidth*4;
	uwCalcHeight = pManager->uwBmAvailHeight + blockCountCeil(uwBoundWidth, uwVpWidth) - 1;
	pManager->pBack = bitmapCreate(
		uwCalcWidth, uwCalcHeight, pManager->sCommon.pVPort->ubBPP, ubBitmapFlags
	);
	if(isDblBfr) {
		pManager->pFront = bitmapCreate(
			uwCalcWidth, uwCalcHeight, pManager->sCommon.pVPort->ubBPP, ubBitmapFlags
		);
	}
	else {
		pManager->pFront = pManager->pBack;
	}
	pManager->uwModulo = pManager->pBack->BytesPerRow - (uwVpWidth >> 3) - 2;

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
