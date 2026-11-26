/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _SHOWCASE_EXAMPLES_SHOOTER_VERTICAL_ART_H_
#define _SHOWCASE_EXAMPLES_SHOOTER_VERTICAL_ART_H_
/** @file Procedural OCS art; returned bitmaps belong to the caller. */
#include <ace/utils/bitmap.h>

/** Fill all 32 OCS colors, including the shared attached-sprite palette. */
void shooterVertArtPalette(UWORD *pPalette);
/** Create one 16x18 2bpp player half, with blank DMA header/footer rows. */
tBitMap *shooterVertArtPlayer(UBYTE ubHalf);
/** Create a pixel-only, interleaved 16x24 4bpp strip of two enemy frames. */
tBitMap *shooterVertArtEnemies(void);
/** Create a pixel-only 16x4 2bpp shot; its occupied rectangle is x=0..1. */
tBitMap *shooterVertArtShot(UBYTE ubColor);
/** Draw the static navy starfield and title once, using CPU writes only. */
void shooterVertArtBackground(tBitMap *pBitmap);
/** Redraw HUD fields only when their corresponding values changed. */
void shooterVertArtHud(
	tBitMap *pBitmap, ULONG ulScore, UBYTE ubLives, UBYTE isGameOver,
	UBYTE isScoreDirty, UBYTE isLivesDirty, UBYTE isStatusDirty
);
#endif // _SHOWCASE_EXAMPLES_SHOOTER_VERTICAL_ART_H_
