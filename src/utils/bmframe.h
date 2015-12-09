#ifndef GUARD_ACE_UTIL_BMFRAME_H
#define GUARD_ACE_UTIL_BMFRAME_H

#include "utils/bitmap.h"
#include "managers/blit.h"

// defines for offsets of tiles in frame file

#define FRAMETILE_NW 0,0
#define FRAMETILE_N 16,0
#define FRAMETILE_NE 32,0

#define FRAMETILE_W 0,16
#define FRAMETILE_C 16,16
#define FRAMETILE_E 32,16

#define FRAMETILE_SW 0,32
#define FRAMETILE_S 16,32
#define FRAMETILE_SE 32,32

void bmFrameDraw(
	IN struct BitMap *pFrameSet,
	IN struct BitMap *pDest,
	IN UWORD uwX,
	IN UWORD uwY,
	IN UBYTE ubTileWidth,
	IN UBYTE ubTileHeight
);

#endif