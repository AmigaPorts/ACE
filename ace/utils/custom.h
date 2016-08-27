#ifndef GUARD_ACE_UTILS_CUSTOM_H
#define GUARD_ACE_UTILS_CUSTOM_H

/**
 * Quick custom chipset struct include
 * Moved to separate file cuz multiple pasting of extern custom is messy
 */

#include <hardware/custom.h> // Custom chip register addresses

__far extern struct Custom custom;

/**
 * Ray position struct.
 * Merges vposr and vhposr read into one.
 */
typedef struct {
	unsigned ubLaced:1;   ///< 1 for interlaced screens
	unsigned uwUnused:14;
	unsigned uwPosY:9;    ///< PAL: 0..312, NTSC: 0..?
	unsigned ubPosX:8;    ///< 0..159?
} tRayPos;

extern tRayPos * const vhPosRegs;

/**
 * Bitplane display regs with 16-bit access.
 * For use with Copper. Other stuff should use custom.bplpt
 */
typedef struct {
	UWORD uwHi; ///< upper WORD of bitplane address
	UWORD uwLo; ///< lower WORD of bitplane address
} tBitplanePtr;

extern tBitplanePtr * const pBplPtrs;

#endif