#ifndef GUARD_ACE_MANAGER_VIEWPORT_SIMPLEBUFFER_H
#define GUARD_ACE_MANAGER_VIEWPORT_SIMPLEBUFFER_H

// Implementacja bufora ekranu zgodnego z API AmigaOS
// korzysta ze scrollingu przez RasInfo

#include "ACE:types.h"
#include "ACE:macros.h"
#include "ACE:config.h"

#include "ACE:utils/bitmap.h"
#include "ACE:utils/extview.h"
#include "ACE:managers/viewport/camera.h"

typedef struct {
	tVpManager sCommon;
	tCameraManager *pCameraManager;
	// scroll-specific fields
	tBitMap *pBuffer;
	tCopBlock *pCopBlock;
	tUwCoordYX uBfrBounds;
} tSimpleBufferManager;

tSimpleBufferManager *simpleBufferCreate(
	IN tVPort *pVPort,
	IN UWORD uwBoundWidth,
	IN UWORD uwBoundHeight
);

void simpleBufferDestroy(
	IN tSimpleBufferManager *pManager
);

void simpleBufferProcess(
	IN tSimpleBufferManager *pManager
);

#endif