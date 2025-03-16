/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/bob.h>
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/scrollbuffer.h> // for SCROLLBUFFER_HEIGHT_MODULO, TODO: get rid of it somehow
#include <ace/utils/custom.h>

#if !defined(ACE_NO_BOB_WRAP_Y)
// Enables support for Y-wrapping of bobs. Required for scroll- and tileBuffer.
// Disable for extra performance in simplebuffer scenarios.
// Making it a runtime flag wasn't giving enough performance boost,
// needs to be define-driven/constexpr.
#define BOB_WRAP_Y
#endif

// Undraw stack must be accessible during adding new bobs, so the most safe
// approach is to have two lists - undraw list gets populated after draw
// and depopulated during undraw
typedef struct tBobQueue {
	tBob **pBobs;
	tBitMap *pDst;
#if !defined(ACE_BOB_PRISTINE_BUFFER)
	tBitMap *pBg;
#endif
	UBYTE ubUndrawCount;
} tBobQueue;

static UBYTE s_ubBufferCurr;
static UBYTE s_ubMaxBobCount;

static UBYTE s_isPushingDone;
static UBYTE s_ubBpp;

// This can't be a decreasing counter such as in toSave/toDraw since after
// decrease another bob may be pushed, which would trash bg saving
static UBYTE s_ubBobsPushed;
static UBYTE s_ubBobsDrawn;
static UWORD s_uwAvailHeight;
static UWORD s_uwDestByteWidth;
#if defined(ACE_BOB_PRISTINE_BUFFER)
static tBitMap *s_pPristineBuffer;
#else
static UWORD s_uwBgBufferLength;
static UBYTE s_ubBobsSaved;
#endif

tBobQueue s_pQueues[2];

//------------------------------------------------------------------ PRIVATE FNS

static void bobCheckGood(const tBitMap *pBack) {
	if(s_pQueues[s_ubBufferCurr].pDst != pBack) {
#if defined(ACE_DEBUG)
		logWrite(
			"ERR: bob manager operates on wrong buffer! Proper current: %p (%hhu), Other: %p, Arg: %p\n",
			s_pQueues[s_ubBufferCurr].pDst, s_ubBufferCurr, s_pQueues[!s_ubBufferCurr].pDst, pBack
		);
		if(s_pQueues[!s_ubBufferCurr].pDst == pBack) {
			logWrite("ERR: Wrong bob buffer as curr\n");
			s_ubBufferCurr = !s_ubBufferCurr;
		}
#endif
	}
}

static void bobDeallocBuffers(void) {
	blitWait();
	systemUse();
	if(s_pQueues[0].pBobs && s_ubMaxBobCount) {
		memFree(s_pQueues[0].pBobs, sizeof(tBob*) * s_ubMaxBobCount);
		s_pQueues[0].pBobs = 0;
	}
	if(s_pQueues[1].pBobs && s_ubMaxBobCount) {
		memFree(s_pQueues[1].pBobs, sizeof(tBob*) * s_ubMaxBobCount);
		s_pQueues[1].pBobs = 0;
	}
	s_ubMaxBobCount = 0;
#if !defined(ACE_BOB_PRISTINE_BUFFER)
	if(s_pQueues[0].pBg) {
		bitmapDestroy(s_pQueues[0].pBg);
		s_pQueues[0].pBg = 0;
	}
	if(s_pQueues[1].pBg) {
		bitmapDestroy(s_pQueues[1].pBg);
		s_pQueues[1].pBg = 0;
	}
#endif
	systemUnuse();
}

static ULONG bobCalculateBitplaneOffset(const tBob *pBob, tBitMap *pDestination) {
	ULONG ulBitplaneOffset = (
		pDestination->BytesPerRow * (
#if defined(BOB_WRAP_Y)
			SCROLLBUFFER_HEIGHT_MODULO(pBob->sPos.uwY, s_uwAvailHeight)
#else
			pBob->sPos.uwY
#endif
		) + pBob->sPos.uwX / 8
	);
	return ulBitplaneOffset;
}

//------------------------------------------------------------------- PUBLIC FNS

