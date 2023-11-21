/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/blit.h>

void blitWait(void) {
	while(!blitIsIdle()) continue;
}

UBYTE blitSafeCopy(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubMinterm, UWORD uwLine, const char *szFile
) {
	if(!blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile)) {
		return 0;
	}
	return blitUnsafeCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm);
}

UBYTE blitSafeCopyAligned(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UWORD uwLine, const char *szFile
) {
	if((wSrcX | wDstX | wWidth) & 0x000F) {
		logWrite("ERR: Dimensions are not divisible by 16\n");
		return 0;
	}
	if(!blitCheck(
		pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile
	)) {
		return 0;
	}
	return blitUnsafeCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight);
}

UBYTE blitSafeCopyMask(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, const UBYTE *pMsk, UWORD uwLine, const char *szFile
) {
	if(wSrcX > wDstX) {
		logWrite(
			"ERR: wSrcX %hd must be smaller than or equal to wDstX %hd\n",
			wSrcX, wDstX
		);
		return 0;
	}
	if(!blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile)) {
		return 0;
	}
	return blitUnsafeCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk);
}

UBYTE blitSafeRect(
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubColor, UWORD uwLine, const char *szFile
) {
	if(!blitCheck(0,0,0,pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile)) {
		return 0;
	}

	return blitUnsafeRect(pDst, wDstX, wDstY, wWidth, wHeight, ubColor);
}

#if defined(ACE_DEBUG)
UBYTE _blitCheck(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	const tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UWORD uwLine, const char *szFile
) {
	WORD wSrcWidth, wSrcHeight, wDstWidth, wDstHeight;

	UBYTE isErr = 0;
	if(pSrc) {
#if defined(AMIGA)
		if(!bitmapIsChip(pSrc)) {
			isErr = 1;
			logWrite("ERR: Source is in FAST mem: %p (%p)\n", pSrc, pSrc->Planes[0]);
		}
#endif
		wSrcWidth = pSrc->BytesPerRow << 3;
		if(bitmapIsInterleaved(pSrc)) {
			wSrcWidth /= pSrc->Depth;
		}
		wSrcHeight = pSrc->Rows;
	}
	else {
		wSrcWidth = 0;
		wSrcHeight = 0;
	}

	if(pDst) {
#if defined(AMIGA)
		if(!bitmapIsChip(pDst)) {
			isErr = 1;
			logWrite("ERR: Dest is in FAST mem: %p (%p)\n", pDst, pDst->Planes[0]);
		}
#endif
		wDstWidth = pDst->BytesPerRow << 3;
		if(bitmapIsInterleaved(pDst)) {
			wDstWidth /= pDst->Depth;
		}
		wDstHeight = pDst->Rows;
	}
	else {
		wDstWidth = 0;
		wDstHeight = 0;
	}

	if(isErr) {
		return 1;
	}

	if(pSrc && (wSrcX < 0 || wSrcWidth < wSrcX + wWidth || wSrcHeight < wSrcY + wHeight)) {
		logWrite(
			"ERR: ILLEGAL BLIT Source out of range: "
			"source %p %dx%d, dest: %p %dx%d, blit: %d,%d -> %d,%d %dx%d (%s:%u)\n",
			pSrc,	wSrcWidth, wSrcHeight, pDst, wDstWidth, wDstHeight,
			wSrcX, wSrcY, wDstX, wDstY, wWidth, wHeight, szFile, uwLine
		);
		return 0;
	}
	if(pDst && (wDstY < 0 || wDstWidth < wDstX + wWidth || wDstHeight < wDstY + wHeight)) {
		logWrite(
			"ERR: ILLEGAL BLIT Dest out of range: "
			"source %p %dx%d, dest: %p %dx%d, blit: %d,%d -> %d,%d %dx%d (%s:%u)\n",
			pSrc,	wSrcWidth, wSrcHeight, pDst, wDstWidth, wDstHeight,
			wSrcX, wSrcY, wDstX, wDstY, wWidth, wHeight, szFile, uwLine
		);
		return 0;
	}
	if(pSrc && pDst && bitmapIsInterleaved(pSrc) && bitmapIsInterleaved(pDst)) {
		if(wHeight * pSrc->Depth > 1024) {
			logWrite(
				"ERR: Blit too big for OCS: height %hd, depth: %hhu, interleaved: %d (%s:%u)\n",
				wHeight, pSrc->Depth, wHeight * pSrc->Depth, szFile, uwLine
			);
		}
	}

	return 1;
}
#endif // defined(ACE_DEBUG)
