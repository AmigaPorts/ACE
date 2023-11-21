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
	UBYTE ubSrcDelta = wSrcX & 0xF;
	UBYTE ubDstDelta = wDstX & 0xF;
	UBYTE ubWidthDelta = (ubSrcDelta + wWidth) & 0xF;

	UBYTE ubBpp = MIN(pSrc->Depth, pDst->Depth);
	if(ubSrcDelta > ubDstDelta || ((wWidth + ubDstDelta + 15) & 0xFFF0) - (wWidth + ubSrcDelta) > 16) {
		// Shifting left - first processed word in row is rightmost
		UWORD uwBlitWidth = (wWidth + (ubSrcDelta > ubDstDelta ? ubSrcDelta : ubDstDelta) + 15) & 0xFFF0;
		UWORD uwBlitWords = uwBlitWidth >> 4;

		UBYTE ubMaskFShift = ((ubWidthDelta + 15) & 0xF0) - ubWidthDelta;
		UBYTE ubMaskLShift = uwBlitWidth - (wWidth + ubMaskFShift);
		UWORD uwFirstMask = 0xFFFF << ubMaskFShift;
		UWORD uwLastMask = 0xFFFF >> ubMaskLShift;
		if(ubMaskLShift > 16) { // Fix for 2-word blits
			uwFirstMask &= 0xFFFF >> (ubMaskLShift - 16);
		}

		UBYTE ubShift = uwBlitWidth - (ubDstDelta + wWidth + ubMaskFShift);

		ULONG ulSrcOffs = pSrc->BytesPerRow * (wSrcY + wHeight - 1) + ((wSrcX + wWidth + ubMaskFShift - 1) / 16) * 2;
		ULONG ulDstOffs = pDst->BytesPerRow * (wDstY + wHeight - 1) + ((wDstX + wWidth + ubMaskFShift - 1) / 16) * 2;
		WORD wSrcModulo = pSrc->BytesPerRow - uwBlitWords * 2;
		WORD wDstModulo = pDst->BytesPerRow - uwBlitWords * 2;
		for(UBYTE ubPlane = 0; ubPlane < ubBpp; ++ubPlane) {
			UWORD *pPosB = ((UWORD*)&pSrc->Planes[ubPlane][ulSrcOffs]);
			UWORD *pPosCd = ((UWORD*)&pDst->Planes[ubPlane][ulDstOffs]);
			for(UWORD uwRow = 0; uwRow < wHeight; ++uwRow) {
				// *pPosCd |= 0b0101010101010101; // debug row start
				UWORD uwBarrelB = 0;
				UWORD uwBarrelA = 0;
				for(UWORD uwCol = 0; uwCol < uwBlitWords; ++uwCol) {
					UWORD uwDataA = 0xFFFF;
					if(uwCol == 0) {
						uwDataA &= uwFirstMask;
					}
					if(uwCol == uwBlitWords - 1) {
						uwDataA &= uwLastMask;
					}
					UWORD uwNewBarrel = uwDataA >> (16 - ubShift);
					uwDataA = (uwDataA << ubShift) | uwBarrelA;
					uwBarrelA = uwNewBarrel;

					UWORD uwDataB = endianBigToNative16(*pPosB);
					uwNewBarrel = uwDataB >> (16 - ubShift);
					uwDataB = (uwDataB << ubShift) | uwBarrelB;
					uwBarrelB = uwNewBarrel;

					UWORD uwDataC = endianBigToNative16(*pPosCd);

					switch(ubMinterm) {
						case MINTERM_COOKIE:
							*pPosCd = endianNativeToBig16((uwDataA & uwDataB) | (~uwDataA & uwDataC));
							break;
						case MINTERM_COPY:
							*pPosCd = endianNativeToBig16(uwDataB);
							break;
						case MINTERM_AB_OR_C:
							*pPosCd = endianNativeToBig16((uwDataA & uwDataB) | uwDataC);
							break;
						case MINTERM_CLEAR_C_ON_AB:
							*pPosCd = endianNativeToBig16(~(uwDataA & uwDataB) & uwDataC);
							break;
						default:
							logWrite("ERR: Unhandled minterm: %hhu\n", ubMinterm);
							break;
					}
					--pPosB;
					--pPosCd;
				}
				// *pPosCd |= 0b0000111100001111; // debug row end
				pPosB -= wSrcModulo / 2;
				pPosCd -= wDstModulo / 2;
			}
		}
	}
	else {
		// Shifting right - first processed word in row is leftmost
		UWORD uwBlitWidth = (wWidth + ubDstDelta + 15) & 0xFFF0;
		UWORD uwBlitWords = uwBlitWidth / 16;

		UBYTE ubMaskFShift = ubSrcDelta;
		UBYTE ubMaskLShift = uwBlitWidth - (wWidth + ubSrcDelta);

		UWORD uwFirstMask = 0xFFFF >> ubMaskFShift;
		UWORD uwLastMask = 0xFFFF << ubMaskLShift;

		UBYTE ubShift = ubDstDelta - ubSrcDelta;

		ULONG ulSrcOffs = pSrc->BytesPerRow * wSrcY + (wSrcX / 16) * 2;
		ULONG ulDstOffs = pDst->BytesPerRow * wDstY + (wDstX / 16) * 2;
		WORD wSrcModulo = pSrc->BytesPerRow - uwBlitWords * 2;
		WORD wDstModulo = pDst->BytesPerRow - uwBlitWords * 2;
		for(UBYTE ubPlane = 0; ubPlane < ubBpp; ++ubPlane) {
			UWORD *pPosB = ((UWORD*)&pSrc->Planes[ubPlane][ulSrcOffs]);
			UWORD *pPosCd = ((UWORD*)&pDst->Planes[ubPlane][ulDstOffs]);
			for(UWORD uwRow = 0; uwRow < wHeight; ++uwRow) {
				// *pPosCd = 0xFFFF; // debug row start
				UWORD uwBarrelA = 0;
				UWORD uwBarrelB = 0;
				for(UWORD uwCol = 0; uwCol < uwBlitWords; ++uwCol) {
					UWORD uwDataA = 0xFFFF;
					if(uwCol == 0) {
						uwDataA &= uwFirstMask;
					}
					if(uwCol == uwBlitWords - 1) {
						uwDataA &= uwLastMask;
					}
					UWORD uwNewBarrel = uwDataA << (16 - ubShift);
					uwDataA = (uwDataA >> ubShift) | uwBarrelA;
					uwBarrelA = uwNewBarrel;

					UWORD uwDataB = endianBigToNative16(*pPosB);
					uwNewBarrel = uwDataB << (16 - ubShift);
					uwDataB = (uwDataB >> ubShift) | uwBarrelB;
					uwBarrelB = uwNewBarrel;

					UWORD uwDataC = endianBigToNative16(*pPosCd);

					switch(ubMinterm) {
						case MINTERM_COOKIE:
							*pPosCd = endianNativeToBig16((uwDataA & uwDataB) | (~uwDataA & uwDataC));
							break;
						case MINTERM_COPY:
							*pPosCd = endianNativeToBig16(uwDataB);
						case MINTERM_AB_OR_C:
							*pPosCd = endianNativeToBig16((uwDataA & uwDataB) | uwDataC);
							break;
						case MINTERM_CLEAR_C_ON_AB:
							*pPosCd = endianNativeToBig16(~(uwDataA & uwDataB) & uwDataC);
							break;
						default:
							logWrite("ERR: Unhandled minterm: %hhu\n", ubMinterm);
							break;
					}
					++pPosB;
					++pPosCd;
				}
				// *pPosCd = 0b1010101010101010; // debug row end
				pPosB += wSrcModulo / 2;
				pPosCd += wDstModulo / 2;
			}
		}
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
