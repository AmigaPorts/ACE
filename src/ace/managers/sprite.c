/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/sprite.h>
#include <ace/macros.h>
#include <ace/generic/screen.h>
#include <ace/managers/system.h>
#include <ace/managers/mouse.h>
#include <ace/managers/log.h>
#include <ace/utils/custom.h>
#include <ace/utils/sprite.h>
#include <ace/utils/assume.h>

#define SPRITE_VPOS_BITS 9
#define SPRITE_HEIGHT_MAX ((1 << SPRITE_VPOS_BITS) - 1)

typedef struct tSpriteChannel {
	tSprite *pFirstSprite; ///< First sprite on the chained list in channel.
	tCopBlock *pCopBlock;
	UWORD uwRawCopPos; ///< Offset for channel's first sprite fetch copper cmd.
	UBYTE ubCopperRegenCount;
} tSpriteChannel;

static const tView *s_pView;
static tSpriteChannel s_pChannelsData[HARDWARE_SPRITE_CHANNEL_COUNT];
static tHardwareSpriteHeader CHIP s_uBlankSprite;
static tCopBlock *s_pInitialClearCopBlock;

static void spriteChannelRequestCopperUpdate(tSpriteChannel *pChannel) {
	assumeNotNull(pChannel);
	pChannel->ubCopperRegenCount = 2; // for front/back buffers in raw mode
}

void spriteManagerCreate(const tView *pView, UWORD uwRawCopPos) {
	assumeNotNull(pView);

	// TODO: add support for non-chained mode (setting sprxdat with copper)?
	s_pView = pView;
	for(UBYTE i = HARDWARE_SPRITE_CHANNEL_COUNT; i--;) {
		s_pChannelsData[i] = (tSpriteChannel){
			.uwRawCopPos = uwRawCopPos + 2 * i
		};
	}

	// Initially disable all sprites so that no garbage will be fed on screen
	// before actual sprites.
	if(pView->pCopList->ubMode == COPPER_MODE_BLOCK) {
		s_pInitialClearCopBlock = spriteDisableInCopBlockMode(
			s_pView->pCopList,
			SPRITE_0 | SPRITE_1 | SPRITE_2 | SPRITE_3 |
			SPRITE_4 | SPRITE_5 | SPRITE_6 | SPRITE_7
		);
	}
	else {
		s_pInitialClearCopBlock = 0;
		spriteDisableInCopRawMode(
			s_pView->pCopList,
			SPRITE_0 | SPRITE_1 | SPRITE_2 | SPRITE_3 |
			SPRITE_4 | SPRITE_5 | SPRITE_6 | SPRITE_7, uwRawCopPos
		);
	}
}

void spriteManagerDestroy(void) {
	systemUse();
	for(UBYTE i = HARDWARE_SPRITE_CHANNEL_COUNT; i--;) {
		tSprite *pSprite = s_pChannelsData[i].pFirstSprite;
		if(pSprite) {
			spriteRemove(pSprite);
		}
	}
	if(s_pInitialClearCopBlock) {
		copBlockDestroy(s_pView->pCopList, s_pInitialClearCopBlock);
	}
	systemUnuse();
}

tSprite *spriteAdd(UBYTE ubChannelIndex, tBitMap *pBitmap) {
	assumeNotNull(pBitmap);

	systemUse();
	// TODO: add support for attaching next sprite to the chain.
	// TODO: add support for attached sprites (16-color)
	tSprite *pSprite = memAllocFastClear(sizeof(*pSprite));
	assumeNotNull(pSprite); // TODO: gracefully fail?
	pSprite->ubChannelIndex = ubChannelIndex;
	pSprite->isEnabled = 1;

	tSpriteChannel *pChannel = &s_pChannelsData[ubChannelIndex];

	// TODO: add support for chaining sprites
	assumeMsg(pChannel->pFirstSprite == 0, "Sprite channel is already used");

	spriteChannelRequestCopperUpdate(pChannel);
	pChannel->pFirstSprite = pSprite;

	if(s_pView->pCopList->ubMode == COPPER_MODE_BLOCK) {
		assumeMsg(pChannel->pCopBlock == 0, "Sprite channel already has copBlock");
		pChannel->pCopBlock = copBlockCreate(s_pView->pCopList, 2, 0, 0);
	}

	spriteSetBitmap(pSprite, pBitmap);
	systemUnuse();
	return pSprite;
}

void spriteRemove(tSprite *pSprite) {
	assumeNotNull(pSprite);

	systemUse();
	tSpriteChannel *pChannel = &s_pChannelsData[pSprite->ubChannelIndex];

	if(pChannel->pFirstSprite == pSprite) {
		// TODO: Move sprite next in chain to be the first one?
		pChannel->pFirstSprite = 0;
		spriteChannelRequestCopperUpdate(pChannel);

		if(s_pView->pCopList->ubMode == COPPER_MODE_BLOCK) {
			tCopBlock *pCopBlock = pChannel->pCopBlock;
			if(pCopBlock) {
				copBlockDestroy(s_pView->pCopList, pCopBlock);
				pChannel->pCopBlock = 0;
			}
		}
	}

	memFree(pSprite, sizeof(*pSprite));
	systemUnuse();
}