void bobManagerReset(void) {
	bobDeallocBuffers();

	// Don't reset s_ubBufferCurr - we still may need to keep track which buffer
	// is in the front and which in the back
	// E.g. multiple states in single buffer manager:
	// fade-out, reset bobs, fade-in, start display

#if !defined(ACE_BOB_PRISTINE_BUFFER)
	s_uwBgBufferLength = 0;
	s_ubBobsSaved = 0;
#endif
	s_isPushingDone = 0;
	s_ubBobsPushed = 0;
	s_ubBobsDrawn = 0;
	bobDiscardUndraw();
}

void bobManagerCreate(
	tBitMap *pFront, tBitMap *pBack,
#if defined(ACE_BOB_PRISTINE_BUFFER)
	tBitMap *pPristineBuffer,
#endif
	UWORD uwAvailHeight
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

	s_ubBpp = pFront->Depth;
	s_pQueues[0].pDst = pBack;
	s_pQueues[1].pDst = pFront;

#if defined(ACE_BOB_PRISTINE_BUFFER)
	s_pPristineBuffer = pPristineBuffer;
#else
	s_pQueues[0].pBg = 0;
	s_pQueues[1].pBg = 0;
#endif
	s_pQueues[0].pBobs = 0;
	s_pQueues[1].pBobs = 0;
	s_ubMaxBobCount = 0;
	bobManagerReset();
	s_ubBufferCurr = 0;
	s_uwAvailHeight = uwAvailHeight;
	s_uwDestByteWidth = bitmapGetByteWidth(pBack);

	logBlockEnd("bobManagerCreate()");
}

void bobReallocateBuffers(void) {
	systemUse();
	logBlockBegin("bobReallocateBuffers()");

#if !defined(ACE_BOB_PRISTINE_BUFFER)
	if(s_pQueues[0].pBg) {
		bitmapDestroy(s_pQueues[0].pBg);
	}
	if(s_pQueues[1].pBg) {
		bitmapDestroy(s_pQueues[1].pBg);
	}
#endif

	logWrite("Max bobs: %hhu\n", s_ubMaxBobCount);
	s_pQueues[0].pBobs = memAllocFast(sizeof(tBob*) * s_ubMaxBobCount);
	s_pQueues[1].pBobs = memAllocFast(sizeof(tBob*) * s_ubMaxBobCount);
#if !defined(ACE_BOB_PRISTINE_BUFFER)
	s_pQueues[0].pBg = bitmapCreate(16, s_uwBgBufferLength, s_ubBpp, BMF_INTERLEAVED);
	s_pQueues[1].pBg = bitmapCreate(16, s_uwBgBufferLength, s_ubBpp, BMF_INTERLEAVED);
	logWrite("Undraw bg buffer length: %hu\n", s_uwBgBufferLength);
#endif
	logBlockEnd("bobReallocateBuffers()");
	systemUnuse();
}

void bobManagerDestroy(void) {
	bobDeallocBuffers();
}

void bobPush(tBob *pBob) {
	tBobQueue *pQueue = &s_pQueues[s_ubBufferCurr];
	pQueue->pBobs[s_ubBobsPushed] = pBob;
	++s_ubBobsPushed;
	if(blitIsIdle()) {
		bobProcessNext();
	}
}

