/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/advancedmultiplexedsprite.h>
#include <string.h>
#include <ace/managers/blit.h>
#include <ace/managers/memory.h>
#include <ace/managers/system.h>

/**
 * @brief Check for a packed interleaved strip made of whole frames.
 */
static UBYTE isStripValid(const tBitMap *pBitmap, UBYTE ubHeight) {
	if(!pBitmap || !ubHeight || !(pBitmap->Flags & BMF_INTERLEAVED) ||
		(pBitmap->Depth != 2 && pBitmap->Depth != 4) ||
		!pBitmap->Rows || pBitmap->Rows % ubHeight) {
		return 0;
	}
	UWORD uwByteWidth = bitmapGetByteWidth(pBitmap);
	if((uwByteWidth != 2 && uwByteWidth != 4) ||
		pBitmap->BytesPerRow != uwByteWidth * pBitmap->Depth || !pBitmap->Planes[0]) {
		return 0;
	}
	for(UBYTE i = 1; i < pBitmap->Depth; ++i) {
		if(pBitmap->Planes[i] != pBitmap->Planes[0] + uwByteWidth * i) {
			return 0;
		}
	}
	return 1;
}

/**
 * @brief Split strips into owned pixel-only 16px, 2bpp frames, one per
 * hardware channel and frame.
 */
static UBYTE createFrames(
	tAdvancedMultiplexedSprite *pSprite, tBitMap *pStrip1, tBitMap *pStrip2
) {
	UWORD uwFirstCount = pStrip1->Rows / pSprite->uwHeight;
	blitWait();
	for(UWORD uwFrame = 0; uwFrame < pSprite->uwAnimCount; ++uwFrame) {
		tBitMap *pStrip = uwFrame < uwFirstCount ? pStrip1 : pStrip2;
		UWORD uwSourceFrame = uwFrame < uwFirstCount ?
			uwFrame : uwFrame - uwFirstCount;
		for(UBYTE i = 0; i < pSprite->ubSpriteCount; ++i) {
			ULONG ulIndex = (ULONG)uwFrame * pSprite->ubSpriteCount + i;
			tBitMap *pFrame = bitmapCreate(
				16, pSprite->uwHeight, 2, BMF_INTERLEAVED
			);
			if(!pFrame) {
				return 0;
			}
			pSprite->pAnimBitmap[ulIndex] = pFrame;
			UBYTE ubColumn = pSprite->is4Bpp ? i / 2 : i;
			UBYTE ubPlane = pSprite->is4Bpp ? (i & 1) * 2 : 0;
			for(UWORD uwY = 0; uwY < pSprite->uwHeight; ++uwY) {
				ULONG ulOffset = ((ULONG)uwSourceFrame * pSprite->uwHeight + uwY) *
					pStrip->BytesPerRow + ubColumn * 2;
				memcpy(pFrame->Planes[0] + 4UL * uwY,
					pStrip->Planes[ubPlane] + ulOffset, 2);
				memcpy(pFrame->Planes[1] + 4UL * uwY,
					pStrip->Planes[ubPlane + 1] + ulOffset, 2);
			}
		}
	}
	return 1;
}

