#ifndef GUARD_ACE_UTIL_BITMAPMASK_H
#define GUARD_ACE_UTIL_BITMAPMASK_H

#include <ace/config.h>

typedef struct _tBitmapMask {
	UWORD uwWidth;
	UWORD uwHeight;
	UWORD *pData;
} tBitmapMask;

tBitmapMask *bitmapMaskCreate(
	IN UWORD uwWidth,
	IN UWORD uwHeight
);

tBitmapMask *bitmapMaskCreateFromFile(
	IN char *szFile
);

void bitmapMaskDestroy(
	IN tBitmapMask *pMask
);

/**
 *  @brief Writes mask as 1bpp bitmap.
 *  Only for debug purposes, because it's as unoptimized as bitmapSaveBmp().
 *  
 *  @see bitmapSaveBmp()
 */
void bitmapMaskSaveBmp(
	IN tBitmapMask *pMask,
	IN char *szPath
);

#endif // GUARD_ACE_UTIL_BITMAPMASK_H
