/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/sprite.h>
#include <ace/managers/blit.h>
#include <ace/managers/sdl_private.h>

#define HARDWARE_SPRITE_CHANNEL_COUNT 8

typedef struct tSpriteChannel {
	tSprite *pFirstSprite; ///< First sprite on the chained list in channel.
} tSpriteChannel;

static tSpriteChannel s_pChannelsData[HARDWARE_SPRITE_CHANNEL_COUNT];
static tBitMap *s_pSpriteMask;
static tBitMap *s_pSpriteEmptyPlane;

static void spriteOnRender(void) {
	tBitMap *pBitMap = sdlGetSurfaceBitmap();

	for(UBYTE i = 0; i < HARDWARE_SPRITE_CHANNEL_COUNT; ++i) {
		const tSprite *pSprite = s_pChannelsData[i].pFirstSprite;
		if(!pSprite || !pSprite->isEnabled) {
			continue;
		}

		// Compose the mask
		UWORD *pSrc1 = (UWORD*)pSprite->pBitmap->Planes[0];
		UWORD *pSrc2 = (UWORD*)pSprite->pBitmap->Planes[1];
		UWORD *pDst = (UWORD*)s_pSpriteMask->Planes[0];
		for(UWORD i = 0; i < pSprite->uwHeight + 1; ++i) {
			UWORD uwMask = pSrc1[i * 2] | pSrc2[i * 2];
			pDst[i * 2] = uwMask;
			pDst[i * 2 + 1] = uwMask;
		}

		// Blit in the sprite with its bitmap modulo
		tBitMap sSpriteSrc = {
			.BytesPerRow = pSprite->pBitmap->BytesPerRow,
			.Depth = 5,
			.Flags = BMF_INTERLEAVED,
			.Planes = {},
			.Rows = pSprite->pBitmap->Rows
		};

		sSpriteSrc.Planes[0] = pSprite->pBitmap->Planes[0];
		sSpriteSrc.Planes[1] = pSprite->pBitmap->Planes[1];
		if(i <= 1) {
			// colors 16..19
			sSpriteSrc.Planes[2] = s_pSpriteEmptyPlane->Planes[0];
			sSpriteSrc.Planes[3] = s_pSpriteEmptyPlane->Planes[0];
			sSpriteSrc.Planes[4] = s_pSpriteMask->Planes[0];
		}
		else if(i <= 3) {
			// colors 20..23
			sSpriteSrc.Planes[2] = s_pSpriteMask->Planes[0];
			sSpriteSrc.Planes[3] = s_pSpriteEmptyPlane->Planes[0];
			sSpriteSrc.Planes[4] = s_pSpriteMask->Planes[0];
		}
		else if(i <= 5) {
			// colors 20..23
			sSpriteSrc.Planes[2] = s_pSpriteEmptyPlane->Planes[0];
			sSpriteSrc.Planes[3] = s_pSpriteMask->Planes[0];
			sSpriteSrc.Planes[4] = s_pSpriteMask->Planes[0];
		}
		else if(i <= 7) {
			// colors 20..23
			sSpriteSrc.Planes[2] = s_pSpriteMask->Planes[0];
			sSpriteSrc.Planes[3] = s_pSpriteMask->Planes[0];
			sSpriteSrc.Planes[4] = s_pSpriteMask->Planes[0];
		}

		blitCopyMask(
			&sSpriteSrc, 0, 1, pBitMap, pSprite->wX, pSprite->wY, 16,
			pSprite->uwHeight, s_pSpriteMask->Planes[0]
		);
	}
}

void spriteManagerCreate(const tView *pView, UWORD uwRawCopPos) {
	for(UBYTE i = HARDWARE_SPRITE_CHANNEL_COUNT; i--;) {
		s_pChannelsData[i] = (tSpriteChannel){
			.pFirstSprite = 0
		};
	}

	sdlRegisterSpriteHandler(spriteOnRender);
	s_pSpriteMask = bitmapCreate(16, 256, 2, BMF_CLEAR | BMF_INTERLEAVED);
	s_pSpriteEmptyPlane = bitmapCreate(16, 256, 2, BMF_CLEAR | BMF_INTERLEAVED);
}

void spriteManagerDestroy(void) {
	sdlRegisterSpriteHandler(0);
	for(UBYTE i = HARDWARE_SPRITE_CHANNEL_COUNT; i--;) {
		tSprite *pSprite = s_pChannelsData[i].pFirstSprite;
		if(pSprite) {
			spriteRemove(pSprite);
			s_pChannelsData[i].pFirstSprite = 0;
		}
	}

	bitmapDestroy(s_pSpriteMask);
	bitmapDestroy(s_pSpriteEmptyPlane);
}

tSprite *spriteAdd(UBYTE ubChannelIndex, tBitMap *pBitmap) {
	tSprite *pSprite = memAllocFastClear(sizeof(*pSprite));
	pSprite->ubChannelIndex = ubChannelIndex;
	pSprite->isEnabled = 1;

	tSpriteChannel *pChannel = &s_pChannelsData[ubChannelIndex];
	if(pChannel->pFirstSprite) {
		// TODO: add support for chaining sprites
		logWrite("ERR: Sprite channel %hhu is already used\n", ubChannelIndex);
	}
	else {
		pChannel->pFirstSprite = pSprite;
	}

	spriteSetBitmap(pSprite, pBitmap);
	return pSprite;
}

void spriteRemove(tSprite *pSprite) {
	tSpriteChannel *pChannel = &s_pChannelsData[pSprite->ubChannelIndex];

	if(pChannel->pFirstSprite == pSprite) {
		// TODO: Move sprite next in chain to be the first one?
		pChannel->pFirstSprite = 0;
	}

	memFree(pSprite, sizeof(*pSprite));
}

void spriteSetBitmap(tSprite *pSprite, tBitMap *pBitmap) {
	if(!(pBitmap->Flags & BMF_INTERLEAVED) || pBitmap->Depth != 2) {
		logWrite(
			"ERR: Sprite channel %hhu bitmap %p isn't interleaved 2BPP!\n",
			pSprite->ubChannelIndex, pBitmap
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
}

void spriteSetHeight(tSprite *pSprite, UWORD uwHeight) {
	pSprite->uwHeight = uwHeight;
}

void spriteSetEnabled(tSprite *pSprite, UBYTE isEnabled) {
	pSprite->isEnabled = isEnabled;
}

void spriteSetAttached(tSprite *pSprite, UBYTE isAttached) {
	pSprite->isAttached = isAttached;
}

void spriteRequestMetadataUpdate(tSprite *pSprite) {

}

void spriteProcess(tSprite *pSprite) {

}

void spriteProcessChannel(UBYTE ubChannelIndex) {

}
