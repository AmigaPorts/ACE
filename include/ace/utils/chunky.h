/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_CHUNKY_H_
#define _ACE_UTILS_CHUNKY_H_

#include <ace/types.h>
#include <ace/utils/bitmap.h>
#include <fixmath/fix16.h>

/**
 * @brief Returns color indices for 16 colors in a row starting from supplied
 *        coords.
 *
 * @param pBitMap Bitmap, from which pixel colors will be read
 * @param uwX     Starting X coord, always word-aligned.
 *                E.g. Read from pixel 18 will start it from x = 16 anyway.
 * @param uwY     Row number, from which pixels will be read.
 * @param pOut    Color index output buffer.
 *
 * @see chunkyFromPlanar()
 */
void chunkyFromPlanar16(
	const tBitMap *pBitMap, UWORD uwX, UWORD uwY, UBYTE *pOut
);

/**
 * @brief Returns color index of selected pixel.
 * Inefficient as hell - use if really needed or for prototyping convenience!
 *
 * @param pBitMap Bitmap, from which pixel color will be read.
 * @param uwX     Pixel X coord.
 * @param uwY     Pixel Y coord.
 * @return Pixels palette color index.
 *
 * @see chunkyFromPlanar16()
 */
UBYTE chunkyFromPlanar(const tBitMap *pBitMap, UWORD uwX, UWORD uwY);

/**
 * @brief Rotates chunky pixels by given angle, on spefied background.
 *
 * This function uses fixed point from libfixmath, so its speed should be
 * acceptable for precalcs.
 * Also, dr Dobb's implementation is faster, but yields greater errors:
 * http://www.drdobbs.com/architecture-and-design/fast-bitmap-rotation-and-scaling/184416337
 *
 * @param pSource   Source chunky pixels.
 * @param pDest     Destination chunky pixels.
 * @param fSin      Sine value of rotation angle.
 * @param fCos      Cosine value of rotation angle.
 * @param ubBgColor Background color to use if rotation goes out of source.
 * @param wWidth    Source/destination chunky map width.
 * @param wHeight   Ditto, height.
 */
void chunkyRotate(
	const UBYTE *pSource, UBYTE *pDest,
	fix16_t fSin, fix16_t fCos,
	UBYTE ubBgColor, WORD wWidth, WORD wHeight
);

/**
 * @brief Puts 16-pixel chunky row on bitmap at given coordinates.
 *
 * This function assumes that chunky pixels are of same depth as bitmap.
 * Higher chunky bits will thus be ignored.
 *
 * @param pIn  Source chunky pixels.
 * @param uwX  Destination start X coordinate.
 * @param uwY  Destination Y coordinate.
 * @param pOut Destination bitmap.
 *
 * @see chunkyFromPlanar16
 * @see chunkyToPlanar
 */
void chunkyToPlanar16(const UBYTE *pIn, UWORD uwX, UWORD uwY, tBitMap *pOut);

/**
 * Puts single chunky pixel on bitmap at given coordinates.
 *
 * Inefficient as hell - use if really needed or for prototyping convenience!
 *
 * @param ubIn: Chunky pixel value (color index).
 * @param uwX: Pixel's x position on bitmap.
 * @param uwY: Pixel's y position on bitmap.
 * @param pOut: Destination bitmap.
 *
 * @see chunkyToPlanar16
 * @see chunkyFromPlanar16
 * @see chunkyFromPlanar
 */
void chunkyToPlanar(UBYTE ubIn, UWORD uwX, UWORD uwY, tBitMap *pOut);

/**
 * @brief Reads given portion of bitmap to chunky buffer.
 *
 * @param pBitmap Source bitmap image.
 * @param pChunky Desitination chunky buffer.
 * @param uwSrcOffsX X offset of conversion area, in pixels.
 * @param uwSrcOffsY Y offset of conversion area, in pixels.
 * @param uwWidth Width of conversion area, in pixels.
 * @param uwHeight Height of conversion area, in pixels.
 *
 * @see chunkyToBitmap
 */
void chunkyFromBitmap(
	const tBitMap *pBitmap, UBYTE *pChunky,
	UWORD uwSrcOffsX, UWORD uwSrcOffsY, UWORD uwWidth, UWORD uwHeight
);

/**
 * @brief Writes given chunky buffer into specified portion of bitmap
 *
 * @param pChunky Source chunky data buffer.
 * @param pBitmap Destination bitmap.
 * @param uwDstOffsX X offset of conversion area, in pixels.
 * @param uwDstOffsY Y offset of conversion area, in pixels.
 * @param uwWidth Width of conversion area, in pixels.
 * @param uwHeight Height of conversion area, in pixels.
 */
void chunkyToBitmap(
	const UBYTE *pChunky, tBitMap *pBitmap,
	UWORD uwDstOffsX, UWORD uwDstOffsY, UWORD uwWidth, UWORD uwHeight
);

#endif // _ACE_UTILS_CHUNKY_H_
