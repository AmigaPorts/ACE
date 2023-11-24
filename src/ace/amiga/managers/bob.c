/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/bob.h>
#include <ace/managers/blit.h>
#include <ace/managers/log.h>

#if !defined(ACE_NO_BOB_WRAP_Y)
// Enables support for Y-wrapping of bobs. Required for scroll- and tileBuffer.
// Disable for extra performance in simplebuffer scenarios.
// Making it a runtime flag wasn't giving enough performance boost,
// needs to be define-driven/constexpr.
#define BOB_WRAP_Y
#endif

void bobBegin(tBitMap *pBuffer) {
	bobCheckGood(pBuffer);
	tBobQueue *pQueue = &g_sBobManager.pQueues[g_sBobManager.ubBufferCurr];

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
			UBYTE *pD = pBob->_pOldDrawOffs[g_sBobManager.ubBufferCurr];
#if defined(BOB_WRAP_Y)
			UWORD uwHeight = pBob->_pOldBgBlitHeight[g_sBobManager.ubBufferCurr];
			UWORD uwPartHeight = g_sBobManager.uwAvailHeight - (pBob->pOldPositions[g_sBobManager.ubBufferCurr].uwY & (g_sBobManager.uwAvailHeight-1));
#endif
			UWORD uwBlitSize = pBob->_pOldBgBlitSize[g_sBobManager.ubBufferCurr];
			blitWait();
			g_pCustom->bltdmod = pBob->_wModuloUndrawSave;
			g_pCustom->bltdpt = (APTR)pD;
#if defined(BOB_WRAP_Y)
			if(uwPartHeight >= uwHeight) {
				g_pCustom->bltsize = uwBlitSize;
			}
			else {
				UWORD uwBlitWords = (pBob->uwWidth+15) / 16 + 1;
				g_pCustom->bltsize =((uwPartHeight * g_sBobManager.ubBpp) << HSIZEBITS) | uwBlitWords;
				pD = &pQueue->pDst->Planes[0][pBob->pOldPositions[g_sBobManager.ubBufferCurr].uwX / 8];
				blitWait();
				g_pCustom->bltdpt = pD;
				g_pCustom->bltsize =(((uwHeight - uwPartHeight) * g_sBobManager.ubBpp) << HSIZEBITS) | uwBlitWords;
			}
#else
			g_pCustom->bltsize = uwBlitSize;
#endif

#ifdef GAME_DEBUG
			UWORD uwBlitWords = (pBob->uwWidth+15) / 16 + 1;
			uwDrawnHeight += uwBlitWords * uwHeight;
#endif
		}
	}
#ifdef GAME_DEBUG
	UWORD uwDrawLimit = g_sBobManager.pQueues[0].pBg->Rows * g_sBobManager.pQueues[0].pBg->Depth;
	if(uwDrawnHeight > uwDrawLimit) {
		logWrite(
			"ERR: BG restore out of bounds: used %hu, limit: %hu",
			uwDrawnHeight, uwDrawLimit
		);
	}
#endif
	g_sBobManager.ubBobsSaved = 0;
	g_sBobManager.ubBobsDrawn = 0;
	g_sBobManager.ubBobsPushed = 0;
	g_sBobManager.isPushingDone = 0;
}

