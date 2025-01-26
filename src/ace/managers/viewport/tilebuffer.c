/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/viewport/tilebuffer.h>
#include <ace/macros.h>
#include <ace/managers/blit.h>
#include <ace/managers/system.h>
#include <ace/utils/tag.h>
#include <proto/exec.h> // Bartman's compiler needs this

#define TILEBUFFER_MAX_TILESET_SIZE (1 << (8 * sizeof(tTileBufferTileIndex)))

// Zero the ACE_SCROLLBUFFER_X_MARGIN_SIZE/ACE_SCROLLBUFFER_Y_MARGIN_SIZE to see the undraw

static UBYTE shiftFromPowerOfTwo(UWORD uwPot) {
	UBYTE ubPower = 0;
	while(uwPot > 1) {
		uwPot >>= 1;
		++ubPower;
	}
	return ubPower;
}

#define BLIT_WORDS_NON_INTERLEAVED_BIT (0b1 << 5) // tileSize is UBYTE, top bit of width is definitely free

static void tileBufferResetRedrawState(
	tRedrawState *pState, WORD wStartX, WORD wEndX, WORD wStartY, WORD wEndY
) {
#if defined(ACE_SCROLLBUFFER_ENABLE_SCROLL_X)
	memset(&pState->sMarginL, 0, sizeof(tMarginState));
	memset(&pState->sMarginR, 0, sizeof(tMarginState));
	pState->sMarginL.wTilePos = wStartX;
	pState->sMarginR.wTilePos = wEndX;
	pState->pMarginX = &pState->sMarginR;
	pState->pMarginOppositeX = &pState->sMarginL;
#else
	(void)wStartX;
	(void)wEndX;
#endif

#if defined(ACE_SCROLLBUFFER_ENABLE_SCROLL_Y)
	memset(&pState->sMarginU, 0, sizeof(tMarginState));
	memset(&pState->sMarginD, 0, sizeof(tMarginState));
	pState->sMarginU.wTilePos = wStartY;
	pState->sMarginD.wTilePos = wEndY;
	pState->pMarginY = &pState->sMarginD;
	pState->pMarginOppositeY = &pState->sMarginU;
#else
	(void)wStartY;
	(void)wEndY;
#endif

	pState->ubPendingCount = 0;
}

static void tileBufferQueueAdd(
	tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY
) {
	// Add two times so that they're drawn properly in double buffering
#if defined ACE_DEBUG
	if(
		pManager->pRedrawStates[0].ubPendingCount > pManager->ubQueueSize - 2 ||
		pManager->pRedrawStates[1].ubPendingCount > pManager->ubQueueSize - 2
	) {
		logWrite("ERR: No more space in redraw queue");
		return;
	}
#endif
	tRedrawState *pState = &pManager->pRedrawStates[0];
	if(pState->ubPendingCount >= pManager->ubQueueSize) {
		logWrite("ERR: Pending tiles queue overflow\n");
	}
	pState->pPendingQueue[pState->ubPendingCount].uwX = uwTileX;
	pState->pPendingQueue[pState->ubPendingCount].uwY = uwTileY;
	++pState->ubPendingCount;

	pState = &pManager->pRedrawStates[1];
	if(pState->ubPendingCount >= pManager->ubQueueSize) {
		logWrite("ERR: Pending tiles queue overflow\n");
	}
	pState->pPendingQueue[pState->ubPendingCount].uwX = uwTileX;
	pState->pPendingQueue[pState->ubPendingCount].uwY = uwTileY;
	++pState->ubPendingCount;
}

void tileBufferQueueProcess(tTileBufferManager *pManager) {
	tRedrawState *pState = &pManager->pRedrawStates[pManager->ubStateIdx];
	if(pState->ubPendingCount) {
		--pState->ubPendingCount;
		UBYTE ubPendingCount = pState->ubPendingCount;
		const tUwCoordYX *pTile = &pState->pPendingQueue[ubPendingCount];
		tileBufferDrawTile(pManager, pTile->uwX, pTile->uwY);
	}
}

