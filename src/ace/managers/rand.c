/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Based on RNG from http://b2d-f9r.blogspot.com/2010/08/16-bit-xorshift-rng-now-with-more.html
// The other considered alternative was xoroshiro64/32 (https://prng.di.unimi.it),
// but it contains multiplications which are way slower than adds and shifts.

#include <ace/managers/rand.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/utils/assume.h>

// Coefficients are chosen from the table of original post, with restriction
// to use shifts lesser than 8 in order to let compiler use "shift immediate"
// asm instructions.
// According to comments on said page, (5,3,1) is busted so use (5,7,4) instead.
// Other available tuple is (6,3,8).
#define RAND_COEFF_A 5
#define RAND_COEFF_B 7
#define RAND_COEFF_C 4

//-------------------------------------------------------------------- FUNCTIONS

tRandManager *randCreate(UWORD uwSeed1, UWORD uwSeed2) {
	tRandManager *pRand = memAllocFast(sizeof(*pRand));
	randInit(pRand, uwSeed1, uwSeed2);
	return pRand;
}

void randDestroy(tRandManager *pRand) {
	assumeNotNull(pRand);
	memFree(pRand, sizeof(*pRand));
}

void randInit(tRandManager *pRand, UWORD uwSeed1, UWORD uwSeed2) {
	logBlockBegin(
		"randInit(pRand: %p, uwSeed1: %hu, uwSeed2: %hu)", pRand, uwSeed1, uwSeed2
	);
	assumeNotNull(pRand);
	assumeMsg(uwSeed1 != 0, "Seeds can't be zero");
	assumeMsg(uwSeed2 != 0, "Seeds can't be zero");

	pRand->uwState1 = uwSeed1;
	pRand->uwState2 = uwSeed2;
	logBlockEnd("randInit()");
}

UWORD randUw(tRandManager *pRand) {
	assumeNotNull(pRand);

  UWORD t = (pRand->uwState1 ^ (pRand->uwState1 << RAND_COEFF_A));
  pRand->uwState1 = pRand->uwState2;
	pRand->uwState2 = (pRand->uwState2 ^ (pRand->uwState2 >> RAND_COEFF_C)) ^ (t ^ (t >> RAND_COEFF_B));
  return pRand->uwState2;
}

UWORD randUwMax(tRandManager *pRand, UWORD uwMax) {
	assumeNotNull(pRand);

	return randUw(pRand) % (uwMax + 1);
}

UWORD randUwMinMax(tRandManager *pRand, UWORD uwMin, UWORD uwMax) {
	assumeNotNull(pRand);

	return uwMin + randUwMax(pRand, uwMax - uwMin);
}

ULONG randUl(tRandManager *pRand) {
	assumeNotNull(pRand);

	UWORD uwUpper = randUw(pRand);
	UWORD uwLower = randUw(pRand);
	return (uwUpper << 16) | (uwLower);
}

ULONG randUlMax(tRandManager *pRand, ULONG ulMax) {
	assumeNotNull(pRand);

	return randUl(pRand) % (ulMax + 1);
}

ULONG randUlMinMax(tRandManager *pRand, ULONG ulMin, ULONG ulMax) {
	assumeNotNull(pRand);

	return ulMin + randUlMax(pRand, ulMax - ulMin);
}

//---------------------------------------------------------------------- GLOBALS
