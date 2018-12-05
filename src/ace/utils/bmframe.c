/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/bmframe.h>

void bmFrameDraw(
	tBitMap *pFrameSet, tBitMap *pDest,
	UWORD uwX, UWORD uwY, UBYTE ubCols, UBYTE ubRows, UBYTE ubTileSize
) {
	// Vertices
	blitCopyAligned(pFrameSet, 0, 0, pDest, uwX, uwY, ubTileSize, ubTileSize);
	blitCopyAligned(
		pFrameSet, ubTileSize * 2, 0, pDest,
		uwX + ((ubCols - 1) * ubTileSize), uwY, ubTileSize, ubTileSize
	);
	blitCopyAligned(
		pFrameSet, 0, ubTileSize * 2, pDest,
		uwX, uwY + ((ubRows - 1) * ubTileSize), ubTileSize, ubTileSize
	);
	blitCopyAligned(
		pFrameSet, ubTileSize * 2, ubTileSize * 2, pDest,
		uwX + ((ubCols - 1) * ubTileSize), uwY + ((ubRows - 1) * ubTileSize),
		ubTileSize, ubTileSize
	);
	// Horizontal edges
	for(UBYTE i = 1; i < ubCols-1; ++i) {
		blitCopyAligned(
			pFrameSet, ubTileSize, 0, pDest, uwX+(i * ubTileSize), uwY,
			ubTileSize, ubTileSize
		);
		blitCopyAligned(
			pFrameSet, ubTileSize, ubTileSize * 2, pDest, uwX + (i * ubTileSize),
			uwY+((ubRows-1) * ubTileSize), ubTileSize, ubTileSize
		);
	}
	// Center
	if(ubRows > 2) {
		// Draw only first row
		blitCopyAligned(
			pFrameSet, 0, ubTileSize, pDest,
			uwX, uwY + ubTileSize, ubTileSize, ubTileSize
		);
		blitCopyAligned(
			pFrameSet, ubTileSize * 2, ubTileSize, pDest,
			uwX + ((ubCols-1) * ubTileSize), uwY + ubTileSize, ubTileSize, ubTileSize
		);
		for(UBYTE i = 1; i < ubCols-1; ++i) {
			blitCopyAligned(
				pFrameSet, ubTileSize, ubTileSize, pDest,
				uwX + (i * ubTileSize), uwY + ubTileSize, ubTileSize, ubTileSize
			);
		}
		// Fill rest of rows with the first one
		for(UBYTE i = 2; i < ubRows - 1; ++i) {
			blitCopyAligned(
				pDest, uwX, uwY+ubTileSize, pDest, uwX, uwY+(i * ubTileSize),
				ubCols * ubTileSize, ubTileSize
			);
		}
	}
}