tTileBufferManager *tileBufferCreate(void *pTags, ...) {
	va_list vaTags;
	tTileBufferManager *pManager;
	UWORD uwTileX, uwTileY;
	UBYTE ubBitmapFlags, isDblBuf;
	UWORD uwCoplistOffStart, uwCoplistOffBreak;

	logBlockBegin("tileBufferCreate(pTags: %p, ...)", pTags);
	va_start(vaTags, pTags);

	// Feed struct with args
	pManager = memAllocFastClear(sizeof(tTileBufferManager));
	pManager->sCommon.process = (tVpManagerFn)tileBufferProcess;
	pManager->sCommon.destroy = (tVpManagerFn)tileBufferDestroy;
	pManager->sCommon.ubId = VPM_TILEBUFFER;

	tVPort *pVPort = (tVPort*)tagGet(pTags, vaTags, TAG_TILEBUFFER_VPORT, 0);
	if(!pVPort) {
		logWrite("ERR: No parent viewport (TAG_TILEBUFFER_VPORT) specified!\n");
		goto fail;
	}
	pManager->sCommon.pVPort = pVPort;
	logWrite("Parent VPort: %p\n", pVPort);

	UBYTE ubTileShift = tagGet(pTags, vaTags, TAG_TILEBUFFER_TILE_SHIFT, 0);
	if(!ubTileShift) {
		logWrite("ERR: No tile shift (TAG_TILEBUFFER_TILE_SHIFT) specified!\n");
		goto fail;
	}
	pManager->ubTileShift = ubTileShift;
	pManager->ubTileSize = 1 << ubTileShift;

	pManager->cbTileDraw = (tTileDrawCallback)tagGet(
		pTags, vaTags, TAG_TILEBUFFER_CALLBACK_TILE_DRAW, 0
	);

	pManager->pTileSet = (tBitMap*)tagGet(pTags, vaTags, TAG_TILEBUFFER_TILESET, 0);
	if(!pManager->pTileSet) {
		logWrite("ERR: No tileset (TAG_TILEBUFFER_TILESET) specified!\n");
		goto fail;
	}
	uwTileX = tagGet(pTags, vaTags, TAG_TILEBUFFER_BOUND_TILE_X, 0);
	uwTileY = tagGet(pTags, vaTags, TAG_TILEBUFFER_BOUND_TILE_Y, 0);
	if(!uwTileX || !uwTileY) {
		logWrite(
			"ERR: No tile boundaries (TAG_TILEBUFFER_BOUND_TILE_X or _Y) specified!\n"
		);
		goto fail;
	}

	pManager->pTileData = 0;
	ubBitmapFlags = tagGet(pTags, vaTags, TAG_TILEBUFFER_BITMAP_FLAGS, BMF_CLEAR);
	isDblBuf = tagGet(pTags, vaTags, TAG_TILEBUFFER_IS_DBLBUF, 0);
	uwCoplistOffStart = tagGet(pTags, vaTags, TAG_TILEBUFFER_COPLIST_OFFSET_START, -1);
	uwCoplistOffBreak = tagGet(pTags, vaTags, TAG_TILEBUFFER_COPLIST_OFFSET_BREAK, -1);
	pManager->ulMaxTilesetSize = tagGet(
		pTags, vaTags, TAG_TILEBUFFER_MAX_TILESET_SIZE, TILEBUFFER_MAX_TILESET_SIZE
	);
	tileBufferReset(pManager, uwTileX, uwTileY, ubBitmapFlags, isDblBuf, uwCoplistOffStart, uwCoplistOffBreak);

	pManager->ubQueueSize = tagGet(
		pTags, vaTags, TAG_TILEBUFFER_REDRAW_QUEUE_LENGTH, 0
	);
	if(!pManager->ubQueueSize) {
		logWrite(
			"ERR: No queue size (TAG_TILEBUFFER_REDRAW_QUEUE_LENGTH) specified!\n"
		);
		goto fail;
	}
	// This alloc could be checked in regard of double buffering
	// but I want process to be as quick as possible (one 'if' less)
	// and redraw queue has no mem footprint at all (256 bytes max?)
	pManager->pRedrawStates[0].pPendingQueue = memAllocFast(pManager->ubQueueSize);
	pManager->pRedrawStates[1].pPendingQueue = memAllocFast(pManager->ubQueueSize);
	if(
		!pManager->pRedrawStates[0].pPendingQueue ||
		!pManager->pRedrawStates[1].pPendingQueue
	) {
		goto fail;
	}

	vPortAddManager(pVPort, (tVpManager*)pManager);

	// find camera manager, create if not exists
	// camera created in scroll bfr
	pManager->pCamera = (tCameraManager*)vPortGetManager(pVPort, VPM_CAMERA);

	// Redraw shouldn't take place here because camera is not in proper pos yet,
	// also pTileData is empty

	va_end(vaTags);
	logBlockEnd("tileBufferCreate");
	return pManager;
fail:
	// TODO: proper fail
	if(pManager->pRedrawStates[0].pPendingQueue) {
		memFree(pManager->pRedrawStates[0].pPendingQueue, pManager->ubQueueSize);
	}
	if(pManager->pRedrawStates[1].pPendingQueue) {
		memFree(pManager->pRedrawStates[1].pPendingQueue, pManager->ubQueueSize);
	}
	va_end(vaTags);
	logBlockEnd("tileBufferCreate");
	return 0;
}

void tileBufferDestroy(tTileBufferManager *pManager) {
	UWORD uwCol;
	logBlockBegin("tileBufferDestroy(pManager: %p)", pManager);

	// Free tile data
	for(uwCol = pManager->uTileBounds.uwX; uwCol--;) {
		memFree(pManager->pTileData[uwCol], pManager->uTileBounds.uwY * sizeof(pManager->pTileData[uwCol][0]));
	}
	memFree(pManager->pTileData, pManager->uTileBounds.uwX * sizeof(pManager->pTileData[0]));

	// Free tile offset lookup table
	if(pManager->pTileSetOffsets) {
		memFree(pManager->pTileSetOffsets, sizeof(pManager->pTileSetOffsets[0]) * pManager->ulMaxTilesetSize);
	}

	if(pManager->pRedrawStates[0].pPendingQueue) {
		memFree(pManager->pRedrawStates[0].pPendingQueue, pManager->ubQueueSize);
	}
	if(pManager->pRedrawStates[1].pPendingQueue) {
		memFree(pManager->pRedrawStates[1].pPendingQueue, pManager->ubQueueSize);
	}

	// Free manager
	memFree(pManager, sizeof(tTileBufferManager));

	logBlockEnd("tileBufferDestroy");
}

