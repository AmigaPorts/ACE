/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_ADVANCED_SPRITE_H_
#define _ACE_MANAGERS_ADVANCED_SPRITE_H_

/**
 * @file sprite.h
 * @brief The advanced sprite manager which rely on the basic sprite manager.
 *
 * @todo 4BPP/16 colors sprites management.
 * @todo 32 wide sprites.
 * @todo Multiplexed sprites
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/utils/bitmap.h>
#include <ace/utils/extview.h>
#include <ace/managers/sprite.h>

typedef struct tAdvancedSprite {
	tSprite **pSprites;
    UBYTE ubSpriteCount;
	WORD wX; ///< X position, measured from the left of the view.
	WORD wY; ///< Y position, measured from the top of the view.
    tBitMap **pAnimBitmap;
    UWORD uwAnimFrame; 
    UWORD uwAnimCount;
	UWORD uwHeight;
    UBYTE ubByteWidth;
    UBYTE uwWidth;
	UBYTE ubChannelIndex;
	UBYTE isEnabled;
	UBYTE isHeaderToBeUpdated;
	UBYTE is4PP; // 16-color sprites only.
} tAdvancedSprite;

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
 *
 * @see advancedSpriteManagerDestroy()
 */
void advancedSpriteManagerCreate(const tView *pView, UWORD uwRawCopPos);

/**
 * @brief Destroys the hardware sprite manager.
 * This also removes all registered sprites in the process.
 *
 * @note This function may temporarily re-enable OS.
 *
 * @see advancedSpriteManagerCreate().
 */
void advancedSpriteManagerDestroy(void);

/**
 * @brief Add sprite on selected hardware channel.
 * Currently, only single sprite per channel is supported.
 *
 * @note This function may temporarily re-enable OS.
 *
 * @param ubChannelIndex Index of the channel. 0 is the first channel.
 * @param pBitmap Bitmap to be used to display sprite. See spriteSetBitMap()'s
 * documentation for relevant constraints.
 * @param pSpriteVerticalStripBitmap Bitmap vertical strip to be used to display sprite and it's animation. Bitmap width is the target sprite width (16px or 32px).
 * @param uwSpriteHeight Height of the sprite.
 * @return Newly created advanced sprite struct on success, 0 on failure.
 *
 * @see advancedSpriteRemove()
 */
tAdvancedSprite *advancedSpriteAdd(UBYTE ubChannelIndex, tBitMap *pSpriteVerticalStripBitmap, UWORD uwSpriteHeight);

/**
 * @brief Removes given sprite from the display and destroys its struct.
 *
 * @note This function may temporarily re-enable OS.
 *
 * @param tAdvancedSprite Advanced Sprite to be destroyed.
 * @see advancedSpriteAdd()
 */
void advancedSpriteRemove(tAdvancedSprite *tAdvancedSprite);

/**
 * @brief Set Anim/content of sprite to display.
 *
 * @param pAdvancedSprite Sprite to be enabled.
 * @param animIndex Animation index from the sprite vertical strip bitmap.
 */
void advancedSpriteSetFrame(tAdvancedSprite *pAdvancedSprite, UWORD animFrame);

/**
 * @brief Enables or disables a given sprite.
 *
 * @param pAdvancedSprite Sprite to be enabled.
 * @param isEnabled Set to 1 to enable sprite, otherwise set to 0.
 */
void advancedSpriteSetEnabled(tAdvancedSprite *pAdvancedSprite, UBYTE isEnabled);

/**
 * @brief Set Position of the sprite.
 *
 * @param pAdvancedSprite Sprite to be enabled.
 * @param wX X position, measured from the left of the view.
 * @param wY Y position, measured from the top of the view.
 */
void advancedSpriteSetPos(tAdvancedSprite *pAdvancedSprite,WORD wX, WORD wY);

/**
 * @brief Set Position of the sprite.
 *
 * @param pAdvancedSprite Sprite to be enabled.
 * @param wX X position, measured from the left of the view.
 */
void advancedSpriteSetPosX(tAdvancedSprite *pAdvancedSprite,WORD wX);

/**
 * @brief Set Position of the sprite. 
 *
 * @param pAdvancedSprite Sprite to be enabled.
 * @param wY Y position, measured from the top of the view.
 */
void advancedSpriteSetPosY(tAdvancedSprite *pAdvancedSprite, WORD wY);

/**
 * @brief Updates the sprite's metadata if set as requiring update.
 * Be sure to call it at least whenever sprite's metadata needs updating.
 *
 * @param pAdvancedSprite Sprite of which screen position is to be updated.
 *
 * @see advancedSpriteProcessChannel()
 */
void advancedSpriteProcess(tAdvancedSprite *pAdvancedSprite);

/**
 * @brief Updates the given sprite channel.
 * Be sure to call it whenever the first sprite in channel have changed
 * and/or was enabled/disabled.
 *
 * @param ubChannelIndex
 *
 * @see advancedSpriteProcess()
 */
void advancedSpriteProcessChannel(UBYTE ubChannelIndex,tAdvancedSprite *pAdvancedSprite);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_SPRITE_H_
