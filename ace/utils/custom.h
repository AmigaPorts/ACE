#ifndef GUARD_ACE_UTILS_CUSTOM_H
#define GUARD_ACE_UTILS_CUSTOM_H

/**
 * Quick custom chipset struct include
 * Moved to separate file cuz multiple pasting of extern custom is messy
 */

#include <hardware/custom.h> // Custom chip register addresses

// Here was __far attrib from DICE/GCC times. Not needed for VBCC, so it was removed.
extern struct Custom custom;

/**
 * Ray position struct.
 * Merges vposr and vhposr read into one.
 * Setting fields as volatile is mandatory for VBCC 0.9d as setting pointer
 * to volatile struct was insufficient.
 */
typedef struct {
	volatile unsigned ubLaced:1;   ///< 1 for interlaced screens
	volatile unsigned uwUnused:14;
	volatile unsigned uwPosY:9;    ///< PAL: 0..312, NTSC: 0..?
	volatile unsigned ubPosX:8;    ///< 0..159?
} tRayPos;

extern volatile tRayPos * const vhPosRegs;

typedef struct {
	UWORD uwHi; ///< upper WORD
	UWORD uwLo; ///< lower WORD
} tCopperUlong;

/**
 * Bitplane display regs with 16-bit access.
 * For use with Copper. Other stuff should use custom.bplpt
 */
extern volatile tCopperUlong * const pBplPtrs;
extern volatile tCopperUlong * const pCopLc;

#endif