tAdvancedMultiplexedSprite *advancedMultiplexedSpriteAdd(
	UBYTE ubChannelIndex, tBitMap *pStrip1, tBitMap *pStrip2,
	UBYTE ubSpriteHeight, UBYTE ubElementCount
) {
	if(!ubElementCount ||
		!isStripValid(pStrip1, ubSpriteHeight) ||
		(pStrip2 && (!isStripValid(pStrip2, ubSpriteHeight) ||
		pStrip2->Depth != pStrip1->Depth ||
		bitmapGetByteWidth(pStrip2) != bitmapGetByteWidth(pStrip1)))) {
		return 0;
	}
	UBYTE ubCount = bitmapGetByteWidth(pStrip1) / 2 * (pStrip1->Depth / 2);
	ULONG ulFrames = pStrip1->Rows / ubSpriteHeight;
	if(pStrip2) {
		ulFrames += pStrip2->Rows / ubSpriteHeight;
	}
	if(ubChannelIndex + ubCount > 8 ||
		(pStrip1->Depth == 4 && (ubChannelIndex & 1)) || ulFrames > 65535UL) {
		return 0;
	}
	systemUse();
	tAdvancedMultiplexedSprite *pSprite = memAllocFastClear(sizeof(*pSprite));
	if(!pSprite) {
		systemUnuse();
		return 0;
	}
	pSprite->ubChannelIndex = ubChannelIndex;
	pSprite->ubMultiplexedCount = ubElementCount;
	pSprite->ubSpriteCount = ubCount;
	pSprite->uwHeight = ubSpriteHeight;
	pSprite->ubByteWidth = bitmapGetByteWidth(pStrip1);
	pSprite->ubWidth = pSprite->ubByteWidth * 8;
	pSprite->uwAnimCount = ulFrames;
	pSprite->is4Bpp = pStrip1->Depth == 4;
	pSprite->isEnabled = 1;
	pSprite->isHeaderToBeUpdated = 1;
	pSprite->pMultiplexedSpriteElements = memAllocFastClear(
		sizeof(*pSprite->pMultiplexedSpriteElements) * ubElementCount
	);
	pSprite->pMultiplexedSprites = memAllocFastClear(
		sizeof(*pSprite->pMultiplexedSprites) * ubCount
	);
	pSprite->pAnimBitmap = memAllocFastClear(
		sizeof(*pSprite->pAnimBitmap) * ulFrames * ubCount
	);
	if(!pSprite->pMultiplexedSpriteElements || !pSprite->pMultiplexedSprites ||
		!pSprite->pAnimBitmap) {
		goto fail;
	}
	for(UWORD i = 0; i < ubElementCount; ++i) {
		pSprite->pMultiplexedSpriteElements[i] = memAllocFastClear(
			sizeof(tSubMultiplexedSprite)
		);
		if(!pSprite->pMultiplexedSpriteElements[i]) {
			goto fail;
		}
		pSprite->pMultiplexedSpriteElements[i]->isEnabled = 1;
	}
	// Reserve all channels before doing the more expensive frame conversion.
	for(UBYTE i = 0; i < ubCount; ++i) {
		pSprite->pMultiplexedSprites[i] = multiplexedSpriteAdd(
			ubChannelIndex + i, ubSpriteHeight, ubElementCount
		);
		if(!pSprite->pMultiplexedSprites[i]) {
			goto fail;
		}
	}
	if(!createFrames(pSprite, pStrip1, pStrip2)) {
		goto fail;
	}
	for(UBYTE i = 0; i < ubCount; ++i) {
		for(UWORD j = 0; j < ubElementCount; ++j) {
			multiplexedSpriteSetElement(pSprite->pMultiplexedSprites[i], j,
				ubSpriteHeight, 1, pSprite->is4Bpp && (i & 1));
			multiplexedSpriteSetBitmap(pSprite->pMultiplexedSprites[i], j,
				pSprite->pAnimBitmap[i]);
		}
	}
	systemUnuse();
	return pSprite;
fail:
	advancedMultiplexedSpriteRemove(pSprite);
	systemUnuse();
	return 0;
}

void advancedMultiplexedSpriteRemove(tAdvancedMultiplexedSprite *pSprite) {
	if(!pSprite) {
		return;
	}
	systemUse();
	if(pSprite->pMultiplexedSprites) {
		for(UBYTE i = 0; i < pSprite->ubSpriteCount; ++i) {
			multiplexedSpriteRemove(pSprite->pMultiplexedSprites[i]);
		}
		memFree(pSprite->pMultiplexedSprites,
			sizeof(*pSprite->pMultiplexedSprites) * pSprite->ubSpriteCount);
	}
	if(pSprite->pAnimBitmap) {
		ULONG ulCount = (ULONG)pSprite->uwAnimCount * pSprite->ubSpriteCount;
		for(ULONG i = 0; i < ulCount; ++i) {
			if(pSprite->pAnimBitmap[i]) {
				bitmapDestroy(pSprite->pAnimBitmap[i]);
			}
		}
		memFree(pSprite->pAnimBitmap, sizeof(*pSprite->pAnimBitmap) * ulCount);
	}
	if(pSprite->pMultiplexedSpriteElements) {
		for(UWORD i = 0; i < pSprite->ubMultiplexedCount; ++i) {
			if(pSprite->pMultiplexedSpriteElements[i]) {
				memFree(pSprite->pMultiplexedSpriteElements[i],
					sizeof(tSubMultiplexedSprite));
			}
		}
		memFree(pSprite->pMultiplexedSpriteElements,
			sizeof(*pSprite->pMultiplexedSpriteElements) * pSprite->ubMultiplexedCount);
	}
	memFree(pSprite, sizeof(*pSprite));
	systemUnuse();
}

