#ifndef GUARD_ACE_MANAGER_VIEWPORT_SCROLLBUFFER_H
#define GUARD_ACE_MANAGER_VIEWPORT_SCROLLBUFFER_H

/**
 * Scrollable buffer manager
 * Uses scrolling-trick from aminet to achieve memory-efficient scroll
 * Must be processed as last, because it calls WaitTOF
 * Author: KaiN
 * Requires viewport managers:
 * 	- camera
 * TODO: make it work without tileBuffer manager
 */

#include "ACE:types.h"
#include "ACE:macros.h"
#include "ACE:config.h"
#include "ACE:utils/custom.h"
#include "ACE:utils/extview.h"
#include "ACE:utils/bitmap.h"

#include "ACE:managers/blit.h"
#include "ACE:managers/copper.h"
#include "ACE:managers/viewport/camera.h"

/* Types */

typedef struct {
	tVpManager sCommon;
	tCameraManager *pCameraManager;  /// Quick ref to camera

	struct BitMap *pBuffer;          /// Ptr to buffer bitmap
	tCopBlock *pStartBlock;          /// Initial data fetch
	tCopBlock *pBreakBlock;          /// Bitplane ptr reset
	tUwCoordYX uBfrBounds;           /// Real bounds of buffer (includes height reserved for x-scroll)
	UWORD uwBmAvailHeight;           /// Avail height of buffer to blit (excludes height reserved for x-scroll)
	UWORD uwVpHeightPrev;            /// Prev height of related VPort, used to force refresh on change
	UWORD uwModulo;                  /// Bitplane modulo
	tAvg *pAvg;
} tScrollBufferManager;

/* Globals */

/* Functions */

tScrollBufferManager *scrollBufferCreate(
	IN tVPort *pVPort,
	IN UBYTE ubMarginWidth,
	IN UWORD uwBoundWidth,
	IN UWORD uwBoundHeight
);

void scrollBufferDestroy(
	IN tScrollBufferManager *pManager
);

void scrollBufferProcess(
	IN tScrollBufferManager *pManager
);

void scrollBufferReset(
	IN tScrollBufferManager *pManager,
	IN UBYTE ubMarginWidth,
	IN UWORD uwBoundWidth,
	IN UWORD uwBoundHeight
);

void scrollBufferBlitMask(
	IN tBitMap *pSrc,
	IN WORD wSrcX,
	IN WORD wSrcY,
	IN tScrollBufferManager *pDstManager,
	IN WORD wDstX,
	IN WORD wDstY,
	IN WORD wWidth,
	IN WORD wHeight,
	IN UWORD *pMsk
);

#endif