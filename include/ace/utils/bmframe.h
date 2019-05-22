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

void bmFrameDraw(
	tBitMap *pFrameSet, tBitMap *pDest,
	UWORD uwX, UWORD uwY, UBYTE ubCols, UBYTE ubRows, UBYTE ubTileSize
);

#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_BMFRAME_H_
