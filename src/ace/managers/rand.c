/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Based on RNG from http://b2d-f9r.blogspot.com/2010/08/16-bit-xorshift-rng-now-with-more.html
// The other considered alternative was xoroshiro64/32 (https://prng.di.unimi.it),
// but it contains multiplications which are way slower than adds and shifts.

#include <ace/managers/rand.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>

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
	memFree(pRand, sizeof(*pRand));
}

void randInit(tRandManager *pRand, UWORD uwSeed1, UWORD uwSeed2) {
	logBlockBegin(
		"randInit(pRand: %p, uwSeed1: %hu, uwSeed2: %hu)", pRand, uwSeed1, uwSeed2
	);
	if(uwSeed1 == 0 || uwSeed2 == 0) {
		logWrite("ERR: Seeds can't be zero\n");
		logBlockEnd("randInit()");
		return;
	}

	pRand->uwState1 = uwSeed1;
	pRand->uwState2 = uwSeed2;
	logBlockEnd("randInit()");
}

UWORD randUw(tRandManager *pRand) {
  UWORD t = (pRand->uwState1 ^ (pRand->uwState1 << RAND_COEFF_A));
  pRand->uwState1 = pRand->uwState2;
	pRand->uwState2 = (pRand->uwState2 ^ (pRand->uwState2 >> RAND_COEFF_C)) ^ (t ^ (t >> RAND_COEFF_B));
  return pRand->uwState2;
}

UWORD randUwMax(tRandManager *pRand, UWORD uwMax) {
	return randUw(pRand) % (uwMax + 1);
}

UWORD randUwMinMax(tRandManager *pRand, UWORD uwMin, UWORD uwMax) {
	return uwMin + randUwMax(pRand, uwMax - uwMin);
}

ULONG randUl(tRandManager *pRand) {
	UWORD uwUpper = randUw(pRand);
	UWORD uwLower = randUw(pRand);
	return (uwUpper << 16) | (uwLower);
}

ULONG randUlMax(tRandManager *pRand, ULONG ulMax) {
	return randUl(pRand) % (ulMax + 1);
}

ULONG randUlMinMax(tRandManager *pRand, ULONG ulMin, ULONG ulMax) {
	return ulMin + randUlMax(pRand, ulMax - ulMin);
}

//---------------------------------------------------------------------- GLOBALS