void advancedMultiplexedSpriteSetEnabled(
	tAdvancedMultiplexedSprite *pSprite, UBYTE ubIndex, UBYTE isEnabled
) {
	if(pSprite && ubIndex < pSprite->ubMultiplexedCount) {
		pSprite->pMultiplexedSpriteElements[ubIndex]->isEnabled = !!isEnabled;
		pSprite->isHeaderToBeUpdated = 1;
	}
}

void advancedMultiplexedSpriteSetPos(
	tAdvancedMultiplexedSprite *pSprite, UBYTE ubIndex, WORD wX, WORD wY
) {
	if(pSprite && ubIndex < pSprite->ubMultiplexedCount) {
		pSprite->pMultiplexedSpriteElements[ubIndex]->wX = wX;
		pSprite->pMultiplexedSpriteElements[ubIndex]->wY = wY;
		pSprite->isHeaderToBeUpdated = 1;
	}
}

void advancedMultiplexedSpriteSetFrame(
	tAdvancedMultiplexedSprite *pSprite, UBYTE ubIndex, UWORD uwFrame
) {
	if(!pSprite || ubIndex >= pSprite->ubMultiplexedCount ||
		uwFrame >= pSprite->uwAnimCount ||
		pSprite->pMultiplexedSpriteElements[ubIndex]->uwAnimFrame == uwFrame) {
		return;
	}
	pSprite->pMultiplexedSpriteElements[ubIndex]->uwAnimFrame = uwFrame;
	ULONG ulIndex = (ULONG)uwFrame * pSprite->ubSpriteCount;
	for(UBYTE i = 0; i < pSprite->ubSpriteCount; ++i) {
		multiplexedSpriteSetBitmap(pSprite->pMultiplexedSprites[i], ubIndex,
			pSprite->pAnimBitmap[ulIndex + i]);
	}
}

void advancedMultiplexedSpriteProcess(tAdvancedMultiplexedSprite *pSprite) {
	if(!pSprite) {
		return;
	}
	for(UBYTE i = 0; i < pSprite->ubSpriteCount; ++i) {
		tMultiplexedSprite *pChannel = pSprite->pMultiplexedSprites[i];
		pChannel->isEnabled = pSprite->isEnabled;
		if(pSprite->isHeaderToBeUpdated) {
			UBYTE ubOffsetX = 16 * (pSprite->is4Bpp ? i / 2 : i);
			for(UWORD j = 0; j < pSprite->ubMultiplexedCount; ++j) {
				tSubMultiplexedSprite *pElement = pSprite->pMultiplexedSpriteElements[j];
				LONG lX = (LONG)pElement->wX + ubOffsetX;
				// Avoid signed WORD wrap before ordinary hardware-range checks.
				multiplexedSpriteSetPos(pChannel, j,
					lX > 32767 ? 32767 : lX, pElement->wY);
				multiplexedSpriteSetEnabled(pChannel, j, pElement->isEnabled);
			}
		}
		multiplexedSpriteProcess(pChannel);
	}
	pSprite->isHeaderToBeUpdated = 0;
}

void advancedMultiplexedSpriteProcessChannel(
	tAdvancedMultiplexedSprite *pSprite
) {
	if(pSprite) {
		for(UBYTE i = 0; i < pSprite->ubSpriteCount; ++i) {
			multiplexedSpriteProcessChannel(pSprite->ubChannelIndex + i);
		}
	}
}
