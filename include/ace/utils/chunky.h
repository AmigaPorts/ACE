#ifndef GUARD_ACE_UTIL_CHUNKY_H
#define GUARD_ACE_UTIL_CHUNKY_H

#include <ace/types.h>
#include <ace/utils/bitmap.h>
#include <fixmath/fix16.h>

/**
 *  @brief Returns color indices for 16 colors in a row starting from supplied
 *         coords.
 *  
 *  @param pBitMap Bitmap, from which pixel colors will be read
 *  @param uwX     Starting X coord, always word-aligned.
 *                 E.g. Read from pixel 18 will start it from x = 16 anyway.
 *  @param uwY     Row number, from which pixels will be read.
 *  @param pOut    Color index output buffer.
 *  
 *  @see chunkyFromPlanar()
 */
void chunkyFromPlanar16(
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
 *  @see chunkyFromPlanar16()
 */
UBYTE chunkyFromPlanar(
	IN tBitMap *pBitMap,
	IN UWORD uwX,
	IN UWORD uwY
);

/**
 *  @brief Rotates chunky pixels by given angle, on spefied background.
 *  
 *  This function uses fixed point from libfixmath, so its speed should be
 *  acceptable for precalcs.
 *  Also, dr Dobb's implementation is faster, but yields greater errors:
 *  http://www.drdobbs.com/architecture-and-design/fast-bitmap-rotation-and-scaling/184416337
 *  
 *  @param pSource   Source chunky pixels.
 *  @param pDest     Destination chunky pixels.
 *  @param fAngle    Rotation angle, in radians.
 *  @param ubBgColor Background color to use if rotation goes out of source.
 *  @param wWidth    Source/destination chunky map width.
 *  @param wHeight   Ditto, height.
 */
void chunkyRotate(
	IN UBYTE *pSource,
	OUT UBYTE *pDest,
	IN fix16_t fAngle,
	IN UBYTE ubBgColor,
	IN WORD wWidth,
	IN WORD wHeight
);

/**
 *  @brief Puts 16-pixel chunky row on bitmap at given coordinates.
 *  
 *  This function assumes that chunky pixels are of same depth as bitmap.
 *  Higher chunky bits will thus be ignored.
 *  
 *  @param pIn  Source chunky pixels.
 *  @param uwX  Destination start X coordinate.
 *  @param uwY  Destination Y coordinate.
 *  @param pOut Destination bitmap.
 *  
 *  @todo Implement using 32-bit fixed points & Taylor sine approximation.
 *  @see chunkyFromPlanar16
 */
void chunkyToPlanar16(
	IN UBYTE *pIn,
	IN UWORD uwX,
	IN UWORD uwY,
	OUT tBitMap *pOut
);


#endif
