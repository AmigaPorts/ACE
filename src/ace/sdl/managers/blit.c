/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/blit.h>
#include <ace/utils/endian.h>

void blitManagerCreate(void) {
	logBlockBegin("blitManagerCreate");
	logBlockEnd("blitManagerCreate");
}

void blitManagerDestroy(void) {
	logBlockBegin("blitManagerDestroy");
	logBlockEnd("blitManagerDestroy");
}

UBYTE blitIsIdle(void) {
	return 1;
}

UBYTE blitUnsafeCopy(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubMinterm
) {
	// blitRect(pDst, wDstX, wDstY, wWidth, wHeight, 3);

	UBYTE ubSrcShift = ((UWORD)wSrcX) % 16;
	UBYTE ubDstShift = ((UWORD)wDstX) % 16;

	UBYTE ubBpp = MIN(pSrc->Depth, pDst->Depth);
	if(ubSrcShift <= ubDstShift) {
		// Shifting right
		UBYTE ubShift = ubDstShift - ubSrcShift;
		UWORD uwFirstWordCd = wDstX / 16;
		UWORD uwLastWordCd = ((wDstX + wWidth - 1) / 16);
		UWORD uwColLast = uwLastWordCd - uwFirstWordCd;
		UWORD uwFirstWordB = wSrcX / 16;
		UWORD uwFirstWordMask = 0xFFFF >> ubDstShift;
		UWORD uwLastWordMask = 0xFFFF << (16 - (((ubDstShift + wWidth - 1) % 16) + 1));
		for(UBYTE ubPlane = 0; ubPlane < ubBpp; ++ubPlane) {
			UWORD *pPosB = ((UWORD*)&pSrc->Planes[ubPlane][0]) + uwFirstWordB + (pSrc->BytesPerRow / 2) * wSrcY;
			UWORD *pPosCd = ((UWORD*)&pDst->Planes[ubPlane][0]) + uwFirstWordCd + (pDst->BytesPerRow / 2) * wDstY;
			for(UWORD uwRow = 0; uwRow < wHeight; ++uwRow) {
				// *pPosCd = 0xFFFF; // debug row start
				UWORD uwBarrelB = 0;
				for(UWORD uwCol = 0; uwCol <= uwColLast; ++uwCol) {
					UWORD uwDataB = (endianBigToNative16(*pPosB) >> ubShift);
					UWORD uwDataC = endianBigToNative16(*pPosCd);
					UWORD uwDataA = 0xFFFF;
					if(uwCol == 0) {
						uwDataA &= uwFirstWordMask;
					}
					if(uwCol == uwColLast) {
						uwDataB = 0;
						uwDataA &= uwLastWordMask;
					}
					uwDataB |= uwBarrelB;
					uwBarrelB = endianBigToNative16(*pPosB) << (16 - ubShift);

					switch(ubMinterm) {
						case MINTERM_COOKIE:
							*pPosCd = endianNativeToBig16((uwDataA & uwDataB) | (~uwDataA & uwDataC));
							break;
						case MINTERM_COPY:
							*pPosCd = endianNativeToBig16(uwDataB);
						default:
							break;
					}
					++pPosB;
					++pPosCd;
				}
				// *pPosCd = 0b1010101010101010; // debug row end
				pPosB += (pSrc->BytesPerRow / 2) - (uwColLast + 1);
				pPosCd += (pDst->BytesPerRow / 2) - (uwColLast + 1);
			}
		}
	}
	else {
		// Shifting left
	}
	return 1;
}

UBYTE blitUnsafeCopyAligned(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight
) {
	UBYTE ubPlaneCount = MIN(pDst->Depth, pSrc->Depth);
	for(UBYTE ubPlane = 0; ubPlane < ubPlaneCount; ++ubPlane) {
		UWORD *pDstPlane = (UWORD*)pDst->Planes[ubPlane];
		UWORD *pSrcPlane = (UWORD*)pSrc->Planes[ubPlane];
		for(UWORD uwRow = 0; uwRow < wHeight; ++uwRow) {
			for(UWORD uwCol = 0; uwCol < wWidth / 16; ++uwCol) {
				ULONG ulDstPos = wDstX / 16 + uwCol + (wDstY + uwRow) * pDst->BytesPerRow / 2;
				ULONG ulSrcPos = wSrcX / 16 + uwCol + (wSrcY + uwRow) * pSrc->BytesPerRow / 2;
				pDstPlane[ulDstPos] = pSrcPlane[ulSrcPos];
			}
		}
	}
	return 1;
}

UBYTE blitUnsafeCopyMask(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, const UBYTE *pMsk
) {
	return 1;
}

UBYTE blitUnsafeRect(
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubColor
) {
	UWORD uwFirstWord = wDstX / 16;
	UWORD uwLastWord = ((wDstX + wWidth - 1) / 16);
	UWORD uwFirstWordMask = 0xFFFF >> (wDstX % 16);
	UWORD uwLastWordMask = 0xFFFF << (16 - (((wDstX + wWidth - 1) % 16) + 1));
	// TODO: endian could be done only done on masks and then skipped
	for(UBYTE ubPlane = 0; ubPlane < pDst->Depth; ++ubPlane) {
		UWORD *pPlane = (UWORD*)pDst->Planes[ubPlane];
		for(UWORD uwY = wDstY; uwY < wDstY + wHeight; ++uwY) {
			for(UWORD uwWordX = uwFirstWord; uwWordX <= uwLastWord; ++uwWordX) {
				UWORD uwData = 0xFFFF;
				if(uwWordX == uwFirstWord) {
					uwData &= uwFirstWordMask;
				}
				if(uwWordX == uwLastWord) {
					uwData &= uwLastWordMask;
				}

				ULONG ulPos = uwWordX + (pDst->BytesPerRow / 2) * uwY;
				if(ubColor & 1) {
					pPlane[ulPos] = endianNativeToBig16(endianBigToNative16(pPlane[ulPos]) | uwData);
				}
				else {
					pPlane[ulPos] = endianNativeToBig16(endianBigToNative16(pPlane[ulPos]) & ~uwData);
				}
			}
		}
		ubColor >>= 1;
	}
	return 1;
}

void blitLine(
	tBitMap *pDst, WORD x1, WORD y1, WORD x2, WORD y2,
	UBYTE ubColor, UWORD uwPattern, UBYTE isOneDot
) {
	return;
}
