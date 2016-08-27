#ifndef GUARD_ACE_UTIL_PLANAR_H
#define GUARD_ACE_UTIL_PLANAR_H

#include <ace/config.h>
#include <ace/utils/bitmap.h>

/**
 *  @brief Returns color indices for 16 colors in a row starting from supplied
 *         coords.
 *  
 *  @param pBitMap Bitmap, from which pixel colors will be read
 *  @param uwX     Starting X coord, always word-aligned.
 *                 E.g. Read from pixel 18 will start it from x = 16 anyway.
 *  @param uwY     Row number, from which pixels will be read.
 *  @param pOut    Color index output buffer.
 */
void planarRead16(
	IN tBitMap *pBitMap,
	IN UWORD uwX,
	IN UWORD uwY,
	OUT UBYTE *pOut
);

/**
 *  @brief Returns color index of selected pixel.
 *  Inefficient as hell - use if really needed or for prototyping convenience!
 *  
 *  @param pBitMap Bitmap, from which pixel color will be read.
 *  @param uwX     Pixel X coord.
 *  @param uwY     Pixel Y coord.
 *  @return Pixels palette color index.
 *  
 *  @see planarRead()
 */
UBYTE planarRead(
	IN tBitMap *pBitMap,
	IN UWORD uwX,
	IN UWORD uwY
);

#endif