UBYTE bobProcessNext(void) {
	if(g_sBobManager.ubBobsSaved < g_sBobManager.ubBobsPushed) {
		tBobQueue *pQueue = &g_sBobManager.pQueues[g_sBobManager.ubBufferCurr];
		if(!g_sBobManager.ubBobsSaved) {
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
		tBob *pBob = pQueue->pBobs[g_sBobManager.ubBobsSaved];
		++g_sBobManager.ubBobsSaved;

		// TODO: for BOB_WRAP_Y and ACE_DEBUG check if bob blit fits g_sBobManager.uwAvailHeight
		ULONG ulSrcOffs = (
			pQueue->pDst->BytesPerRow * (
#if defined(BOB_WRAP_Y)
				pBob->sPos.uwY & (g_sBobManager.uwAvailHeight - 1)
#else
				pBob->sPos.uwY
#endif
			) + pBob->sPos.uwX / 8
		);
		UBYTE *pA = &pQueue->pDst->Planes[0][ulSrcOffs];
		pBob->_pOldDrawOffs[g_sBobManager.ubBufferCurr] = pA;
#if defined(BOB_WRAP_Y)
		pBob->_pOldBgBlitHeight[g_sBobManager.ubBufferCurr] = pBob->uwHeight;
#endif
		pBob->_pOldBgBlitSize[g_sBobManager.ubBufferCurr] = pBob->_uwBlitSize;

		if(pBob->isUndrawRequired) {
#if defined(BOB_WRAP_Y)
			UWORD uwPartHeight = g_sBobManager.uwAvailHeight - (pBob->sPos.uwY & (g_sBobManager.uwAvailHeight-1));
#endif
			blitWait();
			g_pCustom->bltamod = pBob->_wModuloUndrawSave;
			g_pCustom->bltapt = (APTR)pA;
#if defined(BOB_WRAP_Y)
			if(uwPartHeight >= pBob->uwHeight) {
				g_pCustom->bltsize = pBob->_uwBlitSize;
			}
			else {
				UWORD uwBlitWords = (pBob->uwWidth+15) / 16 + 1;
				g_pCustom->bltsize =((uwPartHeight * g_sBobManager.ubBpp) << HSIZEBITS) | uwBlitWords;
				pA = &pQueue->pDst->Planes[0][pBob->sPos.uwX / 8];
				blitWait();
				g_pCustom->bltapt = pA;
				g_pCustom->bltsize =(((pBob->uwHeight - uwPartHeight) * g_sBobManager.ubBpp) << HSIZEBITS) | uwBlitWords;
			}
#else
			g_pCustom->bltsize = pBob->_uwBlitSize;
#endif
		}
		return 1;
	}
	else {
		if(!g_sBobManager.isPushingDone) {
			return 1;
		}

		tBobQueue *pQueue = &g_sBobManager.pQueues[g_sBobManager.ubBufferCurr];
		if(g_sBobManager.ubBobsDrawn < g_sBobManager.ubBobsPushed) {
			// Draw next
			tBob *pBob = pQueue->pBobs[g_sBobManager.ubBobsDrawn];
			const tUwCoordYX * pPos = &pBob->sPos;
			++g_sBobManager.ubBobsDrawn;
			UBYTE ubDstOffs = pPos->uwX & 0xF;
			UWORD uwBlitWidth = (pBob->uwWidth + ubDstOffs + 15) & 0xFFF0;
			UWORD uwBlitWords = uwBlitWidth >> 4;
			UWORD uwBlitSize = ((pBob->uwHeight * g_sBobManager.ubBpp) << HSIZEBITS) | uwBlitWords;
			WORD wSrcModulo = (pBob->uwWidth >> 3) - (uwBlitWords<<1);
			UWORD uwBltCon1 = ubDstOffs << BSHIFTSHIFT;
			UWORD uwBltCon0;
			if(pBob->pMaskData) {
				uwBltCon0 = uwBltCon1 | USEA|USEB|USEC|USED | MINTERM_COOKIE;
			}
			else {
				// TODO change to A - performance boost
				// TODO setting B & C regs isn't necessary - few write cycles less
				uwBltCon0 = uwBltCon1 | USEB|USED | MINTERM_B;
			}

			WORD wDstModulo = g_sBobManager.uwDestByteWidth - (uwBlitWords<<1);
			UBYTE *pB = pBob->pFrameData;
			UBYTE *pCD = pBob->_pOldDrawOffs[g_sBobManager.ubBufferCurr];
#if defined(BOB_WRAP_Y)
			UWORD uwPartHeight = g_sBobManager.uwAvailHeight - (pBob->sPos.uwY & (g_sBobManager.uwAvailHeight-1));
#endif

			blitWait();
			g_pCustom->bltcon0 = uwBltCon0;
			g_pCustom->bltcon1 = uwBltCon1;

			if(pBob->pMaskData) {
				UWORD uwLastMask = 0xFFFF << (uwBlitWidth-pBob->uwWidth);
				UBYTE *pA = pBob->pMaskData;
				g_pCustom->bltalwm = uwLastMask;
				g_pCustom->bltamod = wSrcModulo;
				g_pCustom->bltapt = (APTR)pA;
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
				g_pCustom->bltsize =((uwPartHeight * g_sBobManager.ubBpp) << HSIZEBITS) | uwBlitWords;
				pCD = &pQueue->pDst->Planes[0][pBob->sPos.uwX / 8];
				blitWait();
				g_pCustom->bltcpt = (APTR)pCD;
				g_pCustom->bltdpt = (APTR)pCD;
				g_pCustom->bltsize =(((pBob->uwHeight - uwPartHeight) * g_sBobManager.ubBpp) << HSIZEBITS) | uwBlitWords;
			}
#else
			g_pCustom->bltsize = uwBlitSize;
#endif
			pBob->pOldPositions[g_sBobManager.ubBufferCurr].ulYX = pPos->ulYX;
			return 1;
		}
	}
	return 0;
}
