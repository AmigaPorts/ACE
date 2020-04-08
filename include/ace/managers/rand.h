/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_RAND_H_
#define _ACE_MANAGERS_RAND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>

/* Types */
typedef struct _tRandManager {
	ULONG ulRandX;
	ULONG ulRandY;
	ULONG ulRandZ;
	UWORD uwRandX;
	UWORD uwRandY;
	UWORD uwRandZ;
	UWORD uwRandW;
} tRandManager;

/* Globals */
extern tRandManager g_sRandManager;

/* Functions */
void randInit(ULONG ulSeed);

UBYTE ubRand(void);

UBYTE ubRandMax(UBYTE ubMax);

UBYTE ubRandMinMax(UBYTE ubMin, UBYTE ubMax);

UWORD uwRand(void); // UWORD is faster on Amiga

UWORD uwRandMax(UWORD uwMax);

UWORD uwRandMinMax(UWORD uwMin, UWORD uwMax);

ULONG ulRand(void); // ULONG is slower on Amiga, use UWORD instead

ULONG ulRandMax(ULONG ulMax);

ULONG ulRandMinMax(ULONG ulMin, ULONG ulMax);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_RAND_H_
