/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_SPRITE_H_
#define _ACE_MANAGERS_SPRITE_H_

/**
 * @file sprite.h
 * @brief The basic sprite manager. Sets up the chained sprite list for each
 * of hardware sprite channels.
 *
 * @todo Add support for chained sprites - only one per channel atm
 * @todo Add support for attached (16-color) sprites?
 * @todo AGA differences?
 * @todo Separate spriteAdd/spriteRemove from spriteCreate/spriteDestroy
 * @todo Make allocations optional, allow using spriteInit(tSprite *) instead of Create/Destroy
 * @todo Allow using fragments of bitmap (specified Y offset) for sprite tiles support. How to solve metadata writing?
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/utils/bitmap.h>
#include <ace/utils/extview.h>

typedef struct tSprite {
	tBitMap *pBitmap;
	WORD wX; ///< X position, measured from the left of the view.
	WORD wY; ///< Y position, measured from the top of the view.
	UWORD uwHeight;
	UBYTE ubChannelIndex;
	UBYTE isEnabled;
	UBYTE isHeaderToBeUpdated;
	UBYTE isAttached; // Odd Sprites Only.
} tSprite;

/**
 * @brief Initializes the hardware sprite manager.
 *
 * @note This function may temporarily re-enable OS.
 * @note Be sure to enable sprites by turning on sprite DMA as well as disable
 * unused sprites.
 * @note This function doesn't handle the mouse input etc. automatically,
 * since one may want to control it with joy, keyboard or in other kind of way.
 *
 * @param pView View used for displaying sprites.
 * @param uwRawCopPos In raw mode, specifies an offset on where
 * the sprite commands should reside. Requires space of 16 copper commands.
 * @param pBlankSprite 2 words of CHIP memory (the blank sprite control words).
 * Pass NULL to let the manager deal with the blank sprite memory.
 *
 * @see spriteDisableInCopBlockMode()
 * @see spriteDisableInCopRawMode()
 * @see systemSetDmaBit()
 * @see spriteManagerDestroy()
 */
void spriteManagerCreate(const tView *pView, UWORD uwRawCopPos, ULONG pBlankSprite[1]);

/**
 * @brief Destroys the hardware sprite manager.
 * This also removes all registered sprites in the process.
 *
 * @note This function may temporarily re-enable OS.
 *
 * @see spriteManagerCreate().
 */
void spriteManagerDestroy(void);

/**
 * @brief Add sprite on selected hardware channel.
 * Currently, only single sprite per channel is supported.
 *
 * @note This function may temporarily re-enable OS.
 *
 * @param ubChannelIndex Index of the channel. 0 is the first channel.
 * @param pBitmap Bitmap to be used to display sprite. See spriteSetBitmap()'s
 * documentation for relevant constraints.
 * @return Newly created sprite struct on success, 0 on failure.
 *
 * @see spriteRemove()
 * @see spriteSetBitmap()
 */
tSprite *spriteAdd(UBYTE ubChannelIndex, tBitMap *pBitmap);

/**
 * @brief Removes given sprite from the display and destroys its struct.
 *
 * @note This function may temporarily re-enable OS.
 *
 * @param pSprite Sprite to be destroyed.
 * @see spriteAdd()
 */
void spriteRemove(tSprite *pSprite);

/**
 * @brief Changes bitmap image used to display the sprite.
 * Also resizes sprite to bitmap height.
 *
 * @note The sprite will write to the bitmap's bitplanes to update its control
 * words. If you need to use same bitmap for different sprites, be sure to have
 * separate copies.
 *
 * @param pSprite Sprite of which bitmap is to be updated.
 * @param pBitmap Bitmap to be used for display/control data. The bitmap must be
 * in 2BPP interleaved format as well as start and end with an empty line,
 * which will not be displayed but used for storing control data.
 */
void spriteSetBitmap(tSprite *pSprite, tBitMap *pBitmap);

/**
 * @brief Overrides sprite height to given value.
 * Also sets metadata update pending flag.
 *
 * @param pSprite Sprite of which height is to be changed.
 * @param uwHeight New sprite height. Maximum is 511.
 */
void spriteSetHeight(tSprite *pSprite, UWORD uwHeight);

/**
 * @brief Enables or disables a given sprite.
 *
 * @param pSprite Sprite to be enabled.
 * @param isEnabled Set to 1 to enable sprite, otherwise set to 0.
 */
void spriteSetEnabled(tSprite *pSprite, UBYTE isEnabled);

/**
 * @brief Sets whether the sprite is an attached sprite.
 * Attached sprites are only available on odd sprite channels.
 *
 * @param isAttached Set to 1 to enable sprite attachment, otherwise set to 0.
 *
 * @see spriteProcess()
 */
void spriteSetAttached(tSprite *pSprite, UBYTE isAttached);

/**
 * @brief Sets metadata update as pending. Be sure to call it after
 * changing sprite's position, sizing or pointer to the next sprite.
 *
 * Multiple operations on sprite may independently require rebuilding
 * its metadata.
 * This function marks it as invalid so that it will be rebuilt only once
 * by spriteProcess() later on.
 *
 * @param pSprite Sprite of which metadata should be set to pending update.
 *
 * @see spriteProcess()
 */
void spriteRequestMetadataUpdate(tSprite *pSprite);

/**
 * @brief Updates the sprite's metadata if set as requiring update.
 * Be sure to call it at least whenever sprite's metadata needs updating.
 *
 * @param pSprite Sprite of which screen position is to be updated.
 *
 * @see spriteRequestMetadataUpdate()
 * @see spriteProcessChannel()
 */
void spriteProcess(tSprite *pSprite);

/**
 * @brief Updates the given sprite channel.
 * Be sure to call it whenever the first sprite in channel have changed
 * and/or was enabled/disabled.
 *
 * @param ubChannelIndex
 *
 * @see spriteProcess()
 */
void spriteProcessChannel(UBYTE ubChannelIndex);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_SPRITE_H_
