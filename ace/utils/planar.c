#include <ace/config.h>
#include <ace/managers/log.h>
#include "ace/utils/planar.h"
#include "ace/utils/bitmap.h"

/**
 * Returns color indices for 16 colors in a row starting from uwX%16,uwY
 */
void planarRead16(tBitMap *pBitMap, UWORD uwX, UWORD uwY, UBYTE *pOut) {
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

/**
 * Returns color index of selected pixel.
 * Inefficient as hell - use if really needed and for prototyping convenience!
 */
UBYTE planarRead(tBitMap *pBitMap, UWORD uwX, UWORD uwY) {
	UBYTE pIndicesChunk[16];
	planarRead16(pBitMap, uwX, uwY, pIndicesChunk);
	return pIndicesChunk[uwX&0xF];
}