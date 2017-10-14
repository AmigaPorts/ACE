#ifndef GUARD_ACE_UTIL_BOBFRAMES_H
#define GUARD_ACE_UTIL_BOBFRAMES_H

#include <ace/types.h>
#include <ace/managers/log.h>
#include <ace/managers/memory.h>
#include <ace/utils/bitmap.h>

#define BOBFRAMES_BPP 5

typedef struct {
	tBitMap *pBitMap;
	UWORD *pMask;
} tBobFrame;

typedef struct {
	UBYTE ubFrameWidth;
	UBYTE ubFrameHeight;
	UBYTE ubAnimCount;
	tBobFrame ***pData;
} tBobFrameset;

#define BOB_DIRS   4
#define BOB_DIR_N  0
#define BOB_DIR_E  1
#define BOB_DIR_S  2
#define BOB_DIR_W  3

tBobFrameset *bobFramesCreate(
	IN char *szFileName
);

void bobFramesDestroy(
	IN tBobFrameset *pFrameset
);


#endif
