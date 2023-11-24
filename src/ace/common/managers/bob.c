/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/bob.h>
#include <ace/managers/system.h>
#include <ace/managers/memory.h>
#if defined(AMIGA)
#include <ace/utils/custom.h>
#endif

tBobManager g_sBobManager;

static void bobDeallocBuffers(void) {
	blitWait();
	systemUse();
	if(g_sBobManager.pQueues[0].pBobs && g_sBobManager.ubMaxBobCount) {
		memFree(g_sBobManager.pQueues[0].pBobs, sizeof(tBob*) * g_sBobManager.ubMaxBobCount);
		g_sBobManager.pQueues[0].pBobs = 0;
	}
	if(g_sBobManager.pQueues[1].pBobs && g_sBobManager.ubMaxBobCount) {
		memFree(g_sBobManager.pQueues[1].pBobs, sizeof(tBob*) * g_sBobManager.ubMaxBobCount);
		g_sBobManager.pQueues[1].pBobs = 0;
	}
	g_sBobManager.ubMaxBobCount = 0;
	if(g_sBobManager.pQueues[0].pBg) {
		bitmapDestroy(g_sBobManager.pQueues[0].pBg);
		g_sBobManager.pQueues[0].pBg = 0;
	}
	if(g_sBobManager.pQueues[1].pBg) {
		bitmapDestroy(g_sBobManager.pQueues[1].pBg);
		g_sBobManager.pQueues[1].pBg = 0;
	}
	systemUnuse();
}

void bobManagerReset(void) {
	bobDeallocBuffers();

	// Don't reset g_sBobManager.ubBufferCurr - we still may need to keep track which buffer
	// is in the front and which in the back
	// E.g. multiple states in single buffer manager:
	// fade-out, reset bobs, fade-in, start display

	g_sBobManager.uwBgBufferLength = 0;
	g_sBobManager.isPushingDone = 0;
	g_sBobManager.ubBobsPushed = 0;
	g_sBobManager.ubBobsSaved = 0;
	g_sBobManager.ubBobsDrawn = 0;
	bobDiscardUndraw();
}

void bobManagerCreate(
	tBitMap *pFront, tBitMap *pBack, UWORD uwAvailHeight
) {
	logBlockBegin(
		"bobManagerCreate(pFront: %p, pBack: %p, uwAvailHeight: %hu)",
		pFront, pBack, uwAvailHeight
	);

	if(!bitmapIsInterleaved(pFront)) {
		logWrite("ERR: front buffer bitmap %p isn't interleaved\n", pFront);
	}

	if(!bitmapIsInterleaved(pBack)) {
		logWrite("ERR: back buffer bitmap %p isn't interleaved\n", pBack);
	}

	g_sBobManager.ubBpp = pFront->Depth;
	g_sBobManager.pQueues[0].pDst = pBack;
	g_sBobManager.pQueues[1].pDst = pFront;

	g_sBobManager.pQueues[0].pBg = 0;
	g_sBobManager.pQueues[1].pBg = 0;
	g_sBobManager.ubMaxBobCount = 0;
	bobManagerReset();
	g_sBobManager.ubBufferCurr = 0;
	g_sBobManager.uwAvailHeight = uwAvailHeight;
	g_sBobManager.uwDestByteWidth = bitmapGetByteWidth(pBack);

	g_sBobManager.pQueues[0].pBg = 0;
	g_sBobManager.pQueues[1].pBg = 0;
	g_sBobManager.pQueues[0].pBobs = 0;
	g_sBobManager.pQueues[1].pBobs = 0;
	logBlockEnd("bobManagerCreate()");
}

void bobReallocateBgBuffers(void) {
	systemUse();
	logBlockBegin("bobReallocateBgBuffers()");

	if(g_sBobManager.pQueues[0].pBg) {
		bitmapDestroy(g_sBobManager.pQueues[0].pBg);
	}

	if(g_sBobManager.pQueues[1].pBg) {
		bitmapDestroy(g_sBobManager.pQueues[1].pBg);
	}

	g_sBobManager.pQueues[0].pBobs = memAllocFast(sizeof(tBob*) * g_sBobManager.ubMaxBobCount);
	g_sBobManager.pQueues[1].pBobs = memAllocFast(sizeof(tBob*) * g_sBobManager.ubMaxBobCount);
	g_sBobManager.pQueues[0].pBg = bitmapCreate(16, g_sBobManager.uwBgBufferLength, g_sBobManager.ubBpp, BMF_INTERLEAVED);
	g_sBobManager.pQueues[1].pBg = bitmapCreate(16, g_sBobManager.uwBgBufferLength, g_sBobManager.ubBpp, BMF_INTERLEAVED);
	logWrite(
		"New bg buffer length: %hu, max bobs: %hhu\n",
		g_sBobManager.uwBgBufferLength, g_sBobManager.ubMaxBobCount
	);
	logBlockEnd("bobReallocateBgBuffers()");
	systemUnuse();
}

void bobManagerDestroy(void) {
	bobDeallocBuffers();
}

void bobPush(tBob *pBob) {
	tBobQueue *pQueue = &g_sBobManager.pQueues[g_sBobManager.ubBufferCurr];
	pQueue->pBobs[g_sBobManager.ubBobsPushed] = pBob;
	++g_sBobManager.ubBobsPushed;
	if(blitIsIdle()) {
		bobProcessNext();
	}
}

