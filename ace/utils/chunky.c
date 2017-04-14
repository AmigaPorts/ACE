#include <ace/config.h>
#include <ace/managers/log.h>
#include <ace/utils/chunky.h>
#include <ace/utils/bitmap.h>
#include <ace/libfixmath/fix16.h>

void chunkyFromPlanar16(tBitMap *pBitMap, UWORD uwX, UWORD uwY, UBYTE *pOut) {
	UWORD uwChunk, uwMask;
	UBYTE i, ubPx;
	for(ubPx = 0; ubPx != 16; ++ubPx)
		pOut[ubPx] = 0;
	// From highest to lowest color idx bit
	for(i = pBitMap->Depth; i--;) {
		// Obtain WORD from bitplane - 16 pixels
		uwChunk = ((UWORD*)(pBitMap->Planes[i]))[(pBitMap->BytesPerRow>>1)*uwY + (uwX>>4)];
		uwMask = 0x8000;                 // Start obtaining pixel values from left
		for(ubPx = 0; ubPx != 16; ++ubPx) { // Insert read pixel bit to right
			pOut[ubPx] = (pOut[ubPx] << 1) | ((uwChunk & uwMask) != 0);
			uwMask >>= 1;                  // Shift pixel mask right
		}
	}
}

UBYTE chunkyFromPlanar(tBitMap *pBitMap, UWORD uwX, UWORD uwY) {
	UBYTE pIndicesChunk[16];
	chunkyFromPlanar16(pBitMap, uwX, uwY, pIndicesChunk);
	return pIndicesChunk[uwX&0xF];
}

void chunkyToPlanar16(UBYTE *pIn, UWORD uwX, UWORD uwY, tBitMap *pOut) {
	UBYTE ubPlane, ubPixel;
	UWORD uwPlanarBuffer = 0;
	UWORD *pPlane;
	ULONG ulOffset;
	
	ulOffset = uwY*(pOut->BytesPerRow>>1) + (uwX >> 4);
	for(ubPlane = 0; ubPlane != pOut->Depth; ++ubPlane) {
		for(ubPixel = 0; ubPixel != 16; ++ubPixel) {
			uwPlanarBuffer <<= 1;
			if(pIn[ubPixel] & (1<<ubPlane))
				uwPlanarBuffer |= 1;
		}
		pPlane = (UWORD*)(pOut->Planes[ubPlane]);
		pPlane[ulOffset] = uwPlanarBuffer;
	}
}

void chunkyRotate(
	UBYTE *pSource, UBYTE *pDest,
	fix16_t fAngle, UBYTE ubBgColor,
	WORD wWidth, WORD wHeight
) {
	fix16_t fSin, fCos, fCx, fCy;
	WORD x,y;
	WORD u,v;
		
	fCx = fix16_div(fix16_from_int(wWidth-1), fix16_from_int(2));
	fCy = fix16_div(fix16_from_int(wHeight-1), fix16_from_int(2));
	
	// For each of new bitmap's pixel sample color from rotated source x,y
	fSin = fix16_sin(fAngle);
	fCos = fix16_cos(fAngle);
	fix16_t dx, dy;
	for(y = 0; y != wHeight; ++y) {
		dy = fix16_sub(fix16_from_int(y), fCy);
		for(x = 0; x != wWidth; ++x) {
			dx = fix16_sub(fix16_from_int(x), fCx);
			// u = round(fCos*(x-fCx) - fSin*(y-fCy) +(fCx));
			u = fix16_to_int(
				fix16_add(
					fix16_sub(
						fix16_mul(fCos, dx),
						fix16_mul(fSin, dy)),
					fCx
				)
			);
			// v = fix16_to_int(fSin*(x-fCx) + fCos*(y-fCy) +(fCy));
			v = fix16_to_int(
				fix16_add(
					fix16_add(
						fix16_mul(fSin, dx),
						fix16_mul(fCos, dy)),
					fCy
				)
			);
			
			if(u < 0 || v < 0 || u >= wWidth || v >= wHeight)
				pDest[y*wWidth + x] = ubBgColor;
			else
				pDest[y*wWidth + x] = pSource[v*wWidth + u];
		}
	}
}