void tileBufferReset(
	tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY,
	UBYTE ubBitmapFlags, UBYTE isDblBuf, UWORD uwCoplistOffStart, UWORD uwCoplistOffBreak
) {
	logBlockBegin(
		"tileBufferReset(pManager: %p, uwTileX: %hu, uwTileY: %hu, ubBitmapFlags: %hhu, isDblBuf: %hhu)",
		pManager, uwTileX, uwTileY, ubBitmapFlags, isDblBuf
	);

	// Free old tile data
	if(pManager->pTileData) {
		for(UWORD uwCol = pManager->uTileBounds.uwX; uwCol--;) {
			memFree(pManager->pTileData[uwCol], pManager->uTileBounds.uwY * sizeof(pManager->pTileData[uwCol][0]));
		}
		memFree(pManager->pTileData, pManager->uTileBounds.uwX * sizeof(pManager->pTileData[0]));
		pManager->pTileData = 0;
	}

	// Free old tile offset lookup table
	if(pManager->pTileSetOffsets) {
		memFree(pManager->pTileSetOffsets, sizeof(pManager->pTileSetOffsets[0]) * pManager->ulMaxTilesetSize);
	}

	// Init new tile data
	pManager->uTileBounds.uwX = uwTileX;
	pManager->uTileBounds.uwY = uwTileY;
	if(uwTileX && uwTileY) {
		pManager->pTileData = memAllocFast(uwTileX * sizeof(pManager->pTileData[0]));
		for(UWORD uwCol = uwTileX; uwCol--;) {
			pManager->pTileData[uwCol] = memAllocFastClear(uwTileY * sizeof(pManager->pTileData[uwCol][0]));
		}
	}

	// Init tile offset lookup table
	pManager->pTileSetOffsets = memAllocFast(sizeof(pManager->pTileSetOffsets[0]) * pManager->ulMaxTilesetSize);
	for (ULONG i = 0; i < pManager->ulMaxTilesetSize; ++i) {
		pManager->pTileSetOffsets[i] = pManager->pTileSet->Planes[0] + (pManager->pTileSet->BytesPerRow * (i << pManager->ubTileShift));
	}

	// Reset scrollManager, create if not exists
	UBYTE ubTileShift = pManager->ubTileShift;
	pManager->pScroll = (tScrollBufferManager*)vPortGetManager(
		pManager->sCommon.pVPort, VPM_SCROLL
	);
	if(!(pManager->pScroll)) {
		pManager->pScroll = scrollBufferCreate(0,
			TAG_SCROLLBUFFER_VPORT, pManager->sCommon.pVPort,
			TAG_SCROLLBUFFER_MARGIN_WIDTH, pManager->ubTileSize,
			TAG_SCROLLBUFFER_BOUND_WIDTH, uwTileX << ubTileShift,
			TAG_SCROLLBUFFER_BOUND_HEIGHT, uwTileY << ubTileShift,
			TAG_SCROLLBUFFER_IS_DBLBUF, isDblBuf,
			TAG_SCROLLBUFFER_BITMAP_FLAGS, ubBitmapFlags,
			TAG_SCROLLBUFFER_COPLIST_OFFSET_START, uwCoplistOffStart,
			TAG_SCROLLBUFFER_COPLIST_OFFSET_BREAK, uwCoplistOffBreak,
		TAG_DONE);
	}
	else {
		scrollBufferReset(
			pManager->pScroll, pManager->ubTileSize,
			uwTileX << ubTileShift, uwTileY << ubTileShift, ubBitmapFlags, isDblBuf
		);
	}

	// Scrollin on one of dirs may be disabled - less redraw on other axis margin
	pManager->uwMarginedWidth = bitmapGetByteWidth(pManager->pScroll->pFront) * 8;
	pManager->uwMarginedHeight = pManager->pScroll->uwBmAvailHeight;
	pManager->ubMarginXLength = MIN(
		pManager->uTileBounds.uwY,
		(pManager->sCommon.pVPort->uwHeight >> ubTileShift) + 2 * (ACE_SCROLLBUFFER_Y_MARGIN_SIZE + SCROLLBUFFER_Y_DRAW_MARGIN_SIZE)
	);
	pManager->ubMarginYLength = MIN(
		pManager->uTileBounds.uwX,
		(pManager->sCommon.pVPort->uwWidth >> ubTileShift) + 2 * (ACE_SCROLLBUFFER_X_MARGIN_SIZE + SCROLLBUFFER_X_DRAW_MARGIN_SIZE)
	);
	logWrite(
		"Margin sizes: %hhu,%hhu\n",
		pManager->ubMarginXLength, pManager->ubMarginYLength
	);

	// Reset margin redraw structs - margin positions will be set correctly
	// by tileBufferRedrawAll()
	tileBufferResetRedrawState(&pManager->pRedrawStates[0], 0, 0, 0, 0);
	tileBufferResetRedrawState(&pManager->pRedrawStates[1], 0, 0, 0, 0);

	logBlockEnd("tileBufferReset()");
}

