/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_MULTIPLEXED_SPRITE_H_
#define _ACE_MANAGERS_MULTIPLEXED_SPRITE_H_

/**
 * @file multiplexedsprite.h
 * @brief Several 16px, 2bpp sprites chained on one hardware sprite channel.
 *
 * Each channel gets a DMA stream with one control pair and pixel rows per
 * element, sorted by Y, ended by a zero terminator. An element is shown only
 * if its VSTART is at least the previous shown element's VSTOP + 1, because
 * the VSTOP line reloads the next control pair. Overlapping, disabled or
 * unrepresentable elements are skipped for the frame. There is no clipping.
 *
 * Every frame, for every owned channel: set state, multiplexedSpriteProcess(),
 * multiplexedSpriteProcessChannel(), then process/swap the copper list once.
 * Two DMA streams follow the two copper buffers. Nothing here waits for the
 * beam: before rewriting a buffer, the frame displaying it must be finished
 * (e.g. vPortWaitForEnd()).
 *
 * Limits:
 * - OCS/ECS sprite format only: 16px fetch. With ACE_USE_AGA_FEATURES, keep
 *   the sprite fetch width at 16px (FMODE bits 2-3 cleared). Wide AGA
 *   sprites, fine X and sprite palette banks are not supported. Not tested on
 *   AGA hardware.
 * - Coexistence with sprite.h: both managers blank all eight sprite pointers
 *   on creation. Only raw copper mode is supported: pass the same uwRawCopPos
 *   and the same blank sprite to spriteManagerCreate() and
 *   multiplexedSpriteManagerCreate(), create both before adding any sprite,
 *   and give each manager distinct channels. Mixing them in block copper
 *   mode is not supported.
 *
 * NULL sprites and invalid indices are no-ops. Struct fields are exposed but
 * owned by the manager: use the setters.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/utils/bitmap.h>
#include <ace/utils/extview.h>

typedef struct _tMultiplexedSpriteElement {
	tBitMap *pBitmap; ///< Borrowed pixel-only source, never written.
	WORD wX;
	WORD wY;
	UWORD uwHeight;
	UBYTE isEnabled;
	UBYTE isAttached;
	UBYTE isHeaderToBeUpdated;
	UBYTE isBitmapToBeUpdated; ///< Internal per-buffer dirty mask.
	UWORD pBufferOffset[2]; ///< Internal cached DMA row offsets.
} tMultiplexedSpriteElement;

typedef struct _tMultiplexedSprite {
	tBitMap *pBitmap; ///< Most recently prepared DMA stream.
	UWORD uwTotalHeight;
	UBYTE isEnabled; ///< Whole-channel enable, sampled by Process.
	UBYTE ubChannelIndex;
	UBYTE isHeaderToBeUpdated;
	UBYTE isBitmapToBeUpdated;
	UBYTE ubSpriteCount;
	tMultiplexedSpriteElement **pMultiplexedSpriteElement;
	UWORD uwMaxHeight;
	tBitMap *pDmaBitmap[2];
	tCopBfr *pBufferIdentity;
	tCopBfr *pPreparedBuffer;
	UBYTE *pOrder;
} tMultiplexedSprite;

/**
 * @brief Initialize the manager. Call once, before Add and viewLoad().
 *
 * Blanks all eight sprite pointers. In block copper mode uwRawCopPos is
 * ignored; in raw mode 16 MOVE slots starting at uwRawCopPos are used.
 * See the file comment for sharing them with sprite.h.
 *
 * @param pView View to attach to. Must stay alive until ManagerDestroy.
 * @param uwRawCopPos First of 16 reserved raw copper MOVEs.
 * @param pBlankSprite Zeroed CHIP ULONG, borrowed until ManagerDestroy.
 * Pass NULL to let the manager allocate one.
 * On failure the manager stays uninitialized and Add returns NULL.
 */
