#ifndef GUARD_ACE_UTIL_BITMAP_H
#define GUARD_ACE_UTIL_BITMAP_H

/**
 * Bitmap drawing functions
 *
 * TODO: fast fn for blitting tiles
 * TODO: bitmapBlitMask: interleaved support
 * TODO: disable checks when DEBUG is not set
 */

#include <stdio.h> // FILE etc

#include <clib/exec_protos.h> // Amiga typedefs
#include <clib/graphics_protos.h> // BitMap etc

#include "ACE:config.h"
#include "ACE:managers/log.h"
#include "ACE:managers/memory.h"
#include "ACE:utils/custom.h"

/* Types */
typedef struct BitMap tBitMap;

/* Globals */

/* Functions */

tBitMap* bitmapCreate(
	IN UWORD uwWidth,
	IN UWORD uwHeight,
	IN UBYTE ubDepth,
	IN UBYTE ubFlags
);

tBitMap* bitmapCreateFromFile(
	IN char *szFileName
);

void bitmapDestroy(
	IN tBitMap *pBitMap
);

inline BYTE bitmapIsInterleaved(
	IN tBitMap *pBitMap
);

#endif