/**
 * Prepare quick drawing of tiles by setting up all blitter
 * registers that stay constant when blitting multiple tiles
 * quickly. To be followed by a loop of tileBufferContinueTileDraw
 * calls.
 *
 * @return bltsize - to use in tileBufferContinueTileDraw
 */
static UWORD tileBufferSetupTileDraw(const tTileBufferManager *pManager) {
	WORD wDstModulo, wSrcModulo;

	UWORD uwBlitWords = pManager->ubTileSize / 16;
	UWORD uwBltCon0 = USEA|USED|MINTERM_A;

	wSrcModulo = bitmapGetByteWidth(pManager->pTileSet) - uwBlitWords * sizeof(UWORD);
	wDstModulo = bitmapGetByteWidth(pManager->pScroll->pBack) - uwBlitWords * sizeof(UWORD);

	UBYTE ubSrcInterleaved = bitmapIsInterleaved(pManager->pTileSet);
	UBYTE ubDstInterleaved = bitmapIsInterleaved(pManager->pScroll->pBack);

	UWORD uwHeight = pManager->ubTileSize;

	if(ubSrcInterleaved && ubDstInterleaved) {
		uwHeight *= pManager->pTileSet->Depth;
	}
	else {
		// XXX: misuse uwBlitWords to store the flag for non-interleaved
		uwBlitWords |= BLIT_WORDS_NON_INTERLEAVED_BIT;
		if (ubSrcInterleaved ^ ubDstInterleaved) {
			// Since you're using this fn for speed
			logWrite("WARN: Mixed interleaved - you're losing lots of performance here!\n");
		}
		if(ubSrcInterleaved) {
			wSrcModulo += pManager->pTileSet->BytesPerRow * (pManager->pTileSet->Depth-1);
		}
		else if(ubDstInterleaved) {
			wDstModulo += pManager->pScroll->pBack->BytesPerRow * (pManager->pScroll->pBack->Depth-1);
		}
	}

	blitWait(); // Don't modify registers when other blit is in progress
	g_pCustom->bltcon0 = uwBltCon0;
	g_pCustom->bltcon1 = 0;
	g_pCustom->bltafwm = 0xFFFF;
	g_pCustom->bltalwm = 0xFFFF;
	g_pCustom->bltamod = wSrcModulo;
	g_pCustom->bltdmod = wDstModulo;

	return (uwHeight << 6) | uwBlitWords;
}

/**
 * Quickly draw a tile. This is used after the blitter was set up
 * with a call to tileBufferSetupTileDraw, to draw multiple tiles
 * as quickly as possible. The fastest path by far is for interleaved
 * bitmaps, but it works for non-interleaved bitmaps, too, with
 * a slightly larger performance hit.
 */
FN_HOTSPOT
static inline void tileBufferContinueTileDraw(
	const tTileBufferManager *pManager, const tTileBufferTileIndex *pTileDataColumn,
	UWORD uwTileY, UWORD uwBltsize, ULONG ulDstOffs, PLANEPTR pDstPlane, UBYTE ubSetDst
) {
	tTileBufferTileIndex TileToDraw = pTileDataColumn[uwTileY];

	if (!(uwBltsize & BLIT_WORDS_NON_INTERLEAVED_BIT)) {
		UBYTE *pUbBltapt = pManager->pTileSetOffsets[TileToDraw];
		UBYTE *pUbBltdpt;
		if (ubSetDst) {
			// this function should be inlined into the caller, where
			// ubSetDst should be a *constant* argument, so this check
			// folds away
			pUbBltdpt = pDstPlane + ulDstOffs;
		}

		blitWait(); // Don't modify registers when other blit is in progress
		g_pCustom->bltapt = pUbBltapt;
		if (ubSetDst) {
			g_pCustom->bltdpt = pUbBltdpt;
		}
		g_pCustom->bltsize = uwBltsize;
	}
	else {
		ULONG ulSrcOffs = (ULONG)pManager->pTileSetOffsets[TileToDraw] - (ULONG)pManager->pTileSet->Planes[0];
		for(UBYTE ubPlane = pManager->pTileSet->Depth; ubPlane--;) {
			UBYTE *pUbBltapt = pManager->pTileSet->Planes[ubPlane] + ulSrcOffs;
			UBYTE *pUbBltdpt = pManager->pScroll->pBack->Planes[ubPlane] + ulDstOffs;
			blitWait();  // Don't modify registers when other blit is in progress
			g_pCustom->bltapt = pUbBltapt;
			g_pCustom->bltdpt = pUbBltdpt;
			g_pCustom->bltsize = uwBltsize & ~BLIT_WORDS_NON_INTERLEAVED_BIT;
		}
	}
}

