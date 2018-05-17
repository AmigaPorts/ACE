/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_CUSTOM_H_
#define _ACE_UTILS_CUSTOM_H_

#include <ace/macros.h>

#ifdef AMIGA

#include <hardware/custom.h> // Custom chip register addresses

#define REGPTR volatile * const

typedef struct Custom tCustom;

/**
 * Ray position struct.
 * Merges vposr and vhposr read into one.
 * Setting fields as volatile is mandatory for VBCC 0.9d as setting pointer
 * to volatile struct was insufficient.
 */
typedef struct {
	volatile unsigned bfLaced:1;   ///< 1 for interlaced screens
	volatile unsigned bfUnused:14;
	volatile unsigned bfPosY:9;    ///< PAL: 0..312, NTSC: 0..?
	volatile unsigned bfPosX:8;    ///< 0..159?
} tRayPos;


typedef struct {
	UWORD uwHi; ///< upper WORD
	UWORD uwLo; ///< lower WORD
} tCopperUlong;

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
#define CIAAPRA_OVL  BV(0)
#define CIAAPRA_LED  BV(1)
#define CIAAPRA_CHNG BV(2)
#define CIAAPRA_WPRO BV(3)
#define CIAAPRA_TK0  BV(4)
#define CIAAPRA_RDY  BV(5)
#define CIAAPRA_FIR0 BV(6)
#define CIAAPRA_FIR1 BV(7)

#define CIABPRB_STEP BV(0)
#define CIABPRB_DIR  BV(1)
#define CIABPRB_SIDE BV(2)
#define CIABPRB_SEL0 BV(3)
#define CIABPRB_SEL1 BV(4)
#define CIABPRB_SEL2 BV(5)
#define CIABPRB_SEL3 BV(6)
#define CIABPRB_MTR  BV(7)

#define CIAICR_TIMER_A BV(0)
#define CIAICR_TIMER_B BV(1)
#define CIAICR_TOD     BV(2)
#define CIAICR_SERIAL  BV(3)
#define CIAICR_FLAG    BV(4)
#define CIAICR_SETCLR  BV(7)

#define CIACRA_START   BV(0)
#define CIACRA_PBON    BV(1)
#define CIACRA_OUTMODE BV(2)
#define CIACRA_RUNMODE BV(3)
#define CIACRA_LOAD    BV(4)
#define CIACRA_INMODE  BV(5)
#define CIACRA_SPMODE  BV(6)

#define CIACRB_START   BV(0)
#define CIACRB_PBON    BV(1)
#define CIACRB_OUTMODE BV(2)
#define CIACRB_RUNMODE BV(3)
#define CIACRB_LOAD    BV(4)
#define CIACRB_INMODE  (BV(5) | BV(6))
#define CIACRB_ALARM   BV(7)

/**
 * @brief Gets consistent Timer A value from given CIA chip.
 * Based on https://github.com/keirf/HxC_FF_File_Selector/blob/master/amiga/amiga.c
 * @param pCia Base CIA chip address.
 */
UWORD ciaGetTimerA(tCia REGPTR pCia);

/**
 * @brief Gets consistent Timer A value from given CIA chip.
 * Based on https://github.com/keirf/HxC_FF_File_Selector/blob/master/amiga/amiga.c
 * @param pCia Base CIA chip address.
 */
UWORD ciaGetTimerB(tCia REGPTR pCia);

extern tCustom FAR REGPTR g_pCustom;

extern tRayPos FAR REGPTR g_pRayPos;

/**
 * Bitplane display regs with 16-bit access.
 * For use with Copper. Other stuff should use g_pCustom->bplpt
 */
extern tCopperUlong FAR REGPTR g_pSprFetch;
extern tCopperUlong FAR REGPTR g_pBplFetch;
extern tCopperUlong FAR REGPTR g_pCopLc;

extern tCia FAR REGPTR g_pCiaA;
extern tCia FAR REGPTR g_pCiaB;

#endif // AMIGA
#endif // _ACE_UTILS_CUSTOM_H_
