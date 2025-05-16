/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_BOB_H_
#define _ACE_MANAGERS_BOB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>
#include <ace/managers/blit.h>

/**
 * @file "bob.h"
 * @brief The mighty bob manager.
 * Its workflow is as follows:
 *
 * in gamestate create:
 * bobManagerCreate(...)
 * bobInit(&sBob1, ...)
 * bobInit(&sBob2, ...)
 * bobInit(&sBobN, ...)
 * bobReallocateBuffers()
 *
 * in gamestate loop:
 * bobBegin()
 * someCalcHereOrOtherBlitterOperationsHere()
 * bobPush(&sBobX) <-- no other blitting past this point
 * someCalcHere()
 * bobPush(&sBobY)
 * bobPush(&sBobZ)
 * someCalcHere()
 * bobProcessNext()
 * someCalcHere()
 * bobPush(&sBobT)
 * someCalcHere()
 * bobProcessNext()
 * someCalcHere()
 * bobPushingDone()
 * someCalcHere()
 * bobProcessNext()
 * someCalcHere()
 * bobProcessNext()
 * someCalcHere()
 * bobEnd()
 * someCalcHereOrOtherBlitterOperationsHere()
 *
 * in gamestate destroy:
 * bobManagerDestroy()
 */

/**
 * @brief The bob structure.
 * You can safely change sPos to set new position. Rest is read-only and should
 * only be changed by provided fns.
 */
typedef struct tBob {
	UBYTE *pFrameData;
	UBYTE *pMaskData;
	tUwCoordYX pOldPositions[2];
	tUwCoordYX sPos;
	UWORD uwWidth;
	UWORD uwHeight;
	UBYTE isUndrawRequired;
	// Platform-dependent private fields. Don't rely on them externally.
#if defined(ACE_DEBUG)
	UWORD _uwOriginalWidth;
	UWORD _uwOriginalHeight;
#endif
	UWORD _uwBlitSize;
	WORD _wModuloUndrawSave;
	UWORD _uwInterleavedHeight;
#if defined(ACE_BOB_PRISTINE_BUFFER)
	ULONG _pSaveOffsets[2];
#else
	UBYTE *_pBufferDrawPtrs[2];
#endif
} tBob;

/**
 * @brief Creates bob manager with optional double buffering support.
 * If you use single buffering, pass same pointer in pFront and pBack.
 *
 * After calling this fn you should call series of bobInit() followed by
 * single bobReallocateBuffers().
 *
 * @param pFront Double buffering's front buffer bitmap.
 * @param pBack Double buffering's back buffer bitmap.
 * @param uwAvailHeight True available height for Y-scroll in passed bitmap.
 * For tileBuffer you should use `pTileBuffer->pScroll->uwBmAvailHeight`.
 * For scrollBuffer you should use `pScrollBuffer->uwBmAvailHeight`.
 *
 * @see bobInit()
 * @see bobReallocateBuffers()
 * @see bobManagerDestroy()
 */
void bobManagerCreate(
	tBitMap *pFront, tBitMap *pBack,
#if defined(ACE_BOB_PRISTINE_BUFFER)
	tBitMap *pPristineBuffer,
#endif
	UWORD uwAvailHeight
);

/**
 * @brief Destroys bob manager, releasing all its resources.
 *
 * @see bobManagerCreate()
 */
void bobManagerDestroy(void);

void bobManagerReset(void);

/**
 * @brief Initializes new bob for use with manager.
 *
 * @param pBob Pointer to bob structure.
 * @param uwWidth Bob's width.
 * @param uwHeight Bob's height.
 * @param isUndrawRequired If set to 1, its background will be undrawn.
 * @param pFrameData Pointer to frame to be displayed.
 * @param pMaskData Pointer to transparency mask of pFrameData.
 * @param uwX Initial X position.
 * @param uwY Initial Y position.
 */
void bobInit(
	tBob *pBob, UWORD uwWidth, UWORD uwHeight, UBYTE isUndrawRequired,
	UBYTE *pFrameData, UBYTE *pMaskData, UWORD uwX, UWORD uwY
);

/**
 * @brief Allocates buffers for storing background for later undrawing of bobs.
 * Background of all bobs are stored in single buffer. This way there is no need
 * to reconfigure blitter's destination register when storing BGs.
 *
 * After call to this function, you can't call bobInit() anymore!
 */
void bobReallocateBuffers(void);

