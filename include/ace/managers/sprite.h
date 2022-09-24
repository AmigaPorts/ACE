/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_SPRITE_H_
#define _ACE_MANAGERS_SPRITE_H_

#include <ace/macros.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/extview.h>

typedef struct tSprite {
	tBitMap *pBitmap;
	WORD wX; ///< X position, measured from the left of the view.
	WORD wY; ///< Y position, measured from the top of the view.
	UBYTE ubSpriteIndex;
	UBYTE uwRawCopPos;
	UBYTE ubRawCopListRegenCount;
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
 * @see copBlockDisableSprites()
 * @see copRawDisableSprites()
 * @see systemSetDmaBit()
 * @see spriteManagerDestroy()
 */
void spriteManagerCreate(const tView *pView);

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
 * @param ubSpriteIndex Index of the channel. 0 is the first channel.
 * @param pBitmap Bitmap to be used to display sprite. See spriteSetBitmap()'s
 * documentation for relevant constraints.
 * @param uwRawCopPos If using raw coperlist mode, specifies copperlist position
 * on which relevant copper commands will be written.
 * @return Newly created sprite struct on success, 0 on failure.
 * @see spriteRemove()
 * @see spriteSetBitmap()
 */
tSprite *spriteAdd(
	UBYTE ubSpriteIndex, tBitMap *pBitmap, UWORD uwRawCopPos
);

/**
 * @brief Removes given sprite from the display and destroys its struct.
 *
 * @note This function may temporarily re-enable OS.
 *
 * @param pSprite Sprite to be destroyed.
 * @see spriteCreate()
 */
void spriteRemove(tSprite *pSprite);

/**
 * @brief Changes bitmap image used to display the sprite.
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
 * @brief Updates the sprite on the display.
 * Be sure to call it after changing the sprite position.
 *
 * @param pSprite Sprite of which screen position is to be updated.
 */
void spriteUpdate(tSprite *pSprite);

#endif // _ACE_MANAGERS_SPRITE_H_
