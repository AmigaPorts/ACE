/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/sprite.h>
#include <ace/macros.h>
#include <ace/generic/screen.h>
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/managers/mouse.h>
#include <ace/managers/log.h>
#include <ace/utils/custom.h>
#include <ace/utils/sprite.h>

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
static ULONG *s_pBlankSprite;
static UBYTE s_isOwningBlankSprite;
static tCopBlock *s_pInitialClearCopBlock;

static void spriteChannelRequestCopperUpdate(tSpriteChannel *pChannel) {
	pChannel->ubCopperRegenCount = 2; // for front/back buffers in raw mode
}

/**
 * Patch sprxpth/ptl MOVEs in the vblank disable block (WAIT 0,0).
 * Per-channel WAIT 0,1 blocks are unsafe: after the first of those waits is
 * satisfied, later WAIT 0,1 instructions have already passed this frame, so
 * the copper stalls until next line 1. If COP1LC is restarted on vblank the
 * playfield copper never runs — one sprite works, several hang the machine.
 */
static void spriteChannelWriteSprpt(UBYTE ubChannelIndex, ULONG ulSprAddr) {
	tCopBlock *pBlock = s_pInitialClearCopBlock;
	UWORD uwCmd;

	if(!pBlock) {
		return;
	}
	uwCmd = (UWORD)ubChannelIndex << 1;
	copSetMoveVal(&pBlock->pCmds[uwCmd].sMove, (UWORD)(ulSprAddr >> 16));
	copSetMoveVal(&pBlock->pCmds[uwCmd + 1].sMove, (UWORD)(ulSprAddr & 0xFFFF));
	pBlock->ubUpdated = 2;
	s_pView->pCopList->ubStatus |= STATUS_UPDATE;
}

#ifdef ACE_USE_AGA_FEATURES
/* Bytes per sprite DMA line for current FMODE sprite-fetch bits. */
static UBYTE spriteFetchLineBytes(void) {
	UBYTE ubFmode = 0;
	if(s_pView && s_pView->pFirstVPort) {
		ubFmode = s_pView->pFirstVPort->ubFmode;
	}
	switch((ubFmode >> 2) & 3) {
		case 1:
		case 2:
			return 8;  /* 32px */
		case 3:
			return 16; /* 64px */
		default:
			return 4;  /* 16px */
	}
}

static UBYTE *spriteAlignPtr(UBYTE *p, UBYTE ubAlign) {
	ULONG u = (ULONG)p;
	ULONG a = ubAlign;
	return (UBYTE *)((u + a - 1) & ~(a - 1));
}

static void spriteFreeHwData(tSprite *pSprite) {
	if(pSprite->pHwAlloc) {
		memFree(pSprite->pHwAlloc, pSprite->ulHwAllocSize);
		pSprite->pHwAlloc = 0;
		pSprite->pHwData = 0;
		pSprite->ulHwAllocSize = 0;
	}
}

/* Pad a narrower interleaved line into an FMODE-wide DMA slot (plane0 then plane1). */
static void spriteCopyLineToHw(
	UBYTE *pDst, UBYTE ubDstBytes, const UBYTE *pSrc, UBYTE ubSrcBytes
) {
	UBYTE ubSrcPlane = (UBYTE)(ubSrcBytes >> 1);
	UBYTE ubDstPlane = (UBYTE)(ubDstBytes >> 1);
	UBYTE i;
	for(i = 0; i < ubDstPlane; ++i) {
		pDst[i] = (i < ubSrcPlane) ? pSrc[i] : 0;
	}
	for(i = 0; i < ubDstPlane; ++i) {
		pDst[ubDstPlane + i] = (i < ubSrcPlane) ? pSrc[ubSrcPlane + i] : 0;
	}
}

/* Bump FMODE sprite-fetch if the bitmap is wider than the current setting. */
static UBYTE spriteEnsureMinFetch(UWORD uwPlaneBytes) {
	tVPort *pVp;
	UBYTE ubNeed;
	UBYTE ubOld;

	if(uwPlaneBytes <= 2 || !s_pView || !s_pView->pFirstVPort) {
		return 0;
	}

	pVp = s_pView->pFirstVPort;
	ubNeed = (uwPlaneBytes >= 8) ? 0x0C : 0x04;
	ubOld = (UBYTE)(pVp->ubFmode & 0x0C);
	if(ubOld == ubNeed || (ubNeed == 0x04 && ubOld == 0x0C)) {
		return 0;
	}
	if(ubNeed == 0x04 && (ubOld == 0x04 || ubOld == 0x08)) {
		return 0;
	}

	pVp->ubFmode = (UBYTE)((pVp->ubFmode & ~0x0C) | ubNeed);
	g_pCustom->fmode = pVp->ubFmode;
	return 1;
}

