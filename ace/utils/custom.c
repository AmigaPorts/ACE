#include <ace/utils/custom.h>

volatile tRayPos * const vhPosRegs = (APTR)&custom.vposr;

volatile tCopperUlong * const pBplPtrs = (APTR)&custom.bplpt;
volatile tCopperUlong * const pCopLc = (APTR)&custom.cop1lc;
