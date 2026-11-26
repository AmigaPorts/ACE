/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_ADVANCED_MULTIPLEXED_SPRITE_H_
#define _ACE_MANAGERS_ADVANCED_MULTIPLEXED_SPRITE_H_

/**
 * @file advancedmultiplexedsprite.h
 * @brief Animated 16/32px, 2/4bpp multiplexed sprites on adjacent channels.
 *
 * Built on multiplexedsprite.h: call multiplexedSpriteManagerCreate() first.
 * The ordinary manager's scheduling rules, per-frame processing order,
 * beam synchronization, teardown rules and limits (16px OCS/ECS sprite fetch,
 * raw copper mode only when mixed with sprite.h) also apply here.
 *
 * Channels used per object: 16px/2bpp 1, 16px/4bpp or 32px/2bpp 2,
 * 32px/4bpp 4. 4bpp (15 colors + transparent) uses attached pairs, so it
 * must start on an even channel. A 32px image is two 16px columns, the
 * second at X + 16. Not AGA wide sprites.
 *
 * Frames are copied on Add: the source strips may be freed right after.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/managers/multiplexedsprite.h>

typedef struct _tSubMultiplexedSprite {
	WORD wX;
	WORD wY;
	UWORD uwAnimFrame;
	UBYTE isEnabled;
} tSubMultiplexedSprite;

typedef struct _tAdvancedMultiplexedSprite {
	tMultiplexedSprite **pMultiplexedSprites;
	UBYTE ubMultiplexedCount; ///< Number of logical elements.
	UBYTE ubSpriteCount; ///< Number of hardware channels used.
	UWORD uwAnimCount; ///< Number of animation frames.
	tBitMap **pAnimBitmap;
	UWORD uwHeight;
	UBYTE ubByteWidth;
	UBYTE ubWidth;
	UBYTE ubChannelIndex;
	UBYTE isEnabled; ///< Whole-object enable, sampled by Process.
	UBYTE isHeaderToBeUpdated;
	tSubMultiplexedSprite **pMultiplexedSpriteElements;
	UBYTE is4Bpp;
} tAdvancedMultiplexedSprite;

/**
 * @brief Create an object with enabled elements at (0,0), on frame 0.
 *
 * Strips are vertical, interleaved, 16 or 32px wide, 2 or 4bpp, made of
 * whole frames with pixel rows only. Frames are numbered through strip 1,
 * then strip 2.
 *
 * @param ubChannelIndex First channel. Must be even for 4bpp. All needed
 * channels must be free.
 * @param pStrip1 Animation strip.
 * @param pStrip2 Optional second strip with the same width and depth, or 0.
 * @param ubSpriteHeight Frame height in pixels, 1..255.
 * @param ubElementCount Number of logical elements, 1..255.
 * @return New object, or NULL on invalid input, used channel or allocation
 * failure.
 */
tAdvancedMultiplexedSprite *advancedMultiplexedSpriteAdd(
	UBYTE ubChannelIndex, tBitMap *pStrip1, tBitMap *pStrip2,
	UBYTE ubSpriteHeight, UBYTE ubElementCount
);

/**
 * @brief Free the object, its channels and copied frames.
 *
 * Same DMA/copper rules as multiplexedSpriteRemove(). Call before
 * multiplexedSpriteManagerDestroy().
 */
void advancedMultiplexedSpriteRemove(tAdvancedMultiplexedSprite *pSprite);

/**
 * @brief Select the animation frame of one element.
 */
void advancedMultiplexedSpriteSetFrame(
	tAdvancedMultiplexedSprite *pSprite, UBYTE ubIndex, UWORD uwFrame
);

/**
 * @brief Show or hide one element.
 */
void advancedMultiplexedSpriteSetEnabled(
	tAdvancedMultiplexedSprite *pSprite, UBYTE ubIndex, UBYTE isEnabled
);

/**
 * @brief Set one element's position, relative to the view's top-left corner.
 */
void advancedMultiplexedSpriteSetPos(
	tAdvancedMultiplexedSprite *pSprite, UBYTE ubIndex, WORD wX, WORD wY
);

/**
 * @brief Build the DMA streams of all channels. Call every frame.
 */
void advancedMultiplexedSpriteProcess(tAdvancedMultiplexedSprite *pSprite);

/**
 * @brief Write all channel pointers into the back copper buffer.
 *
 * Call every frame after Process, before the copper swap.
 */
void advancedMultiplexedSpriteProcessChannel(
	tAdvancedMultiplexedSprite *pSprite
);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_ADVANCED_MULTIPLEXED_SPRITE_H_
