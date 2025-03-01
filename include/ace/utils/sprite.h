/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_SPRITE_H_
#define _ACE_UTILS_SPRITE_H_

/**
 * @file sprite.h
 * @brief Useful utilities for sprite management if you don't intend to use the
 * ACE's sprite manager.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/managers/copper.h>

/**
 * @brief Disables given sprites on supplied copperlist at given cmd offset.
 *
 * This function doesn't add any WAIT cmd, be sure to put those cmds in VBlank.
 * Number of MOVE instructions added equals two times number of sprites disabled.
 *
 * @param pList Copperlist to be edited.
 * @param eSpriteMask Determines sprites to be disabled.
 * @param uwCmdOffs Start position on raw copperlist.
 * @param pBlankSprite 2 words of CHIP memory (the blank sprite control words)
 * @return Number of MOVE instructions added.
 */
UBYTE spriteDisableInCopRawMode(
	tCopList *pList, tSpriteMask eSpriteMask, UWORD uwCmdOffs, ULONG pBlankSprite[1]
);

/**
 *  @brief Adds copBlock which disables given sprites.
 *  Resulting copBlock is placed at 0,0 so that it will be executed during VBlank.
 *
 *  @param pList Copperlist to be edited.
 *  @param eSpriteMask Determines sprites to be disabled.
 *  @param pBlankSprite 2 words of CHIP memory (the blank sprite control words)
 *
 *  @return Pointer to newly created copBlock.
 */
tCopBlock *spriteDisableInCopBlockMode(tCopList *pList, tSpriteMask eSpriteMask, ULONG pBlankSprite[1]);

#endif // _ACE_UTILS_SPRITE_H_
