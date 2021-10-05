/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_BMFRAME_H_
#define _ACE_UTILS_BMFRAME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/utils/bitmap.h>
#include <ace/managers/blit.h>

/**
 * @brief Draws the border from passed tileset.
 *
 * @param pFrameSet Tileset. Must consist of 9 tiles: NW, N, NE, W, MID, E, SW, S, SE.
 * @param pDest Destination bitmap.
 * @param uwX
 * @param uwY
 * @param ubCols
 * @param ubRows
 * @param ubTileSize
 */
void bmFrameDraw(
	const tBitMap *pFrameSet, tBitMap *pDest,
	UWORD uwX, UWORD uwY, UBYTE ubCols, UBYTE ubRows, UBYTE ubTileSize
);

#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_BMFRAME_H_
