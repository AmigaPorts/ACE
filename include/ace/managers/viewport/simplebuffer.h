/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GUARD_ACE_MANAGER_VIEWPORT_SIMPLEBUFFER_H
#define GUARD_ACE_MANAGER_VIEWPORT_SIMPLEBUFFER_H

#ifdef AMIGA

/**
 *  Buffer with naive scrolling techniques. Uses loadsa CHIP RAM but there
 *  should'nt be any quirks while using it.
 */

#include <ace/types.h>
#include <ace/macros.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/extview.h>
#include <ace/managers/viewport/camera.h>
#include <ace/utils/tag.h>

// vPort ptr
#define TAG_SIMPLEBUFFER_VPORT          (TAG_USER|1)
// Scrollable area bounds, in pixels
#define TAG_SIMPLEBUFFER_BOUND_WIDTH    (TAG_USER|2)
#define TAG_SIMPLEBUFFER_BOUND_HEIGHT   (TAG_USER|3)
// Buffer bitmap creation flags
#define TAG_SIMPLEBUFFER_BITMAP_FLAGS   (TAG_USER|4)
// If in raw mode, offset on copperlist for placing required copper
// instructions, specified in copper instruction count since beginning.
#define TAG_SIMPLEBUFFER_COPLIST_OFFSET (TAG_USER|6)

// Flags for internal usage.
#define SIMPLEBUFFER_FLAG_X_SCROLLABLE 1
#define SIMPLEBUFFER_FLAG_COPLIST_RAW  2

typedef struct {
	tVpManager sCommon;
	tCameraManager *pCameraManager;
	// scroll-specific fields
	tBitMap *pBuffer;      ///< Bitmap buffer
	tCopBlock *pCopBlock;  ///< CopBlock containing modulo/shift/bitplane cmds
	tUwCoordYX uBfrBounds; ///< Buffer bounds in pixels
	UBYTE ubFlags;         ///< Read only. See SIMPLEBUFFER_FLAG_*.
	UWORD uwCopperOffset;  ///< Offset on copperlist in COP_RAW mode.
} tSimpleBufferManager;

/**
 *  @brief Creates new simple-scrolled buffer manager along with required buffer
 *  bitmap.
 *  This approach is not suitable for big buffers, because you'll run
 *  out of memory quite easily.
 *
 *  @param pTags Initialization taglist.
 *  @param ...   Taglist passed as va_args.
 *  @return Pointer to newly created buffer manager.
 *
 *  @see simpleBufferDestroy
 *  @see simpleBufferSetBitmap
 */
tSimpleBufferManager *simpleBufferCreate(void *pTags,	...);

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
void simpleBufferSetBitmap(tSimpleBufferManager *pManager,	tBitMap *pBitMap);

void simpleBufferDestroy(tSimpleBufferManager *pManager);

void simpleBufferProcess(tSimpleBufferManager *pManager);

UBYTE simpleBufferIsRectVisible(
	tSimpleBufferManager *pManager,
	UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight
);

#endif // AMIGA
#endif // GUARD_ACE_MANAGER_VIEWPORT_SIMPLEBUFFER_H
