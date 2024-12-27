/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MACROS_H_
#define _ACE_MACROS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>

/**
 * ACE Macros
 * General purpose macros used throughout all code
 * Insert all somewhat universal macros here even if used only by single module
 * so future expansion will keep macro dependencies at bay
 */

/**
 * Block count - determine how many blocks of given size are in given length
 * blockCountFloor - counts only full blocks
 * blockCountCeil - counts also non-filled block
 */
#define blockCountFloor(length, blockSize) (length/blockSize)
#define blockCountCeil(length, blockSize) ((length + blockSize-1)/blockSize)

// Rotate fns - should be optimized away by compiler to single instruction
// Based on https://stackoverflow.com/questions/776508/best-practices-for-circular-shift-rotate-operations-in-c

static inline UWORD rol16(UWORD uwIn, UBYTE ubRot) {
  const UWORD uwMask = ((sizeof(uwIn)<<3) - 1);
  ubRot &= uwMask;
  return (uwIn << ubRot) | (uwIn >> ((-ubRot) & uwMask));
}

static inline UWORD ror16(UWORD uwIn, UBYTE ubRot) {
  const UWORD uwMask = ((sizeof(uwIn)<<3) - 1);
  ubRot &= uwMask;
  return (uwIn >> ubRot) | (uwIn << ((-ubRot) & uwMask));
}

static inline ULONG rol32(ULONG ulIn, UBYTE ubRot) {
  const ULONG ulMask = ((sizeof(ulIn)<<3) - 1);
  ubRot &= ulMask;
  return (ulIn << ubRot) | (ulIn >> ((-ubRot) & ulMask));
}

static inline ULONG ror32(ULONG ulIn, UBYTE ubRot) {
  const ULONG ulMask = ((sizeof(ulIn)<<3) - 1);
  ubRot &= ulMask;
  return (ulIn >> ubRot) | (ulIn << ((-ubRot) & ulMask));
}

/**
 * @brief Swaps contents of two vars.
 */
#define SWAP(a, b) do {typeof(a) tmp; tmp = a; a = b; b = tmp;} while(0)

// Math
#define ABS(x) ((x)<0 ? -(x) : (x))
#define SGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
// #define SGN(x) ((x > 0) - (x < 0)) // Branchless, subtracting is slower?
#define MIN(x,y) ((x)<(y)? (x): (y))
#define MAX(x,y) ((x)>(y)? (x): (y))
#define CLAMP(x, min, max) ((x) < (min)? (min) : ((x) > (max) ? (max) : (x)))
#define CEIL_TO_FACTOR(x, m) ((((x) + (m) - 1) / (m)) * (m))
#define FLOOR_TO_FACTOR(x, m) (((x) / (m)) * (m))
#define ROUND_TO_FACTOR(x, m) ((((x) + (x) / 2) / (m)) * (m))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * Bit value macro - useful for setting & testing bits.
 */
#define BV(x) (1 << (x))
#define BTST(x, b) (((x) & BV(b)) != 0)

/**
 *  Checks if given x,y is in specified tRect.
 */
#define inRect(x, y, r) (                     \
	(x) >= r.uwX && (x) <= r.uwX + r.uwWidth && \
	(y) >= r.uwY && (y) <= r.uwY+r.uwHeight     \
)

#define inAbsRect(x, y, ar) (          \
	(x) >= ar.uwX1 && (x) <= ar.uwX2  && \
	(y) >= ar.uwY1 && (y) <= ar.uwY2     \
)

#ifdef __cplusplus
}
#endif

#endif // _ACE_MACROS_H_