static void spriteBuildHwData(tSprite *pSprite) {
	tBitMap *pBm = pSprite->pBitmap;
	UWORD uwBpr;
	UWORD uwHeight;
	UBYTE ubFetch;
	UBYTE ubAlign;
	ULONG ulPayload;
	ULONG ulAlloc;
	ULONG i;
	const UBYTE *pSrc;

	if(!pBm) {
		spriteFreeHwData(pSprite);
		return;
	}

	uwBpr = pBm->BytesPerRow;
	uwHeight = pSprite->uwHeight;
	ubFetch = spriteFetchLineBytes();

	/*
	 * FMODE widens POS/CTL as well as DATA/DATB (HowToCode: 32px = 2 longwords,
	 * 64px = 4). The ACE empty first/last line is exactly that record when the
	 * bitmap line width matches the fetch — use the bitmap as the DMA list.
	 */
	if(uwBpr == ubFetch) {
		spriteFreeHwData(pSprite);
		return;
	}

	ubAlign = (ubFetch >= 16) ? 16 : 8;
	ulPayload = (ULONG)(uwHeight + 2) * ubFetch;
	ulAlloc = ulPayload + ubAlign;
	if(pSprite->pHwAlloc && pSprite->ulHwAllocSize != ulAlloc) {
		spriteFreeHwData(pSprite);
	}
	if(!pSprite->pHwAlloc) {
		pSprite->pHwAlloc = (UBYTE *)memAllocChipClear(ulAlloc);
		pSprite->ulHwAllocSize = ulAlloc;
		pSprite->pHwData = spriteAlignPtr(pSprite->pHwAlloc, ubAlign);
	}

	pSrc = (const UBYTE *)pBm->Planes[0];
	for(i = 0; i < uwHeight; ++i) {
		spriteCopyLineToHw(
			pSprite->pHwData + ubFetch + i * ubFetch, ubFetch,
			pSrc + (i + 1) * uwBpr, (UBYTE)uwBpr
		);
	}
	pSprite->isHeaderToBeUpdated = 1;
}

static void spriteRebuildAllHwData(void) {
	UBYTE i;
	for(i = 0; i < HARDWARE_SPRITE_CHANNEL_COUNT; ++i) {
		tSprite *pSprite = s_pChannelsData[i].pFirstSprite;
		if(pSprite && pSprite->pBitmap) {
			spriteBuildHwData(pSprite);
			spriteChannelRequestCopperUpdate(&s_pChannelsData[i]);
		}
	}
}

static ULONG spriteGetDmaAddr(const tSprite *pSprite) {
	if(pSprite->pHwData) {
		return (ULONG)pSprite->pHwData;
	}
	return (ULONG)pSprite->pBitmap->Planes[0];
}

void spriteRequestDataUpdate(tSprite *pSprite) {
	spriteBuildHwData(pSprite);
}
#endif

