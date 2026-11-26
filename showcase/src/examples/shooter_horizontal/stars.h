/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _SHOWCASE_EXAMPLES_SHOOTER_HORIZONTAL_STARS_H_
#define _SHOWCASE_EXAMPLES_SHOOTER_HORIZONTAL_STARS_H_
/** @file Allocation-free, per-bitmap planar parallax starfield. */
#include <ace/utils/bitmap.h>

#define SHOOTER_STARS_LAYERS 3
#define SHOOTER_STARS_COUNT 32
#define SHOOTER_STARS_WIDTH 320
#define SHOOTER_STARS_Y_MIN 26
#define SHOOTER_STARS_Y_COUNT 226
#define SHOOTER_STARS_SEED 0x53484152UL

typedef struct _tShooterStar {
	UWORD uwInitialX, uwX;
	ULONG ulRowOffset;
	/* Plane-relative cache supports separate and interleaved bitmap layouts. */
	ULONG ulByteOffset;
	UBYTE ubMask;
} tShooterStar;

typedef struct _tShooterStars {
	tShooterStar pStars[SHOOTER_STARS_LAYERS][SHOOTER_STARS_COUNT];
	UWORD pOffsets[SHOOTER_STARS_LAYERS];
	ULONG ulLastFrame;
} tShooterStars;

/** Initialize/draw phase zero on a cleared 320px bitmap with >=3 planes.
 * Iterate layer 0..2, then star 0..31 with one continuous 32-bit LCG stream:
 * r=(r*1664525+1013904223) mod 2^32, seed SHOOTER_STARS_SEED.
 * Advance for X=(r>>16)%320, advance again for Y=26+(r>>16)%226.
 * Each star sets only plane=layer; coincident stars combine by bitwise OR.
 * BytesPerRow is ACE's row stride (including depth when interleaved).
 * Caller waits for the blitter before calling; storage/bitmap are borrowed.
 * Keep a separate initialized tShooterStars for each bitmap when buffering.
 */
void shooterHorzStarsInit(tShooterStars *pStars, tBitMap *pBitmap);

/** Render absolute lifetime phase F, independent of gameplay resets/gameover.
 * Offsets for planes 0/1/2 are floor(F/4), floor(F/2), F, each modulo 320.
 * X=(initialX+320-offset)%320; Y never changes. Unchanged layers are skipped.
 * Forward deltas of one or two frames use cached offsets/masks; larger jumps,
 * backwards calls and ULONG rollover use the absolute phase formula above.
 * Erase ALL moved layers' old own-bits before setting ANY new own-bits.
 * No allocation, full-screen clear, other-plane writes or HUD writes.
 * Call after basic header/sprite preparation and blitter completion.
 */
void shooterHorzStarsRender(
	tShooterStars *pStars, tBitMap *pBitmap, ULONG ulLifetimeFrame
);
#endif // _SHOWCASE_EXAMPLES_SHOOTER_HORIZONTAL_STARS_H_
