/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/bob.h>

static UBYTE *s_pBgSaveDest;

void bobBegin(tBitMap *pBuffer) {
	bobCheckGood(pBuffer);
	tBobQueue *pQueue = &g_sBobManager.pQueues[g_sBobManager.ubBufferCurr];

	// Prepare for undraw
	UBYTE *pBgRestoreSrc = pQueue->pBg->Planes[0];
#ifdef ACE_DEBUG
	UWORD uwDrawnHeight = 0;
#endif

	for(UBYTE i = 0; i < pQueue->ubUndrawCount; ++i) {
		const tBob *pBob = pQueue->pBobs[i];
		if(pBob->isUndrawRequired) {
			UBYTE *pBgRestoreDst = pBob->_pOldDrawOffs[g_sBobManager.ubBufferCurr];
			UWORD uwHeight = pBob->_pOldBgBlitHeight[g_sBobManager.ubBufferCurr];
			UWORD uwBlitByteWidth = ((pBob->uwWidth + 15) / 16 + 1) * 2; // One word more for aligned copy
			UWORD uwBlitHeight = g_sBobManager.ubBpp * uwHeight;
			// Undraw next
			// TODO: BOB_WRAP_Y
			tBitMap sSrc = {
				.BytesPerRow = uwBlitByteWidth,
				.Depth = 1,
				.Flags = 0,
				.Planes = {pBgRestoreSrc},
				.Rows = uwBlitHeight,
			};

			tBitMap sDst = {
				.BytesPerRow = bitmapGetByteWidth(pQueue->pDst),
				.Depth = 1,
				.Flags = 0,
				.Planes = {pBgRestoreDst},
				.Rows = uwBlitHeight,
			};
			blitCopyAligned(&sSrc, 0, 0, &sDst, 0, 0, uwBlitByteWidth * 8, uwBlitHeight);
			pBgRestoreSrc += uwBlitByteWidth * uwBlitHeight;

#ifdef ACE_DEBUG
			UWORD uwBlitWords = (pBob->uwWidth + 15) / 16 + 1;
			uwDrawnHeight += uwBlitWords * uwBlitHeight;
#endif
		}
	}
#ifdef ACE_DEBUG
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
	// TODO: BOB_WRAP_Y
	if(g_sBobManager.ubBobsSaved < g_sBobManager.ubBobsPushed) {
		tBobQueue *pQueue = &g_sBobManager.pQueues[g_sBobManager.ubBufferCurr];
		if(!g_sBobManager.ubBobsSaved) {
			s_pBgSaveDest = pQueue->pBg->Planes[0];
		}
		tBob *pBob = pQueue->pBobs[g_sBobManager.ubBobsSaved];
		++g_sBobManager.ubBobsSaved;

		// TODO: for BOB_WRAP_Y and ACE_DEBUG check if bob blit fits g_sBobManager.uwAvailHeight
		ULONG ulSrcOffs = (
			pQueue->pDst->BytesPerRow * (pBob->sPos.uwY) + pBob->sPos.uwX / 8
		);
		UBYTE *pBgSaveSrc = &pQueue->pDst->Planes[0][ulSrcOffs];
		pBob->_pOldDrawOffs[g_sBobManager.ubBufferCurr] = pBgSaveSrc;
		pBob->_pOldBgBlitHeight[g_sBobManager.ubBufferCurr] = pBob->uwHeight;

		if(pBob->isUndrawRequired) {
			UWORD uwBlitByteWidth = ((pBob->uwWidth + 15) / 16 + 1) * 2; // One word more for aligned copy
			UWORD uwBlitHeight = g_sBobManager.ubBpp * pBob->uwHeight;
			tBitMap sSrc = {
				.BytesPerRow = bitmapGetByteWidth(pQueue->pDst),
				.Depth = 1,
				.Flags = 0,
				.Planes = {pBgSaveSrc},
				.Rows = uwBlitHeight,
			};
			tBitMap sDst = {
				.BytesPerRow = uwBlitByteWidth,
				.Depth = 1,
				.Flags = 0,
				.Planes = {s_pBgSaveDest},
				.Rows = uwBlitHeight,
			};
			blitCopyAligned(&sSrc, 0, 0, &sDst, 0, 0, uwBlitByteWidth * 8, uwBlitHeight);
			s_pBgSaveDest += uwBlitByteWidth * uwBlitHeight;
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
			++g_sBobManager.ubBobsDrawn;

			UWORD uwBlitByteWidth = ((pBob->uwWidth + 15) / 16) * 2;
			UWORD uwBlitHeight = pBob->uwHeight;
			tBitMap sSrc = {
				.BytesPerRow = uwBlitByteWidth * g_sBobManager.ubBpp,
				.Depth = g_sBobManager.ubBpp,
				.Flags = BMF_INTERLEAVED,
				.Rows = uwBlitHeight
			};
			for(UBYTE i = 0; i < g_sBobManager.ubBpp; ++i) {
				sSrc.Planes[i] = pBob->pFrameData + i * uwBlitByteWidth;
			}
			if(pBob->pMaskData) {
				blitCopyMask(
					&sSrc, 0, 0, pQueue->pDst, pBob->sPos.uwX, pBob->sPos.uwY,
					pBob->uwWidth, pBob->uwHeight, pBob->pMaskData
				);
			}
			else {
				blitCopy(
					&sSrc, 0, 0, pQueue->pDst, pBob->sPos.uwX, pBob->sPos.uwY,
					pBob->uwWidth, pBob->uwHeight, MINTERM_COOKIE
				);
			}

			pBob->pOldPositions[g_sBobManager.ubBufferCurr].ulYX = pBob->sPos.ulYX;
			return 1;
		}
	}
	return 0;
}