FN_HOTSPOT
void tileBufferProcess(tTileBufferManager *pManager) {
#if defined(ACE_SCROLLBUFFER_ENABLE_SCROLL_X) || defined(ACE_SCROLLBUFFER_ENABLE_SCROLL_Y)
	tRedrawState *pState = &pManager->pRedrawStates[pManager->ubStateIdx];
	UBYTE ubTileSize = pManager->ubTileSize;
	UBYTE ubTileShift = pManager->ubTileShift;
#endif

#if defined(ACE_SCROLLBUFFER_ENABLE_SCROLL_X)
	// X movement
	WORD wMarginXPos;
	WORD wDeltaX = cameraGetDeltaX(pManager->pCamera);
	if (wDeltaX) {
		// determine movement direction - right or left
		if (wDeltaX > 0) {
			wMarginXPos = ((
				pManager->pCamera->uPos.uwX + pManager->sCommon.pVPort->uwWidth
			) >> ubTileShift) + ACE_SCROLLBUFFER_X_MARGIN_SIZE;
			pState->pMarginX = &pState->sMarginR;
			pState->pMarginOppositeX = &pState->sMarginL;
		}
		else {
			wMarginXPos = (pManager->pCamera->uPos.uwX >> ubTileShift) - ACE_SCROLLBUFFER_X_MARGIN_SIZE;
			pState->pMarginX = &pState->sMarginL;
			pState->pMarginOppositeX = &pState->sMarginR;
		}
		// Not redrawing same column on movement side?
		if (wMarginXPos != pState->pMarginX->wTilePos) {
			// Not finished redrawing all column tiles?
			if(pState->pMarginX->wTileCurr < pState->pMarginX->wTileEnd) {
				UWORD uwTileOffsY = SCROLLBUFFER_HEIGHT_MODULO(
					pState->pMarginX->wTileCurr << ubTileShift, pManager->uwMarginedHeight
				);
				UWORD uwTileOffsX = (pState->pMarginX->wTilePos << ubTileShift);
				// Redraw remaining tiles
				UWORD uwBltsize = tileBufferSetupTileDraw(pManager);
				UWORD uwTileCurr = pState->pMarginX->wTileCurr;
				UWORD uwTileEnd = pState->pMarginX->wTileEnd;
				UWORD uwMarginedHeight = pManager->uwMarginedHeight;
				UWORD uwTilePos = pState->pMarginX->wTilePos;
				const tTileBufferTileIndex *pTileColumn = pManager->pTileData[uwTilePos];
				UWORD uwDstBytesPerRow = pManager->pScroll->pBack->BytesPerRow;
				PLANEPTR pDstPlane = pManager->pScroll->pBack->Planes[0];
				ULONG ulDstOffs = uwDstBytesPerRow * uwTileOffsY + uwTileOffsX / 8;
				UWORD uwDstOffsStep = uwDstBytesPerRow * ubTileSize;
				// set up the first bltdpt for an interleaved blit. if this isn't
				// interleaved, this is wasted, but interleaved will be faster with
				// this. we can just do this here, since tileBufferSetupTileDraw
				// already waited for the blitter to be idle
				g_pCustom->bltdpt = pDstPlane + ulDstOffs;
				while (uwTileCurr < uwTileEnd) {
					tileBufferContinueTileDraw(
						pManager, pTileColumn, uwTileCurr,
						uwBltsize, ulDstOffs, pDstPlane,
						// do not set bltdpt, it was left at the right place by the previous blit
						0
					);
					++uwTileCurr;
					uwTileOffsY += ubTileSize;
					if(uwTileOffsY >= uwMarginedHeight) {
						uwTileOffsY -= uwMarginedHeight;
						ulDstOffs = uwDstBytesPerRow * uwTileOffsY + uwTileOffsX / 8;
						blitWait(); // this happens at most once in a column, so we take the hit
						g_pCustom->bltdpt = pDstPlane + ulDstOffs;
					}
					else {
						ulDstOffs += uwDstOffsStep;
					}
				}
				if (pManager->cbTileDraw) {
					uwTileOffsY = SCROLLBUFFER_HEIGHT_MODULO(
						pState->pMarginX->wTileCurr << ubTileShift, pManager->uwMarginedHeight
					);
					uwTileCurr = pState->pMarginX->wTileCurr;
					while (uwTileCurr < uwTileEnd) {
						pManager->cbTileDraw(uwTilePos, uwTileCurr, pManager->pScroll->pBack, uwTileOffsX, uwTileOffsY);
						++uwTileCurr;
						uwTileOffsY = SCROLLBUFFER_HEIGHT_MODULO(
							uwTileOffsY + ubTileSize, uwMarginedHeight
						);
					}
				}
				pState->pMarginX->wTileCurr = pState->pMarginX->wTileEnd;
			}
			// Prepare new column redraw data
			pState->pMarginX->wTilePos = wMarginXPos;
			if (wMarginXPos < 0 || wMarginXPos >= pManager->uTileBounds.uwX) {
				// Don't redraw if new column is out of map bounds
				pState->pMarginX->wTileCurr = 0;
				pState->pMarginX->wTileEnd = 0;
			}
			else {
				// Prepare new column for redraw
				pState->pMarginX->wTileCurr = MAX(
					0, (pManager->pCamera->uPos.uwY >> ubTileShift) - (ACE_SCROLLBUFFER_Y_MARGIN_SIZE + SCROLLBUFFER_Y_DRAW_MARGIN_SIZE)
				);
				pState->pMarginX->wTileEnd = MIN(
					pState->pMarginX->wTileCurr + pManager->ubMarginXLength,
					pManager->uTileBounds.uwY
				);
			}
			// Modify margin data on opposite side
			if(wDeltaX < 0) {
				--pState->pMarginOppositeX->wTilePos;
			}
			else {
				++pState->pMarginOppositeX->wTilePos;
			}
			pState->pMarginOppositeX->wTileCurr = 0;
			pState->pMarginOppositeX->wTileEnd = 0;
		}
	}

	// Redraw another X tile - regardless of movement in that direction
	if (pState->pMarginX->wTileCurr < pState->pMarginX->wTileEnd) {
		tileBufferDrawTile(
			pManager, pState->pMarginX->wTilePos, pState->pMarginX->wTileCurr
		);
		++pState->pMarginX->wTileCurr;
	}
#endif // defined(ACE_SCROLLBUFFER_ENABLE_SCROLL_X)

#if defined(ACE_SCROLLBUFFER_ENABLE_SCROLL_Y)
	// Y movement
	WORD wMarginYPos;
	WORD wDeltaY = cameraGetDeltaY(pManager->pCamera);
	if (wDeltaY) {
		// determine redraw row - down or up
		if (wDeltaY > 0) {
			wMarginYPos = ((
				pManager->pCamera->uPos.uwY + pManager->sCommon.pVPort->uwHeight
			) >> ubTileShift) + ACE_SCROLLBUFFER_Y_MARGIN_SIZE;
			pState->pMarginY = &pState->sMarginD;
			pState->pMarginOppositeY = &pState->sMarginU;
		}
		else {
			wMarginYPos = (pManager->pCamera->uPos.uwY >> ubTileShift) - ACE_SCROLLBUFFER_Y_MARGIN_SIZE;
			pState->pMarginY = &pState->sMarginU;
			pState->pMarginOppositeY = &pState->sMarginD;
		}
		// Not drawing same row?
		if (wMarginYPos != pState->pMarginY->wTilePos) {
			// Not finished redrawing all row tiles?
			if(pState->pMarginY->wTileCurr < pState->pMarginY->wTileEnd) {
				UWORD uwTileOffsY = SCROLLBUFFER_HEIGHT_MODULO(
					pState->pMarginY->wTilePos << ubTileShift, pManager->uwMarginedHeight
				);
				UWORD uwTileOffsX = (pState->pMarginY->wTileCurr << ubTileShift);
				// Redraw remaining tiles
				UWORD uwBltsize = tileBufferSetupTileDraw(pManager);
				UWORD uwTileCurr = pState->pMarginY->wTileCurr;
				UWORD uwTileEnd = pState->pMarginY->wTileEnd;
				UWORD uwTilePos = pState->pMarginY->wTilePos;
				tTileBufferTileIndex **pTileData = pManager->pTileData;
				PLANEPTR pDstPlane = pManager->pScroll->pBack->Planes[0];
				ULONG ulDstOffs = pManager->pScroll->pBack->BytesPerRow * uwTileOffsY + uwTileOffsX / 8;
				UWORD uwDstOffsStep = ubTileSize / 8;
				while(uwTileCurr < uwTileEnd) {
					tileBufferContinueTileDraw(
						pManager, pTileData[uwTileCurr], uwTilePos,
						uwBltsize, ulDstOffs, pDstPlane, 1
					);
					++uwTileCurr;
					ulDstOffs += uwDstOffsStep;
				}
				if (pManager->cbTileDraw) {
					uwTileCurr = pState->pMarginY->wTileCurr;
					while (uwTileCurr < uwTileEnd) {
						pManager->cbTileDraw(uwTileCurr, uwTilePos, pManager->pScroll->pBack, uwTileOffsX, uwTileOffsY);
						++uwTileCurr;
						uwTileOffsX += ubTileSize;
					}
				}
				pState->pMarginY->wTileCurr = pState->pMarginY->wTileEnd;
			}
			// Prepare new row redraw data
			pState->pMarginY->wTilePos = wMarginYPos;
			if (wMarginYPos < 0 || wMarginYPos >= pManager->uTileBounds.uwY) {
				// Don't redraw if new row is out of map bounds
				pState->pMarginY->wTileCurr = 0;
				pState->pMarginY->wTileEnd = 0;
			}
			else {
				// Prepare new row for redraw
				pState->pMarginY->wTileCurr = MAX(
					0, (pManager->pCamera->uPos.uwX >> ubTileShift) - (ACE_SCROLLBUFFER_X_MARGIN_SIZE + SCROLLBUFFER_X_DRAW_MARGIN_SIZE)
				);
				pState->pMarginY->wTileEnd = MIN(
					pState->pMarginY->wTileCurr + pManager->ubMarginYLength,
					pManager->uTileBounds.uwX
				);
			}
			// Modify opposite margin data
			if(wDeltaY > 0) {
				++pState->pMarginOppositeY->wTilePos;
			}
			else {
				--pState->pMarginOppositeY->wTilePos;
			}
			pState->pMarginOppositeY->wTileCurr = 0;
			pState->pMarginOppositeY->wTileEnd = 0;
		}
	}

	// Redraw another Y tile - regardless of movement in that direction
	if (pState->pMarginY->wTileCurr < pState->pMarginY->wTileEnd) {
		tileBufferDrawTile(
			pManager, pState->pMarginY->wTileCurr, pState->pMarginY->wTilePos
		);
		++pState->pMarginY->wTileCurr;
	}
#endif // defined(ACE_SCROLLBUFFER_ENABLE_SCROLL_Y)

	pManager->ubStateIdx = !pManager->ubStateIdx;
}

