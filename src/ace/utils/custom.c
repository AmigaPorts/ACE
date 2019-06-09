/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/custom.h>
#include <stddef.h>

#ifdef AMIGA

#define CUSTOM_BASE 0xDFF000

tCustom FAR REGPTR g_pCustom = (tCustom REGPTR)CUSTOM_BASE;

tRayPos FAR REGPTR g_pRayPos = (tRayPos REGPTR)(
	CUSTOM_BASE + offsetof(tCustom, vposr)
);

tCopperUlong FAR REGPTR g_pBplFetch = (tCopperUlong REGPTR)(
	CUSTOM_BASE + offsetof(tCustom, bplpt)
);
tCopperUlong FAR REGPTR g_pSprFetch = (tCopperUlong REGPTR)(
	CUSTOM_BASE + offsetof(tCustom, sprpt)
);
tCopperUlong FAR REGPTR g_pCopLc = (tCopperUlong REGPTR)(
	CUSTOM_BASE + offsetof(tCustom, cop1lc)
);

tCia FAR REGPTR g_pCia[CIA_COUNT] = {(tCia*)0xBFE001, (tCia*)0xBFD000};

UWORD ciaGetTimerA(tCia REGPTR pCia) {
	UBYTE ubHi, ubLo;
	do {
		ubHi = pCia->tahi;
		ubLo = pCia->talo;
	} while(ubHi != pCia->tahi);
	return (ubHi << 8) | ubLo;
}

UWORD ciaGetTimerB(tCia REGPTR pCia) {
	UBYTE ubHi, ubLo;
	do {
		ubHi = pCia->tbhi;
		ubLo = pCia->tblo;
	} while(ubHi != pCia->tbhi);
	return (ubHi << 8) | ubLo;
}

#endif // AMIGA
