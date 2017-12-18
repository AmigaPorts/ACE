#ifndef GUARD_ACE_UTILS_CUSTOM_H
#define GUARD_ACE_UTILS_CUSTOM_H

#ifdef AMIGA

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
extern volatile tCopperUlong * const pSprPtrs;
extern volatile tCopperUlong * const pBplPtrs;
extern volatile tCopperUlong * const pCopLc;

/**
 * CIA registers.
 * Borrowed from https://github.com/keirf/Amiga-Stuff
 */
typedef struct _tCia {
	volatile UBYTE pra;
	volatile UBYTE _0[0xff];
	volatile UBYTE prb;
	volatile UBYTE _1[0xff];
	volatile UBYTE ddra;
	volatile UBYTE _2[0xff];
	volatile UBYTE ddrb;
	volatile UBYTE _3[0xff];
	volatile UBYTE talo;
	volatile UBYTE _4[0xff];
	volatile UBYTE tahi;
	volatile UBYTE _5[0xff];
	volatile UBYTE tblo;
	volatile UBYTE _6[0xff];
	volatile UBYTE tbhi;
	volatile UBYTE _7[0xff];
	volatile UBYTE todlow;
	volatile UBYTE _8[0xff];
	volatile UBYTE todmid;
	volatile UBYTE _9[0xff];
	volatile UBYTE todhi;
	volatile UBYTE _a[0xff];
	volatile UBYTE b00;
	volatile UBYTE _b[0xff];
	volatile UBYTE sdr;
	volatile UBYTE _c[0xff];
	volatile UBYTE icr;
	volatile UBYTE _d[0xff];
	volatile UBYTE cra;
	volatile UBYTE _e[0xff];
	volatile UBYTE crb;
	volatile UBYTE _f[0xff];
} tCia;

/**
 * CIA defines.
 */
#define CIAAPRA_OVL  0x01
#define CIAAPRA_LED  0x02
#define CIAAPRA_CHNG 0x04
#define CIAAPRA_WPRO 0x08
#define CIAAPRA_TK0  0x10
#define CIAAPRA_RDY  0x20
#define CIAAPRA_FIR0 0x40
#define CIAAPRA_FIR1 0x80

#define CIABPRB_STEP 0x01
#define CIABPRB_DIR  0x02
#define CIABPRB_SIDE 0x04
#define CIABPRB_SEL0 0x08
#define CIABPRB_SEL1 0x10
#define CIABPRB_SEL2 0x20
#define CIABPRB_SEL3 0x40
#define CIABPRB_MTR  0x80

#define CIAICR_TIMER_A 0x01
#define CIAICR_TIMER_B 0x02
#define CIAICR_TOD     0x04
#define CIAICR_SERIAL  0x08
#define CIAICR_FLAG    0x10
#define CIAICR_SETCLR  0x80

#define CIACRA_START   0x01
#define CIACRA_PBON    0x02
#define CIACRA_OUTMODE 0x04
#define CIACRA_RUNMODE 0x08
#define CIACRA_LOAD    0x10
#define CIACRA_INMODE  0x20
#define CIACRA_SPMODE  0x40

#define CIACRB_START   0x01
#define CIACRB_PBON    0x02
#define CIACRB_OUTMODE 0x04
#define CIACRB_RUNMODE 0x08
#define CIACRB_LOAD    0x10
#define CIACRB_INMODE  0x60
#define CIACRB_ALARM   0x80

extern volatile tCia * const g_pCiaA;
extern volatile tCia * const g_pCiaB;

#endif // AMIGA
#endif // GUARD_ACE_UTILS_CUSTOM_H
