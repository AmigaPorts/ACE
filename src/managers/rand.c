#include "managers/rand.h"

/* Globals */
tRandManager g_sRandManager;

/* Functions */
void randInit(ULONG ulSeed) {
	g_sRandManager.ulRandX = 123456789 + ulSeed;
	g_sRandManager.ulRandY = 362436069 + ulSeed;
	g_sRandManager.ulRandZ = 521288629 + ulSeed;
	g_sRandManager.uwRandX = g_sRandManager.ulRandX & 0xFFFF;
	g_sRandManager.uwRandY = g_sRandManager.ulRandY & 0xFFFF;
	g_sRandManager.uwRandZ = g_sRandManager.ulRandZ & 0xFFFF;
	g_sRandManager.uwRandW = (88675123 + ulSeed) & 0xFFFF;
}

 // ULONG is slower on Amiga, use UWORD instead
ULONG ulRand(void) {
	ULONG ulRandT;
	g_sRandManager.ulRandX ^= g_sRandManager.ulRandX << 16;
	g_sRandManager.ulRandX ^= g_sRandManager.ulRandX >> 5;
	g_sRandManager.ulRandX ^= g_sRandManager.ulRandX << 1;

	ulRandT = g_sRandManager.ulRandX;
	g_sRandManager.ulRandX = g_sRandManager.ulRandY;
	g_sRandManager.ulRandY = g_sRandManager.ulRandZ;
	g_sRandManager.ulRandZ = ulRandT ^ g_sRandManager.ulRandX ^ g_sRandManager.ulRandY;

	return g_sRandManager.ulRandZ;
}

// UWORD is faster on Amiga
UBYTE ubRand() {
	return uwRand() ^ 0xFF;
}

UBYTE ubRandMax(UBYTE ubMax) {
	return ubRand() % (ubMax + 1);
}

UBYTE ubRandMinMax(UBYTE ubMin, UBYTE ubMax) {
	return  ubMin + ubRandMax(ubMax - ubMin);
}

UWORD uwRand(void) {
	UWORD uwT = g_sRandManager.uwRandX ^ (g_sRandManager.uwRandX << 11);
	g_sRandManager.uwRandX = g_sRandManager.uwRandY;
	g_sRandManager.uwRandY = g_sRandManager.uwRandZ; 
	g_sRandManager.uwRandZ = g_sRandManager.uwRandW;
	return g_sRandManager.uwRandW = (g_sRandManager.uwRandW ^ (g_sRandManager.uwRandW >> 19)) ^ (uwT ^ (uwT >> 8));
}

UWORD uwRandMax(UWORD uwMax) {
	return uwRand() % (uwMax + 1);
}

UWORD uwRandMinMax(UWORD uwMin, UWORD uwMax) {
	return uwMin + uwRandMax(uwMax - uwMin);
}

ULONG ulRandMax(ULONG ulMax) {
	return ulRand() % (ulMax + 1);
}

ULONG ulRandMinMax(ULONG ulMin, ULONG ulMax) {
	return ulMin + ulRandMax(ulMax - ulMin);
}