void spriteManagerCreate(const tView *pView, UWORD uwRawCopPos, ULONG pBlankSprite[1]) {
	if (pBlankSprite) {
#ifdef ACE_DEBUG
		if (!(memType(pBlankSprite) & MEMF_CHIP)) {
			logWrite("ERR: ILLEGAL NON-CHIP memory location for blank sprite!");
		}
#endif
		s_isOwningBlankSprite = 0;
		s_pBlankSprite = pBlankSprite;
	} else {
		s_isOwningBlankSprite = 1;
#ifdef ACE_USE_AGA_FEATURES
		/* 4-byte header plus one 64px data line so FMODE 3 cannot walk off CHIP. */
		s_pBlankSprite = memAllocChipClear(4 + 16);
#else
		s_pBlankSprite = memAllocChipClear(sizeof(ULONG));
#endif
		// Just to make sure we don't accidentally mismatch the control words size
		_Static_assert(sizeof(ULONG) == sizeof(tHardwareSpriteHeader), "We expect a Hardware sprite to have a ULONG sized header");
	}
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
			SPRITE_4 | SPRITE_5 | SPRITE_6 | SPRITE_7,
			s_pBlankSprite
		);
	}
	else {
		s_pInitialClearCopBlock = 0;
		spriteDisableInCopRawMode(
			s_pView->pCopList,
			SPRITE_0 | SPRITE_1 | SPRITE_2 | SPRITE_3 |
			SPRITE_4 | SPRITE_5 | SPRITE_6 | SPRITE_7, uwRawCopPos,
			s_pBlankSprite
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
	if (s_isOwningBlankSprite) {
#ifdef ACE_USE_AGA_FEATURES
		memFree(s_pBlankSprite, 4 + 16);
#else
		memFree(s_pBlankSprite, sizeof(ULONG));
#endif
	}
	systemUnuse();
}

tSprite *spriteAdd(UBYTE ubChannelIndex, tBitMap *pBitmap) {
	systemUse();
	// TODO: add support for attaching next sprite to the chain.
	// TODO: add support for attached sprites (16-color)
	tSprite *pSprite = memAllocFastClear(sizeof(*pSprite));
	pSprite->ubChannelIndex = ubChannelIndex;
	pSprite->isEnabled = 1;

	tSpriteChannel *pChannel = &s_pChannelsData[ubChannelIndex];
	if(pChannel->pFirstSprite) {
		// TODO: add support for chaining sprites
		logWrite("ERR: Sprite channel %hhu is already used\n", ubChannelIndex);
	}
	else {
		spriteChannelRequestCopperUpdate(pChannel);
		pChannel->pFirstSprite = pSprite;
	}

	spriteSetBitmap(pSprite, pBitmap);
	systemUnuse();
	return pSprite;
}

void spriteRemove(tSprite *pSprite) {
	systemUse();
	tSpriteChannel *pChannel = &s_pChannelsData[pSprite->ubChannelIndex];

	if(pChannel->pFirstSprite == pSprite) {
		// TODO: Move sprite next in chain to be the first one?
		pChannel->pFirstSprite = 0;
		spriteChannelRequestCopperUpdate(pChannel);

		if(s_pView->pCopList->ubMode == COPPER_MODE_BLOCK) {
			spriteChannelWriteSprpt(pSprite->ubChannelIndex, (ULONG)s_pBlankSprite);
		}
	}

#ifdef ACE_USE_AGA_FEATURES
	spriteFreeHwData(pSprite);
#endif
	memFree(pSprite, sizeof(*pSprite));
	systemUnuse();
}

void spriteSetEnabled(tSprite *pSprite, UBYTE isEnabled) {
	pSprite->isEnabled = isEnabled;
	// TODO: only after modifying first sprite in chain, change next sprite ptr in the prior one
	s_pChannelsData[pSprite->ubChannelIndex].ubCopperRegenCount = 2; // for front/back buffers
}

void spriteSetAttached(tSprite *pSprite, UBYTE isAttached) {
#if defined(ACE_DEBUG)
	if(pSprite->ubChannelIndex % 2 == 0) {
		logWrite(
			"ERR: Invalid sprite to set attachment on. %hhu is not an odd sprite\n",
			pSprite->ubChannelIndex
		);
		isAttached = 0;
	}
#endif
	pSprite->isAttached = isAttached;
	pSprite->isHeaderToBeUpdated = 1;
}

void spriteRequestMetadataUpdate(tSprite *pSprite) {
	pSprite->isHeaderToBeUpdated = 1;
}

void spriteSetBitmap(tSprite *pSprite, tBitMap *pBitmap) {
	if(!(pBitmap->Flags & BMF_INTERLEAVED) || pBitmap->Depth != 2) {
		logWrite(
			"ERR: Sprite channel %hhu bitmap %p isn't interleaved 2BPP\n",
			pSprite->ubChannelIndex, pBitmap
		);
		return;
	}
#if defined(ACE_USE_AGA_FEATURES)
	UBYTE uwMaxSpriteWidth = 8;
#else
	UBYTE uwMaxSpriteWidth = 2;
#endif
	UBYTE ubByteWidth = bitmapGetByteWidth(pBitmap);
	if(ubByteWidth > uwMaxSpriteWidth) {
		logWrite(
			"ERR: Unsupported sprite width: %hhu, expected %hhu\n",
			ubByteWidth * 8, uwMaxSpriteWidth
		);
		return;
	}

	pSprite->pBitmap = pBitmap;
#ifdef ACE_USE_AGA_FEATURES
	if(spriteEnsureMinFetch(ubByteWidth)) {
		spriteRebuildAllHwData();
	}
#if defined(ACE_DEBUG)
	if(ubByteWidth > 2 && s_pView && s_pView->pFirstVPort &&
		!(s_pView->pFirstVPort->eFlags & VP_FLAG_AGA)
	) {
		logWrite("ERR: Wide sprites need an AGA viewport (TAG_VPORT_USES_AGA)\n");
	}
#endif
#endif
	spriteSetHeight(pSprite, pBitmap->Rows - 2);

	tSpriteChannel *pChannel = &s_pChannelsData[pSprite->ubChannelIndex];
	spriteChannelRequestCopperUpdate(pChannel);
}

void spriteProcessChannel(UBYTE ubChannelIndex) {
	tSpriteChannel *pChannel = &s_pChannelsData[ubChannelIndex];
	if(!pChannel->ubCopperRegenCount) {
		return;
	}

	const tSprite *pSprite = pChannel->pFirstSprite;
	if(s_pView->pCopList->ubMode == COPPER_MODE_BLOCK && s_pInitialClearCopBlock) {
		ULONG ulSprAddr;

		pChannel->ubCopperRegenCount = 0;
		ulSprAddr = (
			pSprite && pSprite->isEnabled ?
#ifdef ACE_USE_AGA_FEATURES
			spriteGetDmaAddr(pSprite) :
#else
			(ULONG)(pSprite->pBitmap->Planes[0]) :
#endif
			(ULONG)s_pBlankSprite
		);
		spriteChannelWriteSprpt(ubChannelIndex, ulSprAddr);
	}
	else {
		--pChannel->ubCopperRegenCount;
		UWORD uwRawCopPos = pChannel->uwRawCopPos;
		tCopCmd *pList = &s_pView->pCopList->pBackBfr->pList[uwRawCopPos];

		ULONG ulSprAddr = (
			pSprite && pSprite->isEnabled ?
#ifdef ACE_USE_AGA_FEATURES
			spriteGetDmaAddr(pSprite) :
#else
			(ULONG)(pSprite->pBitmap->Planes[0]) :
#endif
			(ULONG)s_pBlankSprite
		);
		copSetMoveVal(&pList[0].sMove, ulSprAddr >> 16);
		copSetMoveVal(&pList[1].sMove, ulSprAddr & 0xFFFF);
	}
}

void spriteProcess(tSprite *pSprite) {
	if(!pSprite->isHeaderToBeUpdated) {
		return;
	}
	UBYTE isAttached = pSprite->isAttached;
	#if defined(ACE_DEBUG)
		if(pSprite->ubChannelIndex % 2 == 0 && pSprite->isAttached) {
			logWrite(
				"ERR: Invalid sprite to set attachment on. %hhu is not an odd sprite\n",
				pSprite->ubChannelIndex
			);
			isAttached = 0;
		}
	#endif
	// Sprite in list mode has 2-word header before and after data, each
	// occupies 1 line of the bitmap.
	UWORD uwVStart = s_pView->ubPosY + pSprite->wY;
	UWORD uwVStop = uwVStart + pSprite->uwHeight;
	UWORD uwHStart = s_pView->ubPosX - 1 + pSprite->wX; // For diwstrt 0x81, x offset equal to 128 worked fine, hence -1
	UWORD uwRawPos = (UWORD)((uwVStart << 8) | ((uwHStart) >> 1));
	UWORD uwRawCtl = (UWORD)(
		(uwVStop << 8) |
		(isAttached << 7) |
		(BTST(uwVStart, 8) << 2) |
		(BTST(uwVStop, 8) << 1) |
		BTST(uwHStart, 0)
	);

#ifdef ACE_USE_AGA_FEATURES
	/*
	 * Each sprite DMA slot fetches FMODE-wide data and Lisa keeps the first
	 * word: POS at 0, CTL at (line/2). 16px: CTL at +2. 32px: CTL at +4.
	 * Writing OCS-packed POS/CTL at +0/+2 leaves CTL=0 → VSTOP=0 → trails.
	 */
	{
		UBYTE *pBase = pSprite->pHwData
			? pSprite->pHwData
			: (UBYTE *)pSprite->pBitmap->Planes[0];
		UWORD uwLine = pSprite->pHwData
			? spriteFetchLineBytes()
			: pSprite->pBitmap->BytesPerRow;
		UWORD uwCtlOff = (UWORD)(uwLine >> 1);
		UWORD i;
		for(i = 0; i < uwLine; ++i) {
			pBase[i] = 0;
		}
		*(UWORD *)pBase = uwRawPos;
		*(UWORD *)(pBase + uwCtlOff) = uwRawCtl;
	}
#else
	tHardwareSpriteHeader *pHeader = (tHardwareSpriteHeader*)(pSprite->pBitmap->Planes[0]);
	pHeader->uwRawPos = uwRawPos;
	pHeader->uwRawCtl = uwRawCtl;
#endif
}

void spriteSetHeight(tSprite *pSprite, UWORD uwHeight) {
#if defined(ACE_DEBUG)
	UWORD uwVStart = s_pView->ubPosY + pSprite->wY;
	UWORD uwMaxHeight = SPRITE_HEIGHT_MAX - uwVStart;
	if(uwHeight >= uwMaxHeight) {
		logWrite(
			"ERR: Invalid sprite %hhu height %hu, max is %hu\n",
			pSprite->ubChannelIndex, uwHeight, uwMaxHeight
		);
		uwHeight = uwMaxHeight;
	}
#endif

	pSprite->uwHeight = uwHeight;
	pSprite->isHeaderToBeUpdated = 1;
#ifdef ACE_USE_AGA_FEATURES
	spriteBuildHwData(pSprite);
#endif
}
