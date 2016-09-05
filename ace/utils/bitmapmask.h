#ifndef GUARD_ACE_UTIL_BITMAPMASK_H
#define GUARD_ACE_UTIL_BITMAPMASK_H

#include <ace/config.h>

typedef struct {
	UWORD uwWidth;
	UWORD uwHeight;
	UWORD *pData;
} tBitmapMask;

tBitmapMask *bitmapMaskCreate(
	IN char *szFile
);

void bitmapMaskDestroy(
	IN tBitmapMask *pMask
);

#endif // GUARD_ACE_UTIL_BITMAPMASK_H