void bobInit(
	tBob *pBob, UWORD uwWidth, UWORD uwHeight, UBYTE isUndrawRequired,
	UBYTE *pFrameData, UBYTE *pMaskData, UWORD uwX, UWORD uwY
) {
	logBlockBegin(
		"bobInit(pBob: %p, uwWidth: %hu, uwHeight: %hu, isUndrawRequired: %hhu, pFrameData: %p, pMaskData: %p, uwX: %hu, uwY: %hu)",
		pBob, uwWidth, uwHeight, isUndrawRequired, pFrameData, pMaskData, uwX, uwY
	);
#if defined(ACE_DEBUG)
	pBob->_uwOriginalWidth = uwWidth;
	pBob->_uwOriginalHeight = uwHeight;
#endif
	pBob->isUndrawRequired = isUndrawRequired;
	UWORD uwBlitWords = (uwWidth+15) / 16 + 1; // One word more for aligned copy
	pBob->_wModuloUndrawSave = s_uwDestByteWidth - uwBlitWords * 2;
	pBob->_uwBlitSize = uwBlitWords; // Height compontent is set later on
	bobSetFrame(pBob, pFrameData, pMaskData);
	bobSetWidth(pBob, uwWidth);
	bobSetHeight(pBob, uwHeight);

	pBob->sPos.uwX = uwX;
	pBob->sPos.uwY = uwY;
	pBob->pOldPositions[0].uwX = uwX;
	pBob->pOldPositions[0].uwY = uwY;
	pBob->pOldPositions[1].uwX = uwX;
	pBob->pOldPositions[1].uwY = uwY;

#if defined(ACE_BOB_PRISTINE_BUFFER)
	pBob->_pSaveOffsets[0] = 0;
	pBob->_pSaveOffsets[1] = 0;
#else
	pBob->_pBufferDrawPtrs[0] = 0;
	pBob->_pBufferDrawPtrs[1] = 0;
	if(isUndrawRequired) {
		s_uwBgBufferLength += uwBlitWords * pBob->_uwInterleavedHeight;
	}
#endif
	++s_ubMaxBobCount;
	logBlockEnd("bobInit()");
}

void bobSetFrame(tBob *pBob, UBYTE *pFrameData, UBYTE *pMaskData) {
	pBob->pFrameData = pFrameData;
	pBob->pMaskData = pMaskData;
}

void bobSetWidth(tBob *pBob, UWORD uwWidth)
{
#if defined(ACE_DEBUG)
	if(pBob->isUndrawRequired && uwWidth > pBob->_uwOriginalWidth) {
		// NOTE: that could be valid behavior when other bobs get smaller in the same time
		logWrite("WARN: Bob bigger than initial - bg buffer might be too small\n");
		// Change original width so that this warning gets issued only once
		pBob->_uwOriginalWidth = uwWidth;
	}
#endif

	pBob->uwWidth = uwWidth;
	UWORD uwBlitWords = (uwWidth + 15) / 16 + 1; // One word more for aligned copy
	pBob->_uwBlitSize = (pBob->_uwBlitSize & VSIZEMASK) | uwBlitWords;
}

void bobSetHeight(tBob *pBob, UWORD uwHeight)
{
#if defined(ACE_DEBUG)
	if(pBob->isUndrawRequired && uwHeight > pBob->_uwOriginalHeight) {
		// NOTE: that could be valid behavior when other bobs get smaller in the same time
		logWrite("WARN: Bob bigger than initial - bg buffer might be too small\n");
		// Change original height so that this warning gets issued only once
		pBob->_uwOriginalHeight = uwHeight;
	}
#endif

	pBob->uwHeight = uwHeight;
	pBob->_uwInterleavedHeight = uwHeight * s_ubBpp;
	pBob->_uwBlitSize = ((pBob->_uwInterleavedHeight) << HSIZEBITS) | (pBob->_uwBlitSize & HSIZEMASK);
}

UBYTE *bobCalcFrameAddress(tBitMap *pBitmap, UWORD uwOffsetY) {
	if(uwOffsetY >= pBitmap->Rows) {
		logWrite("ERR: bobCalcFrameAddress() OffsY %hu > bitmap height: %hu", uwOffsetY, pBitmap->Rows);
	}
	return &pBitmap->Planes[0][pBitmap->BytesPerRow * uwOffsetY];
}

