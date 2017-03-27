#include <ace/config.h>
#include <ace/managers/log.h>
#include "ace/utils/chunky.h"
#include "ace/utils/bitmap.h"
#include <math.h>

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

void chunkyRotate(UBYTE *pSource, UBYTE *pDest, float fAngle, UBYTE ubBgColor, WORD wWidth, WORD wHeight) {
	// VBCC 0.9e Amiga target doesn't have round() and sinf()
	#define round(x) (x >= 0? (WORD)(x+0.5) : (WORD)(x-0.5))
	#define sinf(x) (sin((double)x))
	#define cosf(x) (cos((double)x))
	float fSin, fCos, fCx, fCy;
	WORD x,y;
	WORD u,v;
		
	fCx = (wWidth-1)/2.0f;
	fCy = (wHeight-1)/2.0f;
	
	// For each of new bitmap's pixel sample color from rotated source x,y
	fSin = sinf(fAngle);
	fCos = cosf(fAngle);
	for(y = 0; y != wHeight; ++y) {
		for(x = 0; x != wWidth; ++x) {
			u = round(fCos*(x-fCx) - fSin*(y-fCy) +(fCx));
			v = round(fSin*(x-fCx) + fCos*(y-fCy) +(fCy));
			if(u < 0 || v < 0 || u >= wWidth || v >= wHeight)
				pDest[y*wWidth + x] = ubBgColor;
			else
				pDest[y*wWidth + x] = pSource[v*wWidth + u];
		}
	}
	
	#undef round
	#undef sinf
	#undef cosf
}
