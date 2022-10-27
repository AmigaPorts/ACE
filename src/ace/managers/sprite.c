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
static tHardwareSpriteHeader CHIP s_uBlankSprite;

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
	pSprite->ubRawCopListRegenCount = 0;
	pSprite->wX = 0;
	pSprite->wY = 0;
	pSprite->uwHeight = 0;
	pSprite->isEnabled = 1;

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

void spriteEnable(tSprite *pSprite, UBYTE isEnabled) {
	pSprite->isEnabled = isEnabled;
	if(s_pView->pCopList->ubMode == COPPER_MODE_RAW) {
		pSprite->ubRawCopListRegenCount = 2; // for front/back buffers
	}
}

void spriteRequestHeaderUpdate(tSprite *pSprite) {
	pSprite->isToUpdateHeader = 1;
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
	spriteSetHeight(pSprite, pBitmap->Rows - 2);

	if(s_pView->pCopList->ubMode == COPPER_MODE_RAW) {
		pSprite->ubRawCopListRegenCount = 2; // for front/back buffers
	}
	else {
		tCopBlock *pBlock = s_pChannelCopBlocks[pSprite->ubSpriteIndex];
		pBlock->uwCurrCount = 0;
		ULONG ulSprAddr = pSprite->isEnabled ? (ULONG)(pSprite->pBitmap->Planes[0]) : s_uBlankSprite.ulRaw;
		copMove(s_pView->pCopList, pBlock, &g_pSprFetch[pSprite->ubSpriteIndex].uwHi, ulSprAddr >> 16);
		copMove(s_pView->pCopList, pBlock, &g_pSprFetch[pSprite->ubSpriteIndex].uwLo, ulSprAddr & 0xFFFF);
	}
}

void spriteUpdate(tSprite *pSprite) {
	// Update relevant part of current raw copperlist
	if(pSprite->ubRawCopListRegenCount) {
		--pSprite->ubRawCopListRegenCount;
		tCopCmd *pList = &s_pView->pCopList->pBackBfr->pList[pSprite->uwRawCopPos];

		ULONG ulSprAddr = pSprite->isEnabled ? (ULONG)(pSprite->pBitmap->Planes[0]) : s_uBlankSprite.ulRaw;
		copSetMove(&pList[0].sMove, &g_pSprFetch[pSprite->ubSpriteIndex].uwHi, ulSprAddr >> 16);
		copSetMove(&pList[1].sMove, &g_pSprFetch[pSprite->ubSpriteIndex].uwLo, ulSprAddr & 0xFFFF);
	}

	if(pSprite->isToUpdateHeader) {
		// Sprite in list mode has 2-word header before and after data, each
		// occupies 1 line of the bitmap.
		// TODO: get rid of hardcoded 128 X offset in reasonable way.
		UWORD uwVStart = s_pView->ubPosY + pSprite->wY;
		UWORD uwVStop = uwVStart + pSprite->uwHeight;
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
}

void spriteSetHeight(tSprite *pSprite, UWORD uwHeight) {
	if(uwHeight >= 512) {
		logWrite(
			"ERR: Invalid sprite %p height %hu, max is 512\n",
			pSprite, uwHeight
		);
	}

	pSprite->uwHeight = uwHeight;
	pSprite->isToUpdateHeader = 1;
}
