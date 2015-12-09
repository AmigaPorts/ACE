#ifndef GUARD_ACE_MANAGER_RAND_H
#define GUARD_ACE_MANAGER_RAND_H

#include <clib/exec_protos.h> // Amiga typedefs

#include "config.h"

/* Types */
typedef struct {
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
void randInit(
	IN ULONG ulSeed
);

UBYTE ubRand(void);

UBYTE ubRandMax(
	IN UBYTE ubMax
);

UBYTE ubRandMinMax(
	IN UBYTE ubMin,
	IN UBYTE ubMax
);

UWORD uwRand(void); // UWORD is faster on Amiga

UWORD uwRandMax(
	IN UWORD uwMax
);

UWORD uwRandMinMax(
	IN UWORD uwMin,
	IN UWORD uwMax
);

ULONG ulRand(void); // ULONG is slower on Amiga, use UWORD instead

ULONG ulRandMax(
	IN ULONG ulMax
);

ULONG ulRandMinMax(
	IN ULONG ulMin,
	IN ULONG ulMax
);

#endif