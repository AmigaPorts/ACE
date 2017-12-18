#include <ace/utils/custom.h>

#ifdef AMIGA

volatile tRayPos * const vhPosRegs = (APTR)&custom.vposr;

volatile tCopperUlong * const pBplPtrs = (APTR)&custom.bplpt;
volatile tCopperUlong * const pSprPtrs = (APTR)&custom.sprpt;
volatile tCopperUlong * const pCopLc = (APTR)&custom.cop1lc;

volatile tCia * const g_pCiaA = (tCia*)0x0bfe001;
volatile tCia * const g_pCiaB = (tCia*)0x0bfd000;

#endif // AMIGA
