#include <ace/utils/custom.h>

#ifdef AMIGA

volatile tRayPos * const vhPosRegs = (APTR)&custom.vposr;

volatile tCopperUlong * const pBplPtrs = (APTR)&custom.bplpt;
volatile tCopperUlong * const pSprPtrs = (APTR)&custom.sprpt;
volatile tCopperUlong * const pCopLc = (APTR)&custom.cop1lc;

#endif // AMIGA