void multiplexedSpriteManagerCreate(
	const tView *pView, UWORD uwRawCopPos, ULONG pBlankSprite[1]
);

/**
 * @brief Free remaining sprites, the owned blank sprite and copper blocks.
 *
 * Remove advanced multiplexed sprites first. Disable sprite DMA and unload
 * the view before calling, and call it before viewDestroy().
 */
void multiplexedSpriteManagerDestroy(void);

/**
 * @brief Reserve a free channel with a fixed number of disabled elements.
 *
 * @param ubChannelIndex Hardware channel, 0..7.
 * @param ubSpriteHeight Maximum element height in pixels, 1..255.
 * @param ubElementCount Number of elements, 1..255.
 * @return New sprite, or NULL on invalid arguments, used channel or
 * allocation failure.
 */
tMultiplexedSprite *multiplexedSpriteAdd(
	UBYTE ubChannelIndex, UBYTE ubSpriteHeight, UBYTE ubElementCount
);

/**
 * @brief Release the sprite's DMA streams and elements.
 *
 * Borrowed bitmaps are not freed. Disable sprite DMA first and keep it
 * disabled until both copper buffers point at blank or replacement data,
 * or unload the view.
 */
void multiplexedSpriteRemove(tMultiplexedSprite *pSprite);

/**
 * @brief Set height, enable and attach state of an element.
 *
 * The whole call is ignored if height is 0, above the maximum, or above the
 * source bitmap height.
 */
void multiplexedSpriteSetElement(
	tMultiplexedSprite *pSprite, UBYTE ubIndex, UWORD uwHeight,
	UBYTE isEnabled, UBYTE isAttached
);

/**
 * @brief Set the element's source bitmap and set its height to the bitmap's.
 *
 * The bitmap must be 16px, 2bpp, interleaved, pixel rows only (no control
 * words or terminator) and no taller than the maximum height. It is
 * borrowed: keep it alive until replaced or removed. Call again with the
 * same bitmap after editing its pixels.
 */
void multiplexedSpriteSetBitmap(
	tMultiplexedSprite *pSprite, UBYTE ubIndex, tBitMap *pBitmap
);

/**
 * @brief Set the element's position, relative to the view's top-left corner.
 */
void multiplexedSpriteSetPos(
	tMultiplexedSprite *pSprite, UBYTE ubIndex, WORD wX, WORD wY
);

/**
 * @brief Show or hide an element. Other elements are not affected.
 */
void multiplexedSpriteSetEnabled(
	tMultiplexedSprite *pSprite, UBYTE ubIndex, UBYTE isEnabled
);

/**
 * @brief Set the ATTACH bit. Only odd channels can be attached.
 *
 * The matching even channel must show the low bitplanes at the same position.
 * advancedmultiplexedsprite.h does this for you.
 */
void multiplexedSpriteSetAttached(
	tMultiplexedSprite *pSprite, UBYTE ubIndex, UBYTE isAttached
);

/**
 * @brief Set the displayed height: 1..maximum and at most the bitmap height.
 */
void multiplexedSpriteSetHeight(
	tMultiplexedSprite *pSprite, UBYTE ubIndex, UWORD uwHeight
);

/**
 * @brief Force re-sorting and pixel refresh after editing an element directly.
 */
void multiplexedSpriteRequestMetadataUpdate(
	tMultiplexedSprite *pSprite, UBYTE ubIndex
);

/**
 * @brief Build the DMA stream for the current back copper buffer.
 *
 * Call every frame before ProcessChannel, even if nothing changed. Does not
 * allocate. Waits for the blitter before reading source pixels.
 */
void multiplexedSpriteProcess(tMultiplexedSprite *pSprite);

/**
 * @brief Write the channel's sprite pointer into the back copper buffer.
 *
 * Call for every owned channel before each copper swap.
 */
void multiplexedSpriteProcessChannel(UBYTE ubChannelIndex);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_MULTIPLEXED_SPRITE_H_
