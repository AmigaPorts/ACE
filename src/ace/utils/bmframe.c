/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/bmframe.h>

typedef enum _tBorderTile {
	BORDER_TILE_NW = 0,
	BORDER_TILE_N = 1,
	BORDER_TILE_NE = 2,
	BORDER_TILE_W = 3,
	BORDER_TILE_MID = 4,
	BORDER_TILE_E = 5,
	BORDER_TILE_SW = 6,
	BORDER_TILE_S = 7,
	BORDER_TILE_SE = 8,
} _tBorderTile;

void bmFrameDraw(
	const tBitMap *pFrameSet, tBitMap *pDest,
	UWORD uwX, UWORD uwY, UBYTE ubCols, UBYTE ubRows, UBYTE ubTileSize
) {
	// Vertices
	blitCopyAligned(
		pFrameSet, 0, BORDER_TILE_NW * ubTileSize, pDest,
		uwX, uwY, ubTileSize, ubTileSize
	);
	blitCopyAligned(
		pFrameSet, 0, BORDER_TILE_NE * ubTileSize, pDest,
		uwX + ((ubCols - 1) * ubTileSize), uwY, ubTileSize, ubTileSize
	);
	blitCopyAligned(
		pFrameSet, 0, BORDER_TILE_SW * ubTileSize, pDest,
		uwX, uwY + ((ubRows - 1) * ubTileSize), ubTileSize, ubTileSize
	);
	blitCopyAligned(
		pFrameSet, 0, BORDER_TILE_SE * ubTileSize, pDest,
		uwX + ((ubCols - 1) * ubTileSize), uwY + ((ubRows - 1) * ubTileSize),
		ubTileSize, ubTileSize
	);

	// Horizontal edges
	for(UBYTE i = 1; i < ubCols-1; ++i) {
		blitCopyAligned(
			pFrameSet, 0, BORDER_TILE_N * ubTileSize, pDest,
			uwX + (i * ubTileSize), uwY,
			ubTileSize, ubTileSize
		);
		blitCopyAligned(
			pFrameSet, 0, BORDER_TILE_S * ubTileSize, pDest,
			uwX + (i * ubTileSize), uwY + ((ubRows - 1) * ubTileSize),
			ubTileSize, ubTileSize
		);
	}

	// Middle rows
	if(ubRows > 2) {
		// Draw only first row
		blitCopyAligned(
			pFrameSet, 0, BORDER_TILE_W * ubTileSize, pDest,
			uwX, uwY + ubTileSize, ubTileSize, ubTileSize
		);
		blitCopyAligned(
			pFrameSet, 0, BORDER_TILE_E * ubTileSize, pDest,
			uwX + ((ubCols-1) * ubTileSize), uwY + ubTileSize, ubTileSize, ubTileSize
		);
		for(UBYTE i = 1; i < ubCols-1; ++i) {
			blitCopyAligned(
				pFrameSet, 0, BORDER_TILE_MID * ubTileSize, pDest,
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
