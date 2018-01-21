#include <ace/utils/custom.h>
#include <stddef.h>

#ifdef AMIGA

#define CUSTOM_BASE 0xDFF000

tCustom FAR volatile * const g_pCustom = (tCustom volatile * const)CUSTOM_BASE;

tRayPos FAR volatile * const vhPosRegs = (tRayPos volatile * const)(CUSTOM_BASE + offsetof(tCustom, vhposr));

tCopperUlong FAR volatile * const pBplPtrs = (tCopperUlong volatile * const)(CUSTOM_BASE + offsetof(tCustom, bplpt));
tCopperUlong FAR volatile * const pSprPtrs = (tCopperUlong volatile * const)(CUSTOM_BASE + offsetof(tCustom, sprpt));
tCopperUlong FAR volatile * const pCopLc = (tCopperUlong volatile * const)(CUSTOM_BASE + offsetof(tCustom, cop1lc));

tCia FAR volatile * const g_pCiaA = (tCia*)0xBFE001;
tCia FAR volatile * const g_pCiaB = (tCia*)0xBFD000;

#endif // AMIGA