void spriteSetEnabled(tSprite *pSprite, UBYTE isEnabled) {
	assumeNotNull(pSprite);

	pSprite->isEnabled = isEnabled;
	// TODO: only after modifying first sprite in chain, change next sprite ptr in the prior one
	s_pChannelsData[pSprite->ubChannelIndex].ubCopperRegenCount = 2; // for front/back buffers
}

void spriteSetAttached(tSprite *pSprite, UBYTE isAttached) {
	assumeNotNull(pSprite);
	assumeMsg((pSprite->ubChannelIndex & 1) == 0, "Invalid sprite to set attachment on - sprite index must be odd");
	pSprite->isAttached = isAttached;
	pSprite->isHeaderToBeUpdated = 1;
}

void spriteRequestMetadataUpdate(tSprite *pSprite) {
	assumeNotNull(pSprite);

	pSprite->isHeaderToBeUpdated = 1;
}

void spriteSetBitmap(tSprite *pSprite, tBitMap *pBitmap) {
	assumeNotNull(pSprite);
	assumeNotNull(pBitmap);
	assumeMsg((pBitmap->Flags & BMF_INTERLEAVED) && pBitmap->Depth == 2, "Sprite bitmap isn't interleaved 2BPP");
	assumeMsg(bitmapGetByteWidth(pBitmap) == 2, "Unsupported sprite width, expected 16");

	pSprite->pBitmap = pBitmap;
	spriteSetHeight(pSprite, pBitmap->Rows - 2);

	tSpriteChannel *pChannel = &s_pChannelsData[pSprite->ubChannelIndex];
	spriteChannelRequestCopperUpdate(pChannel);
}

void spriteProcessChannel(UBYTE ubChannelIndex) {
	tSpriteChannel *pChannel = &s_pChannelsData[ubChannelIndex];
	if(!pChannel->ubCopperRegenCount) {
		return;
	}

	// Update relevant part of current raw copperlist
	const tSprite *pSprite = pChannel->pFirstSprite;
	if(s_pView->pCopList->ubMode == COPPER_MODE_BLOCK && pChannel->pCopBlock) {
		pChannel->ubCopperRegenCount = 0;
		tCopBlock *pCopBlock = pChannel->pCopBlock;
		pCopBlock->uwCurrCount = 0;
		ULONG ulSprAddr = (
			pSprite->isEnabled ?
			(ULONG)(pSprite->pBitmap->Planes[0]) : s_uBlankSprite.ulRaw
		);
		copMove(
			s_pView->pCopList, pCopBlock,
			&g_pSprFetch[pSprite->ubChannelIndex].uwHi, ulSprAddr >> 16
		);
		copMove(
			s_pView->pCopList, pCopBlock,
			&g_pSprFetch[pSprite->ubChannelIndex].uwLo, ulSprAddr & 0xFFFF
		);
	}
	else {
		--pChannel->ubCopperRegenCount;
		UWORD uwRawCopPos = pChannel->uwRawCopPos;
		tCopCmd *pList = &s_pView->pCopList->pBackBfr->pList[uwRawCopPos];

		ULONG ulSprAddr = (
			pSprite && pSprite->isEnabled ?
			(ULONG)(pSprite->pBitmap->Planes[0]) : s_uBlankSprite.ulRaw
		);
		copSetMoveVal(&pList[0].sMove, ulSprAddr >> 16);
		copSetMoveVal(&pList[1].sMove, ulSprAddr & 0xFFFF);
	}
}

void spriteProcess(tSprite *pSprite) {
	assumeNotNull(pSprite);

	if(!pSprite->isHeaderToBeUpdated) {
		return;
	}

	UBYTE isAttached = pSprite->isAttached;
	if(isAttached) {
		assumeMsg((pSprite->ubChannelIndex & 1) == 0, "Invalid sprite to set attachment on - sprite index must be odd");
	}

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
		(isAttached << 7) |
		(BTST(uwVStart, 8) << 2) |
		(BTST(uwVStop, 8) << 1) |
		BTST(uwHStart, 0)
	);

}

void spriteSetHeight(tSprite *pSprite, UWORD uwHeight) {
	assumeNotNull(pSprite);

	UWORD uwVStart = s_pView->ubPosY + pSprite->wY;
	UWORD uwMaxHeight = SPRITE_HEIGHT_MAX - uwVStart;
	assumeMsg(uwHeight < uwMaxHeight, "Invalid sprite height");

	pSprite->uwHeight = uwHeight;
	pSprite->isHeaderToBeUpdated = 1;
}
