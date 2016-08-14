#ifndef GUARD_ACE_UTIL_PLANAR_H
#define GUARD_ACE_UTIL_PLANAR_H

#include <ace/config.h>
#include <ace/utils/bitmap.h>

void planarRead16(
	IN tBitMap *pBitMap,
	IN UWORD uwX,
	IN UWORD uwY,
	OUT UBYTE *pOut
);

UBYTE planarRead(
	IN tBitMap *pBitMap,
	IN UWORD uwX,
	IN UWORD uwY
);

#endif