void tileBufferRedrawAll(tTileBufferManager *pManager) {
	logBlockBegin("tileBufferRedrawAll(pManager: %p)", pManager);

	UBYTE ubTileSize = pManager->ubTileSize;
	UBYTE ubTileShift = pManager->ubTileShift;

	WORD wStartX = MAX(0, (pManager->pCamera->uPos.uwX >> ubTileShift) - ACE_SCROLLBUFFER_X_MARGIN_SIZE);
	WORD wStartY = MAX(0, (pManager->pCamera->uPos.uwY >> ubTileShift) - ACE_SCROLLBUFFER_Y_MARGIN_SIZE);
	// One of bounds may be smaller than viewport + margin size
	UWORD uwEndX = MIN(
		pManager->uTileBounds.uwX,
		wStartX + (pManager->uwMarginedWidth >> ubTileShift)
	);
	UWORD uwEndY = MIN(
		pManager->uTileBounds.uwY,
		wStartY + (pManager->uwMarginedHeight >> ubTileShift)
	);
	// Reset margin redraw structs as we're redrawing everything anyway
	tileBufferResetRedrawState(
		&pManager->pRedrawStates[0], wStartX, uwEndX, wStartY, uwEndY
	);
	tileBufferResetRedrawState(
		&pManager->pRedrawStates[1], wStartX, uwEndX, wStartY, uwEndY
	);

	UWORD uwTileOffsY = SCROLLBUFFER_HEIGHT_MODULO(
		wStartY << ubTileShift, pManager->uwMarginedHeight
	);
	UWORD uwDstBytesPerRow = pManager->pScroll->pBack->BytesPerRow;
	PLANEPTR pDstPlane = pManager->pScroll->pBack->Planes[0];
	tTileBufferTileIndex **pTileData = pManager->pTileData;
	UWORD uwBltsize = tileBufferSetupTileDraw(pManager);
	UWORD uwTileOffsX = (wStartX << ubTileShift);
	UWORD uwDstOffsStep = ubTileSize / 8;
	systemSetDmaBit(DMAB_BLITHOG, 1);
	for (UWORD uwTileY = wStartY; uwTileY < uwEndY; ++uwTileY) {
		UWORD uwTileCurr = wStartX;
		ULONG ulDstOffs = uwDstBytesPerRow * uwTileOffsY + uwTileOffsX / 8;
		while(uwTileCurr < uwEndX) {
			tileBufferContinueTileDraw(
				pManager, pTileData[uwTileCurr], uwTileY,
				uwBltsize, ulDstOffs, pDstPlane, 1
			);
			++uwTileCurr;
			ulDstOffs += uwDstOffsStep;
		}
		uwTileOffsY = SCROLLBUFFER_HEIGHT_MODULO(
			uwTileOffsY + ubTileSize, pManager->uwMarginedHeight
		);
	}

	if (pManager->cbTileDraw) {
		uwTileOffsY = SCROLLBUFFER_HEIGHT_MODULO(wStartY << ubTileShift, pManager->uwMarginedHeight);
		for (UWORD uwTileY = wStartY; uwTileY < uwEndY; ++uwTileY) {
			uwTileOffsX = (wStartX << ubTileShift);
			UWORD uwTileCurr = wStartX;
			while (uwTileCurr < uwEndX) {
				pManager->cbTileDraw(
					uwTileCurr, uwTileY, pManager->pScroll->pBack,
					uwTileOffsX, uwTileOffsY
				);
				++uwTileCurr;
				uwTileOffsX += ubTileSize;
			}
			uwTileOffsY = SCROLLBUFFER_HEIGHT_MODULO(
				uwTileOffsY + ubTileSize, pManager->uwMarginedHeight
			);
		}
	}

	systemSetDmaBit(DMAB_BLITHOG, 0);

	// Copy from back buffer to front buffer.
	// Width is always a multiple of 16, so use WORD copy.
	// TODO: this could be done using blitter.
	UWORD *pSrc = (UWORD*)pManager->pScroll->pBack->Planes[0];
	UWORD *pDst = (UWORD*)pManager->pScroll->pFront->Planes[0];
	ULONG ulWordsToCopy = (
		pManager->pScroll->pFront->BytesPerRow * pManager->pScroll->pFront->Rows
	) / sizeof(UWORD);
	while(ulWordsToCopy--) {
		*(pDst++) = *(pSrc++);
	}

	// Refresh bitplane pointers in scrollBuffer's copprtlist - 2x for dbl bfr
	scrollBufferProcess(pManager->pScroll);
	scrollBufferProcess(pManager->pScroll);

	logBlockEnd("tileBufferRedrawAll()");
}

