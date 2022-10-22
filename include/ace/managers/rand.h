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
	UWORD uwState1;
	UWORD uwState2;
} tRandManager;

//-------------------------------------------------------------------- FUNCTIONS

/**
 * @brief Initializes random number generator with given seed value.
 * You must call it at least once before using other rand fns.
 *
 * Both seed values must be non-zero!
 *
 * @param pRand Instance of RNG to use.
 * @param uwSeed1 First part of seed value for RNG.
 * @param uwSeed2 Second part of seed value for RNG.
 * @see randCreate()
 */
void randInit(tRandManager *pRand, UWORD uwSeed1, UWORD uwSeed2);

/**
 * @brief Allocates and initializes new random number generator.
 *
 * @param uwSeed1 First part of seed value for RNG.
 * @param uwSeed2 Second part of seed value for RNG.
 * @return New instance of random number generator.
 * @see randInit()
 * @see randDestroy()
 */
tRandManager *randCreate(UWORD uwSeed1, UWORD uwSeed2);

/**
 * @brief Destroys previously allocated instance of random number generator.
 *
 * @param pRand Instance of random number generator to destroy.
 * @see randCreate()
 */
void randDestroy(tRandManager *pRand);

/**
 * @brief Returns the random number value from range of all 16-bit values.
 *
 * @param pRand Instance of RNG to use.
 * @return Next random value.
 */
UWORD randUw(tRandManager *pRand);

/**
 * @brief Returns the random value from 0 up to uwMax, including uwMax.
 *
 * @param pRand Instance of RNG to use.
 * @param uwMax Upper bound of the random value.
 * @return Next random value constrained in specified boundaries.
 */
UWORD randUwMax(tRandManager *pRand, UWORD uwMax);

/**
 * @brief Returns the random value between uwMin and uwMax, including uwMin and uwMax.
 *
 * @param pRand Instance of RNG to use.
 * @param uwMin Lower bound of the random value.
 * @param uwMax  Upper bound of the random value.
 * @return Next random value constrained in specified boundaries.
 */
UWORD randUwMinMax(tRandManager *pRand, UWORD uwMin, UWORD uwMax);

/**
 * @brief Returns the random 32-bit number.
 *
 * Operating on 32-bit values on OCS is considerably slower than 16-bit,
 * so use 16-bit randomness where possible.
 *
 * Internally, this function merges two 16-bit random numbers into a 32-bit one,
 * so quality of this RNG is quite bad.
 *
 * @param pRand Instance of RNG to use.
 * @return Next random 32-bit value.
 * @see randUw()
 */
ULONG randUl(tRandManager *pRand);

ULONG randUlMax(tRandManager *pRand, ULONG ulMax);

ULONG randUlMinMax(tRandManager *pRand, ULONG ulMin, ULONG ulMax);

//---------------------------------------------------------------------- GLOBALS

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_RAND_H_