UBYTE bobProcessNext(void) {
#if !defined(ACE_BOB_PRISTINE_BUFFER)
	if(s_ubBobsSaved < s_ubBobsPushed) {
		tBobQueue *pQueue = &s_pQueues[s_ubBufferCurr];
		if(!s_ubBobsSaved) {
			// Prepare for saving.
			// Bltcon0/1, bltaxwm could be reset between Begin and ProcessNext.
			// I tried to change A->D to C->D bug afwm/alwm need to be set
			// for mask-copying bobs, so there's no perf to be gained.
			UWORD uwBltCon0 = USEA|USED | MINTERM_A;
			blitWait();
			g_pCustom->bltcon0 = uwBltCon0;
			g_pCustom->bltcon1 = 0;
			g_pCustom->bltafwm = 0xFFFF;
			g_pCustom->bltalwm = 0xFFFF;

			g_pCustom->bltdmod = 0;
			g_pCustom->bltdpt = pQueue->pBg->Planes[0];
		}
		tBob *pBob = pQueue->pBobs[s_ubBobsSaved];
		++s_ubBobsSaved;

		// TODO: for BOB_WRAP_Y and ACE_DEBUG check if bob blit fits s_uwAvailHeight
		ULONG ulSrcOffs = bobCalculateBitplaneOffset(pBob, pQueue->pDst);
		UBYTE *pA = &pQueue->pDst->Planes[0][ulSrcOffs];
		pBob->_pBufferDrawPtrs[s_ubBufferCurr] = pA;

		if(pBob->isUndrawRequired) {
#if defined(BOB_WRAP_Y)
			UWORD uwPartHeight = s_uwAvailHeight - SCROLLBUFFER_HEIGHT_MODULO(pBob->sPos.uwY, s_uwAvailHeight);
#endif
			blitWait();
			g_pCustom->bltamod = pBob->_wModuloUndrawSave;
			g_pCustom->bltapt = (APTR)pA;
#if defined(BOB_WRAP_Y)
			if(uwPartHeight >= pBob->uwHeight) {
				g_pCustom->bltsize = pBob->_uwBlitSize;
			}
			else {
				UWORD uwInterleavedPartHeight = uwPartHeight * s_ubBpp;
				UWORD uwBlitWords = (pBob->uwWidth+15) / 16 + 1;
				g_pCustom->bltsize =(uwInterleavedPartHeight << HSIZEBITS) | uwBlitWords;
				pA = &pQueue->pDst->Planes[0][pBob->sPos.uwX / 8];
				blitWait();
				g_pCustom->bltapt = pA;
				g_pCustom->bltsize =((pBob->_uwInterleavedHeight - uwInterleavedPartHeight) << HSIZEBITS) | uwBlitWords;
			}
#else
			g_pCustom->bltsize = pBob->_uwBlitSize;
#endif
		}
		return 1;
	}

	if(!s_isPushingDone) {
		return 1;
	}
#endif

	tBobQueue *pQueue = &s_pQueues[s_ubBufferCurr];
	if(s_ubBobsDrawn < s_ubBobsPushed) {
		// Draw next
		tBob *pBob = pQueue->pBobs[s_ubBobsDrawn];
		const tUwCoordYX * pPos = &pBob->sPos;
		++s_ubBobsDrawn;
		UBYTE ubDstOffs = pPos->uwX & 0xF;
		UWORD uwBlitWidth = (pBob->uwWidth + ubDstOffs + 15) & 0xFFF0;
		UWORD uwBlitWords = uwBlitWidth / 16;
		UWORD uwBlitSize = ((pBob->_uwInterleavedHeight) << HSIZEBITS) | uwBlitWords;
		WORD wSrcModulo = pBob->uwWidth / 8 - uwBlitWords * 2;
		UWORD uwBltCon1 = ubDstOffs << BSHIFTSHIFT;
		UWORD uwBltCon0;
		if(pBob->pMaskData) {
			uwBltCon0 = uwBltCon1 | USEA|USEB|USEC|USED | MINTERM_COOKIE;
		}
		else {
			uwBltCon0 = uwBltCon1 | USEB|USEC|USED | MINTERM_COOKIE;
		}

		WORD wDstModulo = s_uwDestByteWidth - uwBlitWords * 2;
		UBYTE *pB = pBob->pFrameData;
#if defined(ACE_BOB_PRISTINE_BUFFER)
		ULONG ulDestinationOffset = bobCalculateBitplaneOffset(pBob, pQueue->pDst);
		UBYTE *pCD = &pQueue->pDst->Planes[0][ulDestinationOffset];
		pBob->_pSaveOffsets[s_ubBufferCurr] = ulDestinationOffset;
#else
		UBYTE *pCD = pBob->_pBufferDrawPtrs[s_ubBufferCurr];
#endif
#if defined(BOB_WRAP_Y)
		UWORD uwPartHeight = s_uwAvailHeight - SCROLLBUFFER_HEIGHT_MODULO(pBob->sPos.uwY, s_uwAvailHeight);
#endif

		UWORD uwLastMask = 0xFFFF << (uwBlitWidth-pBob->uwWidth);
		blitWait();
		g_pCustom->bltcon0 = uwBltCon0;
		g_pCustom->bltcon1 = uwBltCon1;

		g_pCustom->bltalwm = uwLastMask;
		if(pBob->pMaskData) {
			UBYTE *pA = pBob->pMaskData;
			g_pCustom->bltamod = wSrcModulo;
			g_pCustom->bltapt = (APTR)pA;
		}
		else {
			g_pCustom->bltadat = 0xFFFF;
		}

		g_pCustom->bltbmod = wSrcModulo;
		g_pCustom->bltcmod = wDstModulo;
		g_pCustom->bltdmod = wDstModulo;

		g_pCustom->bltbpt = (APTR)pB;
		g_pCustom->bltcpt = (APTR)pCD;
		g_pCustom->bltdpt = (APTR)pCD;
#if defined(BOB_WRAP_Y)
		if(uwPartHeight >= pBob->uwHeight) {
			g_pCustom->bltsize = uwBlitSize;
		}
		else {
			UWORD uwInterleavedPartHeight = uwPartHeight * s_ubBpp;
			g_pCustom->bltsize = (uwInterleavedPartHeight << HSIZEBITS) | uwBlitWords;
			pCD = &pQueue->pDst->Planes[0][pBob->sPos.uwX / 8];
			blitWait();
			g_pCustom->bltcpt = (APTR)pCD;
			g_pCustom->bltdpt = (APTR)pCD;
			g_pCustom->bltsize =((pBob->_uwInterleavedHeight - uwInterleavedPartHeight) << HSIZEBITS) | uwBlitWords;
		}
#else
		g_pCustom->bltsize = uwBlitSize;
#endif
		pBob->pOldPositions[s_ubBufferCurr].ulYX = pPos->ulYX;
		return 1;
	}

	return 0;
}

