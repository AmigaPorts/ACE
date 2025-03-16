/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/viewport/scrollbuffer.h>
#include <ace/utils/tag.h>
#include <ace/generic/screen.h> // Has the look up table for the COPPER_X_WAIT values.
#include <limits.h>

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
	logBlockBegin("scrollBufferCreate(pTags: %p, ...)", pTags);

	va_list vaTags;
	va_start(vaTags, pTags);

	// Init manager
	tScrollBufferManager *pManager = memAllocFastClear(sizeof(tScrollBufferManager));
	pManager->sCommon.process = (tVpManagerFn)scrollBufferProcess;
	pManager->sCommon.destroy = (tVpManagerFn)scrollBufferDestroy;
	pManager->sCommon.ubId = VPM_SCROLL;
	logWrite("Addr: %p\n", pManager);

	UBYTE isCameraCreated = 0;
	tCopList *pCopList = 0;
	tVPort *pVPort = (tVPort*)tagGet(pTags, vaTags, TAG_SCROLLBUFFER_VPORT, 0);
	if(!pVPort) {
		logWrite("ERR: No parent viewport (TAG_SCROLLBUFFER_VPORT) specified\n");
		goto fail;
	}
	pManager->sCommon.pVPort = pVPort;
	logWrite("Parent VPort: %p\n", pVPort);

	UBYTE ubMarginWidth = tagGet(
		pTags, vaTags, TAG_SCROLLBUFFER_MARGIN_WIDTH, UCHAR_MAX
	);
	if(ubMarginWidth == UCHAR_MAX) {
		logWrite(
			"ERR: No margin width (TAG_SCROLLBUFFER_MARGIN_WIDTH) specified\n"
		);
		goto fail;
	}

	// Buffer bitmap
	UWORD uwBoundWidth = tagGet(
		pTags, vaTags, TAG_SCROLLBUFFER_BOUND_WIDTH, pVPort->uwWidth
	);
	UWORD uwBoundHeight = tagGet(
		pTags, vaTags, TAG_SCROLLBUFFER_BOUND_HEIGHT, pVPort->uwHeight
	);
	UBYTE ubBitmapFlags = tagGet(
		pTags, vaTags, TAG_SCROLLBUFFER_BITMAP_FLAGS, BMF_CLEAR
	);
	logWrite("Bounds: %ux%u\n", uwBoundWidth, uwBoundHeight);

	UBYTE isDblBuf = tagGet(pTags, vaTags, TAG_SCROLLBUFFER_IS_DBLBUF, 0);

	// Create copperlist entries
	pCopList = pVPort->pView->pCopList;
	if(pCopList->ubMode == COPPER_MODE_BLOCK) {
		pManager->pStartBlock = copBlockCreate(
			pVPort->pView->pCopList, 2 * pVPort->ubBpp + 8,
			// Vertically addition from DiWStrt, horizontally just so that 6bpp can be set up.
			// First to set are ddf, modulos & shift so they are changed during fetch.
			s_pCopperWaitXByBitplanes[pVPort->ubBpp], pVPort->uwOffsY + pVPort->pView->ubPosY -1
		);
		pManager->pBreakBlock = copBlockCreate(
			pVPort->pView->pCopList, 2 * pVPort->ubBpp + 2,
			// Dummy position - will be updated
			0x7F, 0xFF
		);
	}
	else {
		const UWORD uwInvalidCopOffs = -1;
		pManager->ubFlags |= SCROLLBUFFER_FLAG_COPLIST_RAW;
		pManager->uwCopperOffsetStart = tagGet(
			pTags, vaTags, TAG_SCROLLBUFFER_COPLIST_OFFSET_START, uwInvalidCopOffs
		);
		if(pManager->uwCopperOffsetStart == uwInvalidCopOffs) {
			logWrite(
				"ERR: Copperlist offset (TAG_SCROLLBUFFER_COPLIST_OFFSET_START) not specified\n"
			);
			goto fail;
		}
		pManager->uwCopperOffsetBreak = tagGet(
			pTags, vaTags, TAG_SCROLLBUFFER_COPLIST_OFFSET_BREAK, uwInvalidCopOffs
		);
		if(pManager->uwCopperOffsetBreak == uwInvalidCopOffs) {
			logWrite(
				"ERR: Copperlist offset (TAG_SCROLLBUFFER_COPLIST_OFFSET_BREAK) not specified\n"
			);
			goto fail;
		}
		logWrite("Copperlist offsets: start: %u, break: %u\n",
				 pManager->uwCopperOffsetStart,
				 pManager->uwCopperOffsetBreak);
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
	if(pCopList && pCopList->ubMode == COPPER_MODE_BLOCK && pManager->pStartBlock) {
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

	if(!(pManager->ubFlags & SCROLLBUFFER_FLAG_COPLIST_RAW)) {
		copBlockDestroy(pManager->sCommon.pVPort->pView->pCopList, pManager->pStartBlock);
		copBlockDestroy(pManager->sCommon.pVPort->pView->pCopList, pManager->pBreakBlock);
	}

	if(pManager->pFront && pManager->pFront != pManager->pBack) {
		bitmapDestroy(pManager->pFront);
	}
	if(pManager->pBack) {
		bitmapDestroy(pManager->pBack);
	}
	memFree(pManager, sizeof(tScrollBufferManager));

	logBlockEnd("scrollBufferDestroy()");
}

// All functions to update the raw copperlist below must be matched for the copperlist layout and size

UBYTE scrollBufferGetRawCopperlistInstructionCountStart(UBYTE ubBpp) {
	return (
		1 + // initial WAIT
	 	1 + 2 * ubBpp + // bitplane ptrs & bplcon commands
		4 // After bitplane ptrs & bplcon
	);
}

UBYTE scrollBufferGetRawCopperlistInstructionCountBreak(UBYTE ubBpp) {
	return (
		4 + // copper jump location, skip, strobe
	 	1 + 2 * ubBpp // WAIT & bitplane ptrs
	);
}

static void resetStartCopperlist(tCopCmd *pCmds, tScrollBufferManager *pManager) {
	UWORD uwOffsY = (
		pManager->sCommon.pVPort->pView->ubPosY +
		pManager->sCommon.pVPort->uwOffsY -1
	);
	UBYTE ubBpp = pManager->sCommon.pVPort->ubBpp;
	UBYTE i = 0;
	copSetWait(&pCmds[i++].sWait, s_pCopperWaitXByBitplanes[ubBpp], uwOffsY);
	// prepare bitplane ptrs & bplcon commands. will be updated in process
	copSetMove(&pCmds[i++].sMove, &g_pCustom->bplcon1, 0);
	for(UBYTE j = 0; j < ubBpp; j++) {
		copSetMove(&pCmds[i++].sMove, &g_pBplFetch[j].uwHi, 0);
		copSetMove(&pCmds[i++].sMove, &g_pBplFetch[j].uwLo, 0);
	}
	// After bitplane ptrs & bplcon
	copSetMove(&pCmds[i++].sMove, &g_pCustom->ddfstrt, pManager->uwDDfStrt); // Fetch start
	copSetMove(&pCmds[i++].sMove, &g_pCustom->bpl1mod, pManager->uwModulo); // Odd planes modulo
	copSetMove(&pCmds[i++].sMove, &g_pCustom->bpl2mod, pManager->uwModulo); // Even planes modulo
	copSetMove(&pCmds[i++].sMove, &g_pCustom->ddfstop, pManager->uwDDfStop); // Fetch stop
}

static void updateStartCopperlist(tCopCmd *pCmds, const tBitMap *pBitmap, const UWORD uwShift, const ULONG ulPlaneOffs) {
	UBYTE i = 1; // the wait at the beginning of the start block doesn't change
	copSetMoveVal(&pCmds[i++].sMove, uwShift);
	for(UBYTE j = 0; j < pBitmap->Depth; j++) {
		ULONG ulPlaneAddr = (ULONG)(pBitmap->Planes[j]) + ulPlaneOffs;
		copSetMoveVal(&pCmds[i++].sMove, ulPlaneAddr >> 16);
		copSetMoveVal(&pCmds[i++].sMove, ulPlaneAddr & 0xFFFF);
	}
}

static void resetBreakCopperlist(tCopCmd *pCmds, const UWORD uwOffsY, const UBYTE ubBpp) {
	UBYTE i = 0;
	// copper jump location & strobe to jump past the break block
	UBYTE offset = scrollBufferGetRawCopperlistInstructionCountBreak(ubBpp);
	copSetMove(&pCmds[i++].sMove, &g_pCop2Lc->uwHi, (ULONG)(pCmds + offset) >> 16);
	copSetMove(&pCmds[i++].sMove, &g_pCop2Lc->uwLo, (ULONG)(pCmds + offset) & 0xFFFF);
	// prepare a wait/skip instruction that will always be true immediately
	copSetWait(&pCmds[i++].sWait, 0, 0);
	// strobe
	copSetMove(&pCmds[i++].sMove, &g_pCustom->copjmp2, 1);

	// wait & bitplane ptrs
	copSetWait(&pCmds[i++].sWait, 0, uwOffsY);
	for(UBYTE j = 0; j < ubBpp; j++) {
		copSetMove(&pCmds[i++].sMove, &g_pBplFetch[j].uwHi, 0);
		copSetMove(&pCmds[i++].sMove, &g_pBplFetch[j].uwLo, 0);
	}
}

static void updateBreakCopperlist(tCopCmd *pCmds, const tBitMap *pBitmap, const UWORD uwSplitPos, const ULONG ulBplAddX) {
	pCmds[2].sWait.bfIsSkip = 1; // skip the jump so we have this block enabled

	UBYTE i = 4; // the first 4 bytes are cop2lch, cop2lcl, SKIP/WAIT, cop2jmp
	pCmds[i++].sWait.bfWaitY = uwSplitPos;
	for(UBYTE j = 0; j < pBitmap->Depth; j++) {
		ULONG ulPlaneAddr = (ULONG)(pBitmap->Planes[j]) + ulBplAddX;
		copSetMoveVal(&pCmds[i++].sMove, ulPlaneAddr >> 16);
		copSetMoveVal(&pCmds[i++].sMove, ulPlaneAddr & 0xFFFF);
	}
}

static void disableBreakCopperlist(tCopCmd *pCmds) {
	// disable the block by setting the block to skip the jump if we're past the beginning
	// of the start block (which we always are)
	pCmds[2].sWait.bfIsSkip = 0; // do not skip the jump
}

FN_HOTSPOT
void scrollBufferProcess(tScrollBufferManager *pManager) {
	UWORD uwVpHeight = pManager->sCommon.pVPort->uwHeight;

	// TODO: use deltaX and deltaY to decide if we need to update anything

	// convert camera pos to scroll pos
	UWORD uwScrollX = pManager->pCamera->uPos.uwX;
	UWORD uwScrollY = SCROLLBUFFER_HEIGHT_MODULO(
		pManager->pCamera->uPos.uwY, pManager->uwBmAvailHeight
	);

	// preparations for new copperlist
	UWORD uwShift = (16 - (uwScrollX & 0xF)) & 0xF; // Bitplane shift - single
	ULONG ulBplAddX = ((uwScrollX - 1) >> 4) << 1;  // must be ULONG!
	if(pManager->sCommon.pVPort->eFlags & VP_FLAG_HIRES) {
		uwShift >>= 1; // Usable scroll values are 0..7, shifts 2 pixels per value
		ulBplAddX -= 2; // Fetch 4 bytes (2 words) in scrolling instead of 2 (4)
	}
	uwShift = (uwShift << 4) | uwShift;             // Bitplane shift - PF1 | PF2

	tCopList *pCopList = pManager->sCommon.pVPort->pView->pCopList;

	ULONG ulPlaneOffs = ulBplAddX + (pManager->pBack->BytesPerRow*uwScrollY);
	if(pManager->ubFlags & SCROLLBUFFER_FLAG_COPLIST_RAW) {
		updateStartCopperlist(&pCopList->pBackBfr->pList[pManager->uwCopperOffsetStart], pManager->pBack, uwShift, ulPlaneOffs);

		tCopCmd *pCmdListBreak = &pCopList->pBackBfr->pList[pManager->uwCopperOffsetBreak];
		if(pManager->uwBmAvailHeight - uwScrollY < uwVpHeight) {
			updateBreakCopperlist(
				pCmdListBreak, pManager->pBack,
				pManager->sCommon.pVPort->pView->ubPosY +
				pManager->sCommon.pVPort->uwOffsY +
				pManager->uwBmAvailHeight - uwScrollY - 1,
				ulBplAddX
			);
		}
		else {
			disableBreakCopperlist(pCmdListBreak);
		}
	}
	else {
		// Initial copper block
		tCopBlock *pBlock = pManager->pStartBlock;
		pBlock->uwCurrCount = 0; // Rewind copBlock
		copMove(pCopList, pBlock, &g_pCustom->bplcon1, uwShift);
		for(UBYTE i = pManager->sCommon.pVPort->ubBpp; i--;) {
			ULONG ulPlaneAddr = (ULONG)(pManager->pBack->Planes[i]) + ulPlaneOffs;
			copMove(pCopList, pBlock, &g_pBplFetch[i].uwHi, ulPlaneAddr >> 16);
			copMove(pCopList, pBlock, &g_pBplFetch[i].uwLo, ulPlaneAddr & 0xFFFF);
		}
		// NOTE trying to set colors before and after copper instructions made vport
		// move one line lower on 4bpp - there will be problem on 5 & 6bpp
		pBlock->uwCurrCount += 4; // Add constant part

		// Copper block after Y-break
		pBlock = pManager->pBreakBlock;

		pBlock->uwCurrCount = 0; // Rewind copBlock
		if(pManager->uwBmAvailHeight - uwScrollY < uwVpHeight) {
			if(pBlock->ubDisabled) {
				copBlockEnable(pCopList, pBlock);
			}
			copBlockWait(pCopList, pBlock, s_pCopperWaitXByBitplanes[pManager->sCommon.pVPort->ubBpp], (
				pManager->sCommon.pVPort->pView->ubPosY +
				pManager->sCommon.pVPort->uwOffsY +
				pManager->uwBmAvailHeight - uwScrollY - 1
			));
			for(UBYTE i = pManager->sCommon.pVPort->ubBpp; i--;) {
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
	logBlockBegin(
		"scrollBufferReset(pManager: %p, ubMarginWidth: %hu, uwBoundWidth: %u, uwBoundHeight: %u)",
		pManager, ubMarginWidth, uwBoundWidth, uwBoundHeight
	);
	// Helper vars
	UWORD uwVpWidth = pManager->sCommon.pVPort->uwWidth;
	UWORD uwVpHeight = pManager->sCommon.pVPort->uwHeight;

	// Reset manager fields
	pManager->uwVpHeightPrev = 0;
	pManager->uBfrBounds.uwX = uwBoundWidth;
	pManager->uBfrBounds.uwY = uwBoundHeight;
	// Optimize avail height to power of two so that modulo can be an AND
	pManager->uwBmAvailHeight =
		ubMarginWidth * (blockCountCeil(uwVpHeight, ubMarginWidth) + 2 * (ACE_SCROLLBUFFER_Y_MARGIN_SIZE + SCROLLBUFFER_Y_DRAW_MARGIN_SIZE));
#if defined(ACE_SCROLLBUFFER_POT_BITMAP_HEIGHT)
	pManager->uwBmAvailHeight = nearestPowerOf2(pManager->uwBmAvailHeight);
#endif

	// Destroy old buffer bitmap
	if(pManager->pFront && pManager->pFront != pManager->pBack) {
		bitmapDestroy(pManager->pFront);
	}
	if(pManager->pBack) {
		bitmapDestroy(pManager->pBack);
	}

	// Create new buffer bitmap
	UWORD uwCalcWidth = uwVpWidth + ubMarginWidth * 2 * (ACE_SCROLLBUFFER_X_MARGIN_SIZE + SCROLLBUFFER_X_DRAW_MARGIN_SIZE);
	UWORD uwCalcHeight = pManager->uwBmAvailHeight + blockCountCeil(uwBoundWidth, uwVpWidth) - 1;
	pManager->pBack = bitmapCreate(
		uwCalcWidth, uwCalcHeight, pManager->sCommon.pVPort->ubBpp, ubBitmapFlags
	);
	if(isDblBuf) {
		pManager->pFront = bitmapCreate(
			uwCalcWidth, uwCalcHeight, pManager->sCommon.pVPort->ubBpp, ubBitmapFlags
		);
	}
	else {
		pManager->pFront = pManager->pBack;
	}
	pManager->uwModulo = pManager->pBack->BytesPerRow - (uwVpWidth >> 3) - 2;

	pManager->uwDDfStrt = (pManager->sCommon.pVPort->pView->ubPosX + 15) / 2 - 16;
	pManager->uwDDfStop = pManager->uwDDfStrt + ((pManager->sCommon.pVPort->pView->uwWidth / 16) - 1) * 8;
	pManager->uwDDfStrt -= 8; // for scroll reasons
	if(pManager->sCommon.pVPort->eFlags & VP_FLAG_HIRES) {
		// Start/stop one 4-step bitplane fetch pattern later: 3120
		pManager->uwDDfStrt += 4;
		pManager->uwDDfStop += 4;

		// One word more for fetch
		pManager->uwModulo -= 2;
	}
	logWrite("DDFSTRT: %04X, DDFSTOP: %04X, Modulo: %u\n", pManager->uwDDfStrt, pManager->uwDDfStop, pManager->uwModulo);

	// Constant stuff in copperlist
	tCopList *pCopList = pManager->sCommon.pVPort->pView->pCopList;
	if(pManager->ubFlags & SCROLLBUFFER_FLAG_COPLIST_RAW) {
		resetStartCopperlist(
			&pCopList->pBackBfr->pList[pManager->uwCopperOffsetStart],
			pManager
		);
		resetBreakCopperlist(
			&pCopList->pBackBfr->pList[pManager->uwCopperOffsetBreak],
			pManager->sCommon.pVPort->pView->ubPosY +
			pManager->sCommon.pVPort->uwOffsY - 1,
			pManager->sCommon.pVPort->ubBpp);
		// again for double bufferred
		resetStartCopperlist(
			&pCopList->pFrontBfr->pList[pManager->uwCopperOffsetStart],
			pManager
		);
		resetBreakCopperlist(
			&pCopList->pFrontBfr->pList[pManager->uwCopperOffsetBreak],
			pManager->sCommon.pVPort->pView->ubPosY +
			pManager->sCommon.pVPort->uwOffsY - 1,
			pManager->sCommon.pVPort->ubBpp
		);
	}
	else {
		tCopBlock *pBlock = pManager->pStartBlock;
		// Set initial WAIT
		copBlockWait(pCopList, pBlock, s_pCopperWaitXByBitplanes[pManager->sCommon.pVPort->ubBpp], (
			pManager->sCommon.pVPort->pView->ubPosY +
			pManager->sCommon.pVPort->uwOffsY - 1
		));
		// After bitplane ptrs & bplcon
		pBlock->uwCurrCount = 2 * pManager->sCommon.pVPort->ubBpp + 1;
		copMove(pCopList, pBlock, &g_pCustom->ddfstrt, pManager->uwDDfStrt); // Fetch start
		copMove(pCopList, pBlock, &g_pCustom->bpl1mod, pManager->uwModulo);  // Odd planes modulo
		copMove(pCopList, pBlock, &g_pCustom->bpl2mod, pManager->uwModulo);  // Even planes modulo
		copMove(pCopList, pBlock, &g_pCustom->ddfstop, pManager->uwDDfStop); // Fetch stop
	}

	// Refresh bitplane pointers in copperlist - 2x for double buffered and with swapped cop front and back buffers
	scrollBufferProcess(pManager);
	tCopBfr *pBackBfr = pCopList->pBackBfr;
	pCopList->pBackBfr = pCopList->pFrontBfr;
	scrollBufferProcess(pManager);
	pCopList->pBackBfr = pBackBfr;

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
			wWidth, wHeight, (UBYTE*)pMsk
		);
	}
	else {
		// split blit in two
		WORD wPartHeight;
		wPartHeight = pDstManager->uwBmAvailHeight - wDstY;
		blitUnsafeCopyMask(
			pSrc, wSrcX, wSrcY,
			pDstManager->pBack, wDstX, wDstY,
			wWidth, wPartHeight, (UBYTE*)pMsk
		);
		blitUnsafeCopyMask(
			pSrc, wSrcX, wSrcY + wPartHeight,
			pDstManager->pBack, wDstX, 0,
			wWidth, wHeight - wPartHeight, (UBYTE*)pMsk
		);
	}
}
