#ifndef GUARD_ACE_MANAGER_VIEWPORT_SIMPLEBUFFER_H
#define GUARD_ACE_MANAGER_VIEWPORT_SIMPLEBUFFER_H

// Implementacja bufora ekranu zgodnego z API AmigaOS
// korzysta ze scrollingu przez RasInfo

#include <ace/types.h>
#include <ace/macros.h>
#include <ace/config.h>

#include <ace/utils/bitmap.h>
#include <ace/utils/extview.h>
#include <ace/managers/viewport/camera.h>

typedef struct {
	tVpManager sCommon;
	tCameraManager *pCameraManager;
	// scroll-specific fields
	tBitMap *pBuffer;      ///< Bitmap buffer
	tCopBlock *pCopBlock;  ///< CopBlock containing modulo/shift/bitplane cmds
	tUwCoordYX uBfrBounds; ///< Buffer bounds in pixels
	UBYTE ubXScrollable;   ///< 1 if scrollable, otherwise 0. Read only.
} tSimpleBufferManager;

/**
 *  @brief Creates new simple-scrolled buffer manager along with required buffer
 *  bitmap.
 *  This approach is not suitable for big buffers, because you'll run
 *  out of memory quite easily.
 *  
 *  @param pVPort        Parent VPort.
 *  @param uwBoundWidth  Buffer width, in pixels.
 *  @param uwBoundHeight Buffer height, in pixels.
 *  @param ubBitmapFlags Buffer bitmap creation flags (BMF_*).
 *  @return Pointer to newly created buffer manager.
 *  
 *  @see simpleBufferDestroy
 *  @see simpleBufferSetBitmap
 */
tSimpleBufferManager *simpleBufferCreate(
	IN tVPort *pVPort,
	IN UWORD uwBoundWidth,
	IN UWORD uwBoundHeight,
	IN UBYTE ubBitmapFlags
);

 /**
 *  @brief Sets new bitmap to be displayed by buffer manager.
 *  If there was buffer created by manager, be sure to intercept & free it.
 *  Also, both buffer bitmaps must have same BPP, as difference would require
 *  copBlock realloc, which is not implemented.
 *  @param pManager The buffer manager, which buffer is to be changed.
 *  @param pBitMap  New bitmap to be used by manager.
 *  
 *  @todo Realloc copper buffer to reflect BPP change.
 */
void simpleBufferSetBitmap(
	IN tSimpleBufferManager *pManager,
	IN tBitMap *pBitMap
);

void simpleBufferDestroy(
	IN tSimpleBufferManager *pManager
);

void simpleBufferProcess(
	IN tSimpleBufferManager *pManager
);

#endif