void bobBegin(tBitMap *pBuffer) {
	bobCheckGood(pBuffer);
	tBobQueue *pQueue = &s_pQueues[s_ubBufferCurr];

#if defined(ACE_BOB_PRISTINE_BUFFER)
	UWORD uwBltCon0 = USEA|USED | MINTERM_A;
	blitWait();
	g_pCustom->bltcon0 = uwBltCon0;
	g_pCustom->bltcon1 = 0;
	g_pCustom->bltafwm = 0xFFFF;
	g_pCustom->bltalwm = 0xFFFF;

	for(UBYTE i = 0; i < pQueue->ubUndrawCount; ++i) {
		const tBob *pBob = pQueue->pBobs[i];
		if(!pBob->isUndrawRequired) {
			continue;
		}

#if defined(BOB_WRAP_Y)
		UWORD uwPartHeight = s_uwAvailHeight - SCROLLBUFFER_HEIGHT_MODULO(
			pBob->pOldPositions[s_ubBufferCurr].uwY, s_uwAvailHeight
		);
#endif
		ULONG ulBitplaneOffset = pBob->_pSaveOffsets[s_ubBufferCurr];
		blitWait();
		g_pCustom->bltamod = pBob->_wModuloUndrawSave;
		g_pCustom->bltdmod = pBob->_wModuloUndrawSave;
		g_pCustom->bltapt = &s_pPristineBuffer->Planes[0][ulBitplaneOffset];
		g_pCustom->bltdpt = &pQueue->pDst->Planes[0][ulBitplaneOffset];
#if defined(BOB_WRAP_Y)
		if(uwPartHeight >= pBob->uwHeight) {
			g_pCustom->bltsize = pBob->_uwBlitSize;
		}
		else {
			UWORD uwBlitWords = (pBob->uwWidth+15) / 16 + 1;
			UWORD uwInterleavedPartHeight = uwPartHeight * s_ubBpp;
			g_pCustom->bltsize = (uwInterleavedPartHeight << HSIZEBITS) | uwBlitWords;
			ulBitplaneOffset = pBob->pOldPositions[s_ubBufferCurr].uwX / 8;
			blitWait();
			g_pCustom->bltapt = &s_pPristineBuffer->Planes[0][ulBitplaneOffset];
			g_pCustom->bltdpt = &pQueue->pDst->Planes[0][ulBitplaneOffset];
			g_pCustom->bltsize =((pBob->_uwInterleavedHeight - uwInterleavedPartHeight) << HSIZEBITS) | uwBlitWords;
		}
#else
		g_pCustom->bltsize = pBob->_uwBlitSize;
#endif
	}
#else
	// Prepare for undraw
	UBYTE *pA = pQueue->pBg->Planes[0];
	blitWait();
	g_pCustom->bltcon0 = USEA|USED | MINTERM_A;
	g_pCustom->bltcon1 = 0;
	g_pCustom->bltafwm = 0xFFFF;
	g_pCustom->bltalwm = 0xFFFF;
	g_pCustom->bltamod = 0;
	g_pCustom->bltapt = pA;
#ifdef GAME_DEBUG
	UWORD uwDrawnHeight = 0;
#endif

	for(UBYTE i = 0; i < pQueue->ubUndrawCount; ++i) {
		const tBob *pBob = pQueue->pBobs[i];
		if(pBob->isUndrawRequired) {
			// Undraw next
			UBYTE *pD = pBob->_pBufferDrawPtrs[s_ubBufferCurr];
#if defined(BOB_WRAP_Y)
			UWORD uwPartHeight = s_uwAvailHeight - SCROLLBUFFER_HEIGHT_MODULO(
				pBob->pOldPositions[s_ubBufferCurr].uwY, s_uwAvailHeight
			);
#endif
			blitWait();
			g_pCustom->bltdmod = pBob->_wModuloUndrawSave;
			g_pCustom->bltdpt = (APTR)pD;
#if defined(BOB_WRAP_Y)
			if(uwPartHeight >= pBob->uwHeight) {
				g_pCustom->bltsize = pBob->_uwBlitSize;
			}
			else {
				UWORD uwBlitWords = (pBob->uwWidth+15) / 16 + 1;
				UWORD uwInterleavedPartHeight = uwPartHeight * s_ubBpp;
				g_pCustom->bltsize =(uwInterleavedPartHeight << HSIZEBITS) | uwBlitWords;
				pD = &pQueue->pDst->Planes[0][pBob->pOldPositions[s_ubBufferCurr].uwX / 8];
				blitWait();
				g_pCustom->bltdpt = pD;
				g_pCustom->bltsize =((pBob->_uwInterleavedHeight - uwInterleavedPartHeight) << HSIZEBITS) | uwBlitWords;
			}
#else
			g_pCustom->bltsize = pBob->_uwBlitSize;
#endif

#ifdef GAME_DEBUG
			UWORD uwBlitWords = (pBob->uwWidth+15) / 16 + 1;
			uwDrawnHeight += uwBlitWords * pBob->uwHeight;
#endif
		}
	}

	s_ubBobsSaved = 0;
#endif

#ifdef GAME_DEBUG
	UWORD uwDrawLimit = s_pQueues[0].pBg->Rows * s_pQueues[0].pBg->Depth;
	if(uwDrawnHeight > uwDrawLimit) {
		logWrite(
			"ERR: BG restore out of bounds: used %hu, limit: %hu",
			uwDrawnHeight, uwDrawLimit
		);
	}
#endif

	s_ubBobsDrawn = 0;
	s_ubBobsPushed = 0;
	s_isPushingDone = 0;
}

void bobPushingDone(void) {
	s_isPushingDone = 1;
}

void bobProcessAll(void) {
	while(bobProcessNext()) continue;
}

UBYTE bobGetCurrentBufferIndex(void) {
	return s_ubBufferCurr;
}

void bobEnd(void) {
	bobPushingDone();
	bobProcessAll();
	s_pQueues[s_ubBufferCurr].ubUndrawCount = s_ubBobsPushed;
	s_ubBufferCurr = !s_ubBufferCurr;
}

void bobDiscardUndraw(void) {
	s_pQueues[0].ubUndrawCount = 0;
	s_pQueues[1].ubUndrawCount = 0;
}

void bobSetCurrentBuffer(tBitMap *pCurrent) {
	if(s_pQueues[!s_ubBufferCurr].pDst == pCurrent) {
		s_ubBufferCurr = !s_ubBufferCurr;
	}
}