void tileBufferDrawTile(
	const tTileBufferManager *pManager, UWORD uwTileIdxX, UWORD uwTileIdxY
) {
	// Buffer X coord will overflow dimensions but that's fine 'cuz we need to
	// draw on bitplane 1 as if it is part of bitplane 0.
	UWORD uwBfrY = SCROLLBUFFER_HEIGHT_MODULO(
		uwTileIdxY << pManager->ubTileShift, pManager->uwMarginedHeight
	);
	UWORD uwBfrX = (uwTileIdxX << pManager->ubTileShift);

	tileBufferDrawTileQuick(pManager, uwTileIdxX, uwTileIdxY, uwBfrX, uwBfrY);
}

void tileBufferDrawTileQuick(
	const tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY,
	UWORD uwBfrX, UWORD uwBfrY
) {
	tTileBufferTileIndex TileToDraw = pManager->pTileData[uwTileX][uwTileY];
	UBYTE ubTileShift = pManager->ubTileShift;
	// This can't use safe blit fn because when scrolling in X direction,
	// we need to draw on bitplane 1 as if it is part of bitplane 0.
	blitUnsafeCopyAligned(
		pManager->pTileSet,
		0, TileToDraw << ubTileShift,
		pManager->pScroll->pBack, uwBfrX, uwBfrY,
		pManager->ubTileSize, pManager->ubTileSize
	);
	if(pManager->cbTileDraw) {
		pManager->cbTileDraw(
			uwTileX, uwTileY, pManager->pScroll->pBack, uwBfrX, uwBfrY
		);
	}
}

