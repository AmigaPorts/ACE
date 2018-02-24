#include <ace/utils/custom.h>
#include <stddef.h>

#ifdef AMIGA

#define CUSTOM_BASE 0xDFF000

tCustom FAR REGPTR g_pCustom = (tCustom REGPTR)CUSTOM_BASE;

tRayPos FAR REGPTR g_pRayPos = (tRayPos REGPTR)(CUSTOM_BASE + offsetof(tCustom, vposr));

tCopperUlong FAR REGPTR g_pBplFetch = (tCopperUlong REGPTR)(CUSTOM_BASE + offsetof(tCustom, bplpt));
tCopperUlong FAR REGPTR g_pSprFetch = (tCopperUlong REGPTR)(CUSTOM_BASE + offsetof(tCustom, sprpt));
tCopperUlong FAR REGPTR g_pCopLc = (tCopperUlong REGPTR)(CUSTOM_BASE + offsetof(tCustom, cop1lc));

tCia FAR REGPTR g_pCiaA = (tCia*)0xBFE001;
tCia FAR REGPTR g_pCiaB = (tCia*)0xBFD000;

#endif // AMIGA
