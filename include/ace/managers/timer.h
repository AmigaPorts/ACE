/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_TIMER_H_
#define _ACE_MANAGERS_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AMIGA
#include <clib/exec_protos.h> // Amiga typedefs
#include <exec/interrupts.h>  // struct Interrupt
#include <hardware/intbits.h> // INTB_VERTB
#endif // AMIGA

#include <ace/types.h>
#include <ace/managers/memory.h>
#include <ace/utils/custom.h>

/* Types */

/**
 * Timer manager structure
 * ulGameTicks should be used by all timers, as it won't update during pause
 * Game pause should be implemented as setting ubPause to 1
 * and still calling timerProcess during pause loop
 */
typedef struct _tTimerManager {
	ULONG ulGameTicks;             /// Actual ticks passed in game
	ULONG ulLastTime;              /// Internal - used to update ulGameTicks
	volatile UWORD uwFrameCounter; /// Incremented by VBlank interrupt
	UBYTE ubPaused;                /// 1: pause on
} tTimerManager;

/* Globals */
extern tTimerManager g_sTimerManager;

/* Functions */

/**
 * Creates Vertical Blank server for counting frames.
 */
void timerCreate(void);

/**
 * Removes Vertical Blank server.
 */
void timerDestroy(void);

/**
 * Gets current time based on frame number
 * One tick equals: PAL - 20ms, NTSC - 16.67ms
 * Max time capacity: 33 months
 */
ULONG timerGet(void);

/**
 * Gets as precise current time as possible
 * Implementation based on ray position and frame number
 * One tick equals: PAL - 0.40us, NTSC - 0.45us
 * Max time capacity: 1715s (28,5 min)
 */
ULONG timerGetPrec(void);

/**
 * Gets time difference between two times
 * For use on both precise and frame time
 */
ULONG timerGetDelta(ULONG ulStart, ULONG ulStop);

/**
 * Returns if timer has passed without updating its state
 */
UBYTE timerPeek(ULONG *pTimer, ULONG ulTimerDelay);

/**
 * Returns if timer has passed
 * If passed, its state gets resetted and countdown starts again
 */
UBYTE timerCheck(ULONG *pTimer,ULONG ulTimerDelay);

/**
 * Updates game ticks if game not paused
 */
void timerProcess(void);

/**
 * Formats precise time to human readable form on supplied buffer
 * Current version works correctly only on ulPrecTime < 0xFFFFFFFF/4 (7 min)
 * and there seems to be no easy fix for this
 */
void timerFormatPrec(char *szBfr, ULONG ulPrecTime);

/**
 *  Min number of us to wait: 5. It is recommended to wait for multiples of 5us.
 */
void timerWaitUs(UWORD uwUsCnt);

void timerOnInterrupt(void);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_TIMER_H_