void tileBufferInvalidateRect(
	tTileBufferManager *pManager, UWORD uwX, UWORD uwY,
	UWORD uwWidth, UWORD uwHeight
) {
	// Tile x/y ranges in given coord
	UWORD uwStartX = uwX >> pManager->ubTileShift;
	UWORD uwEndX = (uwX+uwWidth) >> pManager->ubTileShift;
	UWORD uwStartY = uwY >> pManager->ubTileShift;
	UWORD uwEndY = (uwY+uwHeight) >> pManager->ubTileShift;

	for(uwX = uwStartX; uwX <= uwEndX; ++uwX) {
		for(uwY = uwStartY; uwY <= uwEndY; ++uwY) {
			tileBufferInvalidateTile(pManager, uwX, uwY);
		}
	}
}

void tileBufferInvalidateTile(
	tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY
) {
	// FIXME it ain't working right yet
	// if(!tileBufferIsTileOnBuffer(pManager, uwTileX, uwTileY)) {
	// 	return;
	// }

	// Previous state will have one more tile to draw, so only check smaller range
	// omit if not yet drawn on redraw margin - let manager draw it when it
	// will be that tile's turn
	// const tRedrawState *pState = &pManager->pRedrawStates[pManager->ubStateIdx];
	// if(
	// 	pState->pMarginX->wTilePos == uwTileX &&
	// 	pState->pMarginX->wTileCurr <= uwTileY &&
	// 	uwTileY <= pState->pMarginX->wTileEnd
	// ) {
	// 	return;
	// }
	// if(
	// 	pState->pMarginY->wTilePos == uwTileY &&
	// 	pState->pMarginY->wTileCurr <= uwTileX &&
	// 	uwTileX <= pState->pMarginY->wTileEnd
	// ) {
	// 	return;
	// }

	// Add to queue
	tileBufferQueueAdd(pManager, uwTileX, uwTileY);
}

UBYTE tileBufferIsTileOnBuffer(
	const tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY
) {
	UBYTE ubTileShift = pManager->ubTileShift;
	UWORD uwStartX = MAX(0, pManager->pCamera->uPos.uwX - 1) >> ubTileShift;
	UWORD uwEndX = (pManager->pCamera->uPos.uwX + pManager->sCommon.pVPort->uwWidth) >> ubTileShift;
	UWORD uwStartY = MAX(0, pManager->pCamera->uPos.uwY - 1) >> ubTileShift;
	UWORD uwEndY = (pManager->pCamera->uPos.uwY + pManager->sCommon.pVPort->uwHeight) >> ubTileShift;

	UBYTE isOnBuffer = (
		uwStartX <= uwTileX && uwTileX <= uwEndX &&
		uwStartY <= uwTileY && uwTileY <= uwEndY
	);
	return isOnBuffer;
}

UBYTE tileBufferIsRectFullyOnBuffer(
	const tTileBufferManager *pManager, UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight
) {
	UBYTE ubTileShift = pManager->ubTileShift;
	UWORD uwStartX = MAX(0, pManager->pCamera->uPos.uwX - 1) >> ubTileShift;
	UWORD uwEndX = (pManager->pCamera->uPos.uwX + pManager->sCommon.pVPort->uwWidth) >> ubTileShift;
	UWORD uwStartY = MAX(0, pManager->pCamera->uPos.uwY - 1) >> ubTileShift;
	UWORD uwEndY = (pManager->pCamera->uPos.uwY + pManager->sCommon.pVPort->uwHeight) >> ubTileShift;

	UWORD uwX1 = uwX >> ubTileShift;
	UWORD uwY1 = uwY >> ubTileShift;
	UWORD uwX2 = (uwX + uwWidth) >> ubTileShift;
	UWORD uwY2 = (uwY + uwHeight) >> ubTileShift;
	UBYTE isOnBuffer = (
		uwStartX <= uwX1 && uwX2 <= uwEndX &&
		uwStartY <= uwY1 && uwY2 <= uwEndY
	);
	return isOnBuffer;
}

void tileBufferSetTile(
	tTileBufferManager *pManager, UWORD uwX, UWORD uwY, tTileBufferTileIndex Index
) {
 	pManager->pTileData[uwX][uwY] = Index;
	tileBufferInvalidateTile(pManager, uwX, uwY);
}
