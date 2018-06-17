/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/types.h>
#include <ace/managers/log.h>
#include <ace/utils/chunky.h>
#include <ace/utils/bitmap.h>
#include <fixmath/fix16.h>

void chunkyFromPlanar16(
	const tBitMap *pBitMap, UWORD uwX, UWORD uwY, UBYTE *pOut
) {
	UWORD uwChunk, uwMask;
	UBYTE i, ubPx;
	memset(pOut, 0, 16*sizeof(*pOut));
	// From highest to lowest color idx bit
	for(i = pBitMap->Depth; i--;) {
		// Obtain WORD from bitplane - 16 pixels
		uwChunk = ((UWORD*)(pBitMap->Planes[i]))[(pBitMap->BytesPerRow>>1)*uwY + (uwX>>4)];
		uwMask = 0x8000; // Start obtaining pixel values from left
		for(ubPx = 0; ubPx != 16; ++ubPx) { // Insert read pixel bit to right
			pOut[ubPx] = (pOut[ubPx] << 1) | ((uwChunk & uwMask) != 0);
			uwMask >>= 1; // Shift pixel mask right
		}
	}
}

UBYTE chunkyFromPlanar(const tBitMap *pBitMap, UWORD uwX, UWORD uwY) {
	UBYTE pIndicesChunk[16];
	chunkyFromPlanar16(pBitMap, uwX, uwY, pIndicesChunk);
	return pIndicesChunk[uwX&0xF];
}

void chunkyToPlanar16(const UBYTE *pIn, UWORD uwX, UWORD uwY, tBitMap *pOut) {
	UBYTE ubPlane, ubPixel;
	UWORD uwPlanarBuffer = 0;
	UWORD *pPlane;
	ULONG ulOffset;

	ulOffset = uwY*(pOut->BytesPerRow>>1) + (uwX >> 4);
	for(ubPlane = 0; ubPlane != pOut->Depth; ++ubPlane) {
		for(ubPixel = 0; ubPixel != 16; ++ubPixel) {
			uwPlanarBuffer <<= 1;
			if(pIn[ubPixel] & (1<<ubPlane)) {
				uwPlanarBuffer |= 1;
			}
		}
		pPlane = (UWORD*)(pOut->Planes[ubPlane]);
		pPlane[ulOffset] = uwPlanarBuffer;
	}
}

void chunkyToPlanar(UBYTE ubIn, UWORD uwX, UWORD uwY, tBitMap *pOut) {
	UBYTE pChunky[16];
	chunkyFromPlanar16(pOut, uwX, uwY, pChunky);
	pChunky[uwX & 15] = ubIn;
	chunkyToPlanar16(pChunky, uwX, uwY, pOut);
}

void chunkyRotate(
	const UBYTE *pSource, UBYTE *pDest, fix16_t fSin, fix16_t fCos,
	UBYTE ubBgColor, WORD wWidth, WORD wHeight
) {
	fix16_t fCx, fCy;
	fix16_t fHalf = fix16_one>>1;
	WORD x,y;
	WORD u,v;

	fCx = fix16_div(fix16_from_int(wWidth), fix16_from_int(2));
	fCy = fix16_div(fix16_from_int(wHeight), fix16_from_int(2));

	// For each of new bitmap's pixel sample color from rotated source x,y
	fix16_t fDx, fDy;
	for(y = 0; y != wHeight; ++y) {
		fDy = fix16_from_int(y) - fCy;
		for(x = 0; x != wWidth; ++x) {
			fDx = fix16_from_int(x) - fCx;
			// u = round(fCos*(x-fCx) - fSin*(y-fCy) +(fCx));
			// v = fix16_to_int(fSin*(x-fCx) + fCos*(y-fCy) +(fCy));
			u = fix16_to_int(fix16_mul(fCos, fDx) - fix16_mul(fSin, fDy) + fCx + fHalf);
			v = fix16_to_int(fix16_mul(fSin, fDx) + fix16_mul(fCos, fDy) + fCy + fHalf);

			if(u < 0 || v < 0 || u >= wWidth || v >= wHeight) {
				pDest[y*wWidth + x] = ubBgColor;
			}
			else {
				pDest[y*wWidth + x] = pSource[v*wWidth + u];
			}
		}
	}
}

void chunkyFromBitmap(
	const tBitMap *pBitmap, UBYTE *pChunky,
	UWORD uwSrcOffsX, UWORD uwSrcOffsY, UWORD uwWidth, UWORD uwHeight
) {
	for(UWORD y = 0; y < uwHeight; ++y) {
		for(UWORD x = 0; x < uwWidth; x += 16) {
			chunkyFromPlanar16(
				pBitmap, uwSrcOffsX + x, uwSrcOffsY + y, &pChunky[y*uwWidth + x]
			);
		}
	}
}

void chunkyToBitmap(
	const UBYTE *pChunky, tBitMap *pBitmap,
	UWORD uwDstOffsX, UWORD uwDstOffsY, UWORD uwWidth, UWORD uwHeight
) {
	for(UWORD y = 0; y < uwHeight; ++y) {
		for(UWORD x = 0; x < uwWidth; x += 16)
			chunkyToPlanar16(
				&pChunky[(y*uwWidth) + x],
				uwDstOffsX + x, uwDstOffsY + y,
				pBitmap
			);
	}
}
