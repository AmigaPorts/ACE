/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_RAND_H_
#define _ACE_MANAGERS_RAND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>

//---------------------------------------------------------------------- DEFINES

//------------------------------------------------------------------------ TYPES

typedef struct _tRandManager {
	ULONG ulRandX;
	ULONG ulRandY;
	ULONG ulRandZ;
	UWORD uwRandX;
	UWORD uwRandY;
	UWORD uwRandZ;
	UWORD uwRandW;
} tRandManager;

//-------------------------------------------------------------------- FUNCTIONS

/**
 * @brief Initializes random number generator with given seed value.
 * You must call it at least once before using other rand fns.
 *
 * @param ulSeed New seed value for RNG.
 */
void randInit(ULONG ulSeed);

/**
 * @brief Returns the random number value from range of all 16-bit values.
 *
 * @return Next random value.
 */
UWORD uwRand(void);

/**
 * @brief Returns the random value from 0 up to uwMax, including uwMax.
 *
 * @param uwMax Upper bound of the random value.
 * @return Next random value constrained in specified boundaries.
 */
UWORD uwRandMax(UWORD uwMax);

/**
 * @brief Returns the random value between uwMin and uwMax, including uwMin and uwMax.
 *
 * @param uwMin Lower bound of the random value.
 * @param uwMax  Upper bound of the random value.
 * @return Next random value constrained in specified boundaries.
 */
UWORD uwRandMinMax(UWORD uwMin, UWORD uwMax);

ULONG ulRand(void); // ULONG is slower on Amiga, use UWORD instead

ULONG ulRandMax(ULONG ulMax);

ULONG ulRandMinMax(ULONG ulMin, ULONG ulMax);

//---------------------------------------------------------------------- GLOBALS

extern tRandManager g_sRandManager;

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_RAND_H_