void bobInit(
	tBob *pBob, UWORD uwWidth, UWORD uwHeight, UBYTE isUndrawRequired,
	UBYTE *pFrameData, UBYTE *pMaskData, UWORD uwX, UWORD uwY
) {
#if defined(ACE_DEBUG)
	pBob->_uwOriginalWidth = uwWidth;
	pBob->_uwOriginalHeight = uwHeight;
#endif
	pBob->isUndrawRequired = isUndrawRequired;
	UWORD uwBlitWords = (uwWidth+15) / 16 + 1; // One word more for aligned copy
	pBob->_wModuloUndrawSave = g_sBobManager.uwDestByteWidth - uwBlitWords * 2;
	pBob->_uwBlitSize = uwBlitWords; // Height compontent is set later on
	bobSetFrame(pBob, pFrameData, pMaskData);
	bobSetWidth(pBob, uwWidth);
	bobSetHeight(pBob, uwHeight);

	pBob->sPos.uwX = uwX;
	pBob->sPos.uwY = uwY;
	pBob->pOldPositions[0].uwX = uwX;
	pBob->pOldPositions[0].uwY = uwY;
	pBob->_pOldDrawOffs[0] = 0;
	pBob->pOldPositions[1].uwX = uwX;
	pBob->pOldPositions[1].uwY = uwY;
	pBob->_pOldDrawOffs[1] = 0;

	if(isUndrawRequired) {
		g_sBobManager.uwBgBufferLength += uwBlitWords * uwHeight * g_sBobManager.ubBpp;
	}
	++g_sBobManager.ubMaxBobCount;
	// logWrite("Added bob, now max: %hhu\n", g_sBobManager.ubMaxBobCount);
}

void bobSetWidth(tBob *pBob, UWORD uwWidth)
{
#if defined(ACE_DEBUG)
	if(pBob->isUndrawRequired && uwWidth > pBob->_uwOriginalWidth) {
		// NOTE: that could be valid behavior when other bobs get smaller in the same time
		logWrite("WARN: Bob bigger than initial - bg buffer might be too small!\n");
		// Change original width so that this warning gets issued only once
		pBob->_uwOriginalWidth = uwWidth;
	}
#endif

	pBob->uwWidth = uwWidth;
	UWORD uwBlitWords = (uwWidth + 15) / 16 + 1; // One word more for aligned copy
#if defined(AMIGA)
	pBob->_uwBlitSize = (pBob->_uwBlitSize & VSIZEMASK) | uwBlitWords;
#endif
}

void bobSetHeight(tBob *pBob, UWORD uwHeight)
{
#if defined(ACE_DEBUG)
	if(pBob->isUndrawRequired && uwHeight > pBob->_uwOriginalHeight) {
		// NOTE: that could be valid behavior when other bobs get smaller in the same time
		logWrite("WARN: Bob bigger than initial - bg buffer might be too small!\n");
		// Change original height so that this warning gets issued only once
		pBob->_uwOriginalHeight = uwHeight;
	}
#endif

	pBob->uwHeight = uwHeight;
#if defined(AMIGA)
	pBob->_uwBlitSize = ((uwHeight*g_sBobManager.ubBpp) << HSIZEBITS) | (pBob->_uwBlitSize & HSIZEMASK);
#endif
}

void bobCheckGood(const tBitMap *pBack) {
	if(g_sBobManager.pQueues[g_sBobManager.ubBufferCurr].pDst != pBack) {
#if defined(ACE_DEBUG)
		logWrite(
			"ERR: bob manager operates on wrong buffer! Proper current: %p (%hhu), Other: %p, Arg: %p\n",
			g_sBobManager.pQueues[g_sBobManager.ubBufferCurr].pDst, g_sBobManager.ubBufferCurr, g_sBobManager.pQueues[!g_sBobManager.ubBufferCurr].pDst, pBack
		);
		if(g_sBobManager.pQueues[!g_sBobManager.ubBufferCurr].pDst == pBack) {
			logWrite("ERR: Wrong bob buffer as curr!\n");
			g_sBobManager.ubBufferCurr = !g_sBobManager.ubBufferCurr;
		}
#endif
	}
}

void bobPushingDone(void) {
	g_sBobManager.isPushingDone = 1;
}

void bobEnd(void) {
	bobPushingDone();
	do {
	} while(bobProcessNext());
	g_sBobManager.pQueues[g_sBobManager.ubBufferCurr].ubUndrawCount = g_sBobManager.ubBobsPushed;
	g_sBobManager.ubBufferCurr = !g_sBobManager.ubBufferCurr;
}

void bobDiscardUndraw(void) {
	g_sBobManager.pQueues[0].ubUndrawCount = 0;
	g_sBobManager.pQueues[1].ubUndrawCount = 0;
}

void bobSetFrame(tBob *pBob, UBYTE *pFrameData, UBYTE *pMaskData) {
	pBob->pFrameData = pFrameData;
	pBob->pMaskData = pMaskData;
}

UBYTE *bobCalcFrameAddress(tBitMap *pBitmap, UWORD uwOffsetY) {
	return &pBitmap->Planes[0][pBitmap->BytesPerRow * uwOffsetY];
}
