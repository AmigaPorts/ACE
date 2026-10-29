/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _SHOWCASE_EXAMPLES_SHOOTER_HORIZONTAL_ART_H_
#define _SHOWCASE_EXAMPLES_SHOOTER_HORIZONTAL_ART_H_
/** @file Procedural OCS art; returned bitmaps belong to the caller. */
#include <ace/utils/bitmap.h>
#include "examples/shooter_horizontal/logic.h"

#define SHOOTER_ART_DMA_HEADER_ROWS 1
#define SHOOTER_ART_DMA_FOOTER_ROWS 1

/** Fill all 32 OCS colors, including the shared attached-sprite palette. */
void shooterHorzArtPalette(UWORD *pPalette);
/** Create one 16x18 2bpp player half, with blank DMA header/footer rows.
 * Right-facing pixels: output[y][x] = original upward map[15-x][y].
 * Each half must stay alive and writable until basic manager teardown. */
tBitMap *shooterHorzArtPlayer(UBYTE ubHalf);
/** Create a pixel-only, interleaved 16x24 4bpp strip of two enemy frames. */
tBitMap *shooterHorzArtEnemies(void);
/** Create pixel-only 16x2 2bpp art, occupied x=0..5; local color 1 or 2.
 * Multiplexed sprites borrow this bitmap: keep it until manager teardown. */
tBitMap *shooterHorzArtShot(UBYTE ubColor);
/** Draw the static STAR SENTINEL title and HUD divider once. */
void shooterHorzArtBackground(tBitMap *pBitmap);
/** Redraw HUD fields only when their corresponding values changed. */
void shooterHorzArtHud(
	tBitMap *pBitmap, ULONG ulScore, UBYTE ubLives, UBYTE isGameOver,
	UBYTE isScoreDirty, UBYTE isLivesDirty, UBYTE isStatusDirty
);
#endif // _SHOWCASE_EXAMPLES_SHOOTER_HORIZONTAL_ART_H_