/**
 * @brief Changes bob's animation frame.
 *
 * Storing animation frames one under another implies simplest calculations,
 * hence exclusively supported by this manager.
 *
 * @param pBob Bob which should have its frame changed.
 * @param pFrameData Pointer to frame to be displayed.
 * @param pMaskData Pointer to transparency mask of pFrameData.
 */
void bobSetFrame(tBob *pBob, UBYTE *pFrameData, UBYTE *pMaskData);

/**
 * @brief Changes bob's width.
 *
 * @warning When using BG restore for bob, Watch out for BG buffer size
 * calculations - be sure to set initial bob's width to maximum value.
 * Otherwise, you're risking memory corruption!
 *
 * @param pBob Bob which width is to be resized.
 * @param uwWidth New width.
 */
void bobSetWidth(tBob *pBob, UWORD uwWidth);

/**
 * @brief Changes bob's height.
 *
 * @warning When using BG restore for bob, Watch out for BG buffer size
 * calculations - be sure to set initial bob's height to maximum value.
 * Otherwise, you're risking memory corruption!
 *
 * @param pBob Bob which height is to be resized.
 * @param uwHeight New height.
 */
void bobSetHeight(tBob *pBob, UWORD uwHeight);

/**
 * @brief Calculates byte address of a frame located at given Y offset.
 *
 * This function assumes that bitmap is exactly 1 frame-wide and next frames
 * are located one after another.
 *
 * @param pBitmap Bitmap which stores animation frames/masks.
 * @param uwOffsetY Y Offset of frame which address is to be calculated.
 * @return Byte address of frame/mask data of given frame.
 */
UBYTE *bobCalcFrameAddress(tBitMap *pBitmap, UWORD uwOffsetY);

/**
 * @brief Undraws all bobs, restoring BG to its former state.
 * Also bob current drawing queue is reset, making room for pushing new bobs.
 * After calling this function, you may push new bobs to screen.
 *
 * @see bobPush()
 */
void bobBegin(tBitMap *pBuffer);

/**
 * @brief Adds next bob to draw queue.
 * Bobs which were pushed in previous frame but not in current will still be
 * undrawn if needed.
 * There is no z-order, thus bobs are drawn in order of pushing.
 * When this function operates, it calls bobProcessNext().
 * Don't modify bob's struct past calling this fn - there is no guarantee when
 * bob system will access its data!
 *
 * @param pBob Pointer to bob to be drawn.
 *
 * @see bobProcessNext()
 * @see bobPushingDone()
 */
void bobPush(tBob *pBob);

/**
 * @brief Tries to store BG of or draw next bob.
 * Call this function periodically to check if blitter is idle and if it is,
 * give it more work to do.
 * Before calling bobPushingDone() bobs have their BG stored so that BG of
 * later pushed bobs won't get corrupted with gfx of earlier processed ones.
 *
 * Don't use blitter for any other thing until you do bobEnd()! It will
 * heavily corrupt memory!
 *
 * @return 1 if there's still some work to do by the blitter, otherwise 0.
 */
UBYTE bobProcessNext(void);

/**
 * @brief Closes drawing queue.
 * After calling this function bobs will get actually drawn, instead of just
 * storing BGs of bobs pushed to this point.
 * It also indicates that there will be no call to bobPush() until next
 * bobBegin().
 *
 * @see bobEnd()
 */
void bobPushingDone(void);

/**
 * @brief Processes all pending bobs so far.
 * This is only for advanced usage while ensuring that the bobs pushed so far
 * were already processed, e.g. alter the bitmaps mid-bob (un)draw.
 */
void bobProcessAll(void);

/**
 * @brief Gets the index of currently processed buffer in double buffering.
 * Used only in advanced scenarios to allow external access to bob struct's
 * private fields.
 * @return Index of the buffer - either 0 or 1.
 */
UBYTE bobGetCurrentBufferIndex(void);

/**
 * @brief Ends bob processing, enforcing all remaining bobs to be drawn.
 * After making this call all other blitter operations are safe again.
 */
void bobEnd(void);

void bobDiscardUndraw(void);

/**
 * @brief Sets the current buffer to given bitmap in case it loses sync.
 * Usually used in tandem with bobDiscardUndraw() when bob system was disabled
 * for some time.
 *
 * @param pCurrent Current buffer to use. Must be same as one of passed
 * in bobManagerCreate().
 */
void bobSetCurrentBuffer(tBitMap *pCurrent);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_BOB_H_
