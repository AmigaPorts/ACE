/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/sprite.h>
#include <ace/managers/system.h>
#include <ace/macros.h>
#include <ace/managers/mouse.h>
#include <ace/managers/log.h>
#include <ace/utils/custom.h>
#include <ace/generic/screen.h>

static const tView *s_pView;
static tSprite *s_pFirstSpritesInChannel[HARDWARE_SPRITE_CHANNEL_COUNT];
static tCopBlock *s_pChannelCopBlocks[HARDWARE_SPRITE_CHANNEL_COUNT];

void spriteManagerCreate(const tView *pView) {
	// TODO: add support for non-chained mode (setting sprxdat with copper)?
	// TODO: add copBlockDisableSprites / copRawDisableSprites?
	s_pView = pView;
	for(UBYTE i = HARDWARE_SPRITE_CHANNEL_COUNT; i--;) {
		s_pChannelCopBlocks[i] = 0;
		s_pFirstSpritesInChannel[i] = 0;
	}
}

void spriteManagerDestroy(void) {
	systemUse();
	for(UBYTE i = HARDWARE_SPRITE_CHANNEL_COUNT; i--;) {
		tSprite *pSprite = s_pFirstSpritesInChannel[i];
		if(pSprite) {
			spriteRemove(pSprite);
		}
	}
	systemUnuse();
}

tSprite *spriteAdd(
	UBYTE ubSpriteIndex, tBitMap *pBitmap, UWORD uwRawCopPos
) {
	systemUse();
	// TODO: add support for attaching next sprite to the chain.
	// TODO: add support for attached sprites (16-color)
	tSprite *pSprite = memAllocFast(sizeof(*pSprite));
	pSprite->ubSpriteIndex = ubSpriteIndex;
	pSprite->uwRawCopPos = uwRawCopPos;
	pSprite->wX = 0;
	pSprite->wY = 0;

	if(s_pView->pCopList->ubMode == COPPER_MODE_BLOCK) {
		// TODO: when in sprite chain mode, only for first sprite in the channel
		if(s_pChannelCopBlocks[ubSpriteIndex] != 0) {
			logWrite("ERR: Sprite channel %hhu is already used\n", ubSpriteIndex);
			systemUnuse();
			return 0;
		}
		s_pChannelCopBlocks[ubSpriteIndex] = copBlockCreate(s_pView->pCopList, 2, 0, 0);
	}

	spriteSetBitmap(pSprite, pBitmap);
	systemUnuse();
	return pSprite;
}

void spriteRemove(tSprite *pSprite) {
	systemUse();
	if(s_pChannelCopBlocks[pSprite->ubSpriteIndex]) {
		copBlockDestroy(
			s_pView->pCopList, s_pChannelCopBlocks[pSprite->ubSpriteIndex]
		);
		s_pChannelCopBlocks[pSprite->ubSpriteIndex] = 0;
	}
	memFree(pSprite, sizeof(*pSprite));
	systemUnuse();
}

void spriteSetBitmap(tSprite *pSprite, tBitMap *pBitmap) {
	if(!(pBitmap->Flags & BMF_INTERLEAVED) || pBitmap->Depth != 2) {
		logWrite(
			"ERR: Sprite channel %hhu bitmap %p isn't interleaved 2BPP!\n",
			pSprite->ubSpriteIndex, pBitmap
		);
		return;
	}

	UBYTE ubByteWidth = bitmapGetByteWidth(pBitmap);
	if(ubByteWidth != 2) {
		logWrite(
			"ERR: Unsupported sprite width: %hhu, expected 16\n",
			ubByteWidth * 8
		);
		return;
	}

	pSprite->pBitmap = pBitmap;

	if (s_pView->pCopList->ubMode == COPPER_MODE_RAW) {
		pSprite->ubRawCopListRegenCount = 2; // for front/back buffers
	}
	else {
		ULONG ulSprAddr = (ULONG)(pSprite->pBitmap->Planes[0]);
		tCopBlock *pBlock = s_pChannelCopBlocks[pSprite->ubSpriteIndex];
		pBlock->uwCurrCount = 0;
		copMove(s_pView->pCopList, pBlock, &g_pSprFetch[pSprite->ubSpriteIndex].uwHi, ulSprAddr >> 16);
		copMove(s_pView->pCopList, pBlock, &g_pSprFetch[pSprite->ubSpriteIndex].uwLo, ulSprAddr & 0xFFFF);
	}

	spriteUpdate(pSprite);
}

void spriteUpdate(tSprite *pSprite) {
	// Update relevant part of current raw copperlist
	if(pSprite->ubRawCopListRegenCount) {
		--pSprite->ubRawCopListRegenCount;
		ULONG ulSprAddr = (ULONG)(pSprite->pBitmap->Planes[0]);
		tCopCmd *pList = &s_pView->pCopList->pBackBfr->pList[pSprite->uwRawCopPos];

		copSetMove(&pList[0].sMove, &g_pSprFetch[pSprite->ubSpriteIndex].uwHi, ulSprAddr >> 16);
		copSetMove(&pList[1].sMove, &g_pSprFetch[pSprite->ubSpriteIndex].uwLo, ulSprAddr & 0xFFFF);
	}

	// Sprite in list mode has 2-word header before and after data.
	// TODO: get rid of hardcoded 128 X offset in reasonable way.
	const UWORD uwSpriteHeight = pSprite->pBitmap->Rows;
	UWORD uwVStart = s_pView->ubPosY + pSprite->wY;
	UWORD uwVStop = uwVStart + uwSpriteHeight;
	UWORD uwHStart = 128 + pSprite->wX;

	tHardwareSpriteHeader *pHeader = (tHardwareSpriteHeader*)(pSprite->pBitmap->Planes[0]);
	pHeader->uwRawPos = ((uwVStart << 8) | ((uwHStart) >> 1));
	pHeader->uwRawCtl = (UWORD) (
		(uwVStop << 8) |
		(BTST(uwVStart, 8) << 2) |
		(BTST(uwVStop, 8) << 1) |
		BTST(uwHStart, 0)
	);
}
