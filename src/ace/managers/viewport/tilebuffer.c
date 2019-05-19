/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/viewport/tilebuffer.h>
#include <ace/macros.h>
#include <ace/managers/blit.h>
#include <ace/utils/tag.h>

static UBYTE shiftFromPowerOfTwo(UWORD uwPot) {
	UBYTE ubPower = 0;
	while(uwPot > 1) {
		uwPot >>= 1;
		++ubPower;
	}
	return ubPower;
}

#ifdef AMIGA

static void tileBufferQueueAdd(
	tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY
) {
	tRedrawState *pState = &pManager->pRedrawStates[0];
	pState->pPendingQueue[pState->ubPendingCount].sUwCoord.uwX = uwTileX;
	pState->pPendingQueue[pState->ubPendingCount].sUwCoord.uwY = uwTileY;
	++pState->ubPendingCount;
	pState = &pManager->pRedrawStates[1];
	pState->pPendingQueue[pState->ubPendingCount].sUwCoord.uwX = uwTileX;
	pState->pPendingQueue[pState->ubPendingCount].sUwCoord.uwY = uwTileY;
	++pState->ubPendingCount;
}

void tileBufferQueueProcess(tTileBufferManager *pManager) {
	tRedrawState *pState = &pManager->pRedrawStates[pManager->ubStateIdx];
	if(pState->ubPendingCount) {
		--pState->ubPendingCount;
		UBYTE ubPendingCount = pState->ubPendingCount;
		const tUwCoordYX *pTile = &pState->pPendingQueue[ubPendingCount];
		tileBufferDrawTile(pManager, pTile->sUwCoord.uwX, pTile->sUwCoord.uwY);
	}
}

tTileBufferManager *tileBufferCreate(void *pTags, ...) {
	va_list vaTags;
	tTileBufferManager *pManager;
	UWORD uwTileX, uwTileY;
	UBYTE ubBitmapFlags, isDblBuf;

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
	tileBufferReset(pManager, uwTileX, uwTileY, ubBitmapFlags, isDblBuf);

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
	for(uwCol = pManager->uTileBounds.sUwCoord.uwX; uwCol--;) {
		memFree(pManager->pTileData[uwCol], pManager->uTileBounds.sUwCoord.uwY * sizeof(UBYTE));
	}
	memFree(pManager->pTileData, pManager->uTileBounds.sUwCoord.uwX * sizeof(UBYTE *));

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
	UBYTE ubBitmapFlags, UBYTE isDblBuf
) {
	logBlockBegin(
		"tileBufferReset(pManager: %p, uwTileX: %hu, uwTileY: %hu, ubBitmapFlags: %hhu, isDblBuf: %hhu)",
		pManager, uwTileX, uwTileY, ubBitmapFlags, isDblBuf
	);

	// Free old tile data
	if(pManager->pTileData) {
		for(UWORD uwCol = pManager->uTileBounds.sUwCoord.uwX; uwCol--;) {
			memFree(pManager->pTileData[uwCol], pManager->uTileBounds.sUwCoord.uwY * sizeof(UBYTE));
		}
		memFree(pManager->pTileData, pManager->uTileBounds.sUwCoord.uwX * sizeof(UBYTE *));
		pManager->pTileData = 0;
	}

	// Init new tile data
	pManager->uTileBounds.sUwCoord.uwX = uwTileX;
	pManager->uTileBounds.sUwCoord.uwY = uwTileY;
	if(uwTileX && uwTileY) {
		pManager->pTileData = memAllocFast(uwTileX * sizeof(UBYTE*));
		for(UWORD uwCol = uwTileX; uwCol--;) {
			pManager->pTileData[uwCol] = memAllocFastClear(uwTileY * sizeof(UBYTE));
		}
	}

	// Reset margin redraw structs
	UBYTE ubTileShift = pManager->ubTileShift;
	memset(&pManager->pRedrawStates[0], 0, sizeof(tRedrawState));
	memset(&pManager->pRedrawStates[1], 0, sizeof(tRedrawState));
	pManager->pRedrawStates[0].pMarginX         = &pManager->pRedrawStates[0].sMarginR;
	pManager->pRedrawStates[0].pMarginOppositeX = &pManager->pRedrawStates[0].sMarginL;
	pManager->pRedrawStates[0].pMarginY         = &pManager->pRedrawStates[0].sMarginD;
	pManager->pRedrawStates[0].pMarginOppositeY = &pManager->pRedrawStates[0].sMarginU;
	pManager->pRedrawStates[1].pMarginX         = &pManager->pRedrawStates[1].sMarginR;
	pManager->pRedrawStates[1].pMarginOppositeX = &pManager->pRedrawStates[1].sMarginL;
	pManager->pRedrawStates[1].pMarginY         = &pManager->pRedrawStates[1].sMarginD;
	pManager->pRedrawStates[1].pMarginOppositeY = &pManager->pRedrawStates[1].sMarginU;

	// Reset scrollManager, create if not exists
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
		TAG_DONE);
	}
	else {
		scrollBufferReset(
			pManager->pScroll, pManager->ubTileSize,
			uwTileX << ubTileShift, uwTileY << ubTileShift, ubBitmapFlags, isDblBuf
		);
	}

	// Scrollin on one of dirs may be disabled - less redraw on other axis margin
	pManager->uwMarginedWidth = bitmapGetByteWidth(pManager->pScroll->pFront)*8;
	pManager->uwMarginedHeight = pManager->pScroll->uwBmAvailHeight;
	pManager->ubWidthShift = shiftFromPowerOfTwo(pManager->uwMarginedWidth);
	pManager->ubMarginXLength = MIN(
		pManager->uTileBounds.sUwCoord.uwY,
		(pManager->sCommon.pVPort->uwHeight >> ubTileShift) + 4
	);
	pManager->ubMarginYLength = MIN(
		pManager->uTileBounds.sUwCoord.uwX,
		(pManager->sCommon.pVPort->uwWidth >> ubTileShift) + 4
	);
	logWrite(
		"Margin sizes: %hhu,%hhu\n",
		pManager->ubMarginXLength, pManager->ubMarginYLength
	);

	logBlockEnd("tileBufferReset()");
}

FN_HOTSPOT
void tileBufferProcess(tTileBufferManager *pManager) {
	WORD wTileIdxX, wTileIdxY;
	UWORD uwTileOffsX, uwTileOffsY;
	UBYTE ubAddY;
	tRedrawState *pState = &pManager->pRedrawStates[pManager->ubStateIdx];

	UBYTE ubTileSize = pManager->ubTileSize;
	UBYTE ubTileShift = pManager->ubTileShift;
	WORD wDeltaX = cameraGetDeltaX(pManager->pCamera);
	WORD wDeltaY = cameraGetDeltaY(pManager->pCamera);
	// X movement
	if (wDeltaX) {
		// determine movement direction - right or left
		if (wDeltaX > 0) {
			wTileIdxX = ((
				pManager->pCamera->uPos.sUwCoord.uwX + pManager->sCommon.pVPort->uwWidth
			) >> ubTileShift) +1; // delete +1 to see redraw
			pState->pMarginX = &pState->sMarginR;
			pState->pMarginOppositeX = &pState->sMarginL;
		}
		else {
			wTileIdxX = (pManager->pCamera->uPos.sUwCoord.uwX >> ubTileShift) -1;
			pState->pMarginX = &pState->sMarginL;
			pState->pMarginOppositeX = &pState->sMarginR;
		}
		// Not redrawing same column on movement side?
		if (wTileIdxX != pState->pMarginX->wTilePos) {
			// Not finished redrawing all column tiles?
			if(pState->pMarginX->wTileCurr < pState->pMarginX->wTileEnd) {
				uwTileOffsY = (pState->pMarginX->wTileCurr << ubTileShift) & (pManager->uwMarginedHeight-1);
				ubAddY =      (pState->pMarginX->wTilePos << ubTileShift) / pManager->uwMarginedWidth;
				uwTileOffsX = (pState->pMarginX->wTilePos << ubTileShift) & (pManager->uwMarginedWidth-1);
				// Redraw remaining tiles
				while (pState->pMarginX->wTileCurr < pState->pMarginX->wTileEnd) {
					tileBufferDrawTileQuick(
						pManager,
						pState->pMarginX->wTilePos, pState->pMarginX->wTileCurr,
						uwTileOffsX, uwTileOffsY+ubAddY
					);
					++pState->pMarginX->wTileCurr;
					uwTileOffsY = (uwTileOffsY + ubTileSize);
					if(uwTileOffsY >= pManager->uwMarginedHeight) {
						uwTileOffsY -= pManager->uwMarginedHeight;
					}
				}
			}
			// Prepare new column redraw data
			pState->pMarginX->wTilePos = wTileIdxX;
			if (wTileIdxX < 0 || wTileIdxX >= pManager->uTileBounds.sUwCoord.uwX) {
				// Don't redraw if new column is out of map bounds
				pState->pMarginX->wTileCurr = 0;
				pState->pMarginX->wTileEnd = 0;
			}
			else {
				// Prepare new column for redraw
				pState->pMarginX->wTileCurr = MAX(
					0, (pManager->pCamera->uPos.sUwCoord.uwY >> ubTileShift) - 2
				);
				pState->pMarginX->wTileEnd = MIN(
					pState->pMarginX->wTileCurr + pManager->ubMarginXLength,
					pManager->uTileBounds.sUwCoord.uwY
				);
			}
			// Modify margin data on opposite side
			if(wDeltaX < 0) {
				--pState->pMarginOppositeX->wTileCurr;
			}
			else {
				++pState->pMarginOppositeX->wTileCurr;
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

	// Y movement
	if (wDeltaY) {
		// determine redraw row - down or up
		if (wDeltaY > 0) {
			wTileIdxY = ((
				pManager->pCamera->uPos.sUwCoord.uwY + pManager->sCommon.pVPort->uwHeight
			) >> ubTileShift) + 1; // Delete +1 to see redraw
			pState->pMarginY = &pState->sMarginD;
			pState->pMarginOppositeY = &pState->sMarginU;
		}
		else {
			wTileIdxY = (pManager->pCamera->uPos.sUwCoord.uwY >> ubTileShift) -1;
			pState->pMarginY = &pState->sMarginU;
			pState->pMarginOppositeY = &pState->sMarginD;
		}
		// Not drawing same row?
		if (wTileIdxY != pState->pMarginY->wTilePos) {
			// Not finished redrawing all row tiles?
			if(pState->pMarginY->wTileCurr < pState->pMarginY->wTileEnd) {
				uwTileOffsY = (pState->pMarginY->wTilePos << ubTileShift) & (pManager->uwMarginedHeight-1);
				ubAddY =      (pState->pMarginY->wTileCurr << ubTileShift) >> pManager->ubWidthShift;
				uwTileOffsX = (pState->pMarginY->wTileCurr << ubTileShift) & (pManager->uwMarginedWidth-1);
				// Redraw remaining tiles
				while(pState->pMarginY->wTileCurr < pState->pMarginY->wTileEnd) {
					tileBufferDrawTileQuick(
						pManager,
						pState->pMarginY->wTileCurr, pState->pMarginY->wTilePos,
						uwTileOffsX, uwTileOffsY+ubAddY
					);
					++pState->pMarginY->wTileCurr;
					uwTileOffsX += ubTileSize;
					if(uwTileOffsX >= pManager->uwMarginedWidth) {
						uwTileOffsX -= pManager->uwMarginedWidth;
						++ubAddY;
					}
				}
			}
			// Prepare new row redraw data
			pState->pMarginY->wTilePos = wTileIdxY;
			if (wTileIdxY < 0 || wTileIdxY >= pManager->uTileBounds.sUwCoord.uwY) {
				// Don't redraw if new row is out of map bounds
				pState->pMarginY->wTileCurr = 0;
				pState->pMarginY->wTileEnd = 0;
			}
			else {
				// Prepare new row for redraw
				pState->pMarginY->wTileCurr = MAX(
					0, (pManager->pCamera->uPos.sUwCoord.uwX >> ubTileShift) - 2
				);
				pState->pMarginY->wTileEnd = MIN(
					pState->pMarginY->wTileCurr + pManager->ubMarginYLength,
					pManager->uTileBounds.sUwCoord.uwX
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
	pManager->ubStateIdx = !pManager->ubStateIdx;
}

void tileBufferInitialDraw(const tTileBufferManager *pManager) {
	logBlockBegin("tileBufferInitialDraw(pManager: %p)", pManager);
	UBYTE ubTileSize = pManager->ubTileSize;
	UBYTE ubTileShift = pManager->ubTileShift;

	WORD wStartY = MAX(
		0, (pManager->pCamera->uPos.sUwCoord.uwY >> ubTileShift) -1
	);
	WORD wStartX = MAX(
		0, (pManager->pCamera->uPos.sUwCoord.uwX >> ubTileShift) -1
	);
	// One of bounds may be smaller than viewport + margin size
	UWORD uwEndX = MIN(
		pManager->uTileBounds.sUwCoord.uwX,
		pManager->uwMarginedWidth >> ubTileShift
	);
	UWORD uwEndY = MIN(
		pManager->uTileBounds.sUwCoord.uwY,
		pManager->uwMarginedHeight >> ubTileShift
	);

	UWORD uwTileOffsY = (wStartY << ubTileShift) & (pManager->uwMarginedHeight - 1);
		for (UWORD uwTileY = wStartY; uwTileY < uwEndY; ++uwTileY) {
		UBYTE ubAddY =      (wStartX << ubTileShift) >> pManager->ubWidthShift;
		UWORD uwTileOffsX = (wStartX << ubTileShift) & (pManager->uwMarginedWidth-1);

		for (UWORD uwTileX = wStartX; uwTileX < uwEndX; ++uwTileX) {
			tileBufferDrawTileQuick(
				pManager, uwTileX, uwTileY, uwTileOffsX, uwTileOffsY+ubAddY
			);
			uwTileOffsX += ubTileSize;
			if(uwTileOffsX >= pManager->uwMarginedWidth) {
				++ubAddY;
				uwTileOffsX -= pManager->uwMarginedWidth;
			}
		}
		uwTileOffsY = (uwTileOffsY + ubTileSize) & (pManager->uwMarginedHeight - 1);
	}

	// Copy from back buffer to front buffer
	CopyMemQuick(
		pManager->pScroll->pBack->Planes[0], pManager->pScroll->pFront->Planes[0],
		pManager->pScroll->pFront->BytesPerRow * pManager->pScroll->pFront->Rows
	);

	// Refresh bitplane pointers in scrollBuffer's copprtlist
	scrollBufferProcess(pManager->pScroll);

	logBlockEnd("tileBufferInitialDraw()");
}

void tileBufferDrawTile(
	const tTileBufferManager *pManager, UWORD uwTileIdxX, UWORD uwTileIdxY
) {
	UWORD uwBfrX, uwBfrY;
	UBYTE ubAddY;

	uwBfrY = (uwTileIdxY << pManager->ubTileShift) & (pManager->uwMarginedHeight - 1);
	ubAddY = (uwTileIdxX << pManager->ubTileShift) >> pManager->ubWidthShift;
	uwBfrX = (uwTileIdxX << pManager->ubTileShift) & (pManager->uwMarginedWidth-1);

	tileBufferDrawTileQuick(
		pManager, uwTileIdxX, uwTileIdxY, uwBfrX, uwBfrY+ubAddY
	);
}

void tileBufferDrawTileQuick(
	const tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY,
	UWORD uwBfrX, UWORD uwBfrY
) {
	blitCopyAligned(
		pManager->pTileSet,
		0, pManager->pTileData[uwTileX][uwTileY] << pManager->ubTileShift,
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
	UWORD uwStartX = MAX(0, (pManager->pCamera->uPos.sUwCoord.uwX >> ubTileShift) -1);
	UWORD uwEndX = uwStartX + ((pManager->uwMarginedWidth >> ubTileShift) - 2);
	UWORD uwStartY = MAX(0, (pManager->pCamera->uPos.sUwCoord.uwY >> ubTileShift) -1);
	UWORD uwEndY = uwStartY + ((pManager->uwMarginedHeight >> ubTileShift) - 2);

	if(
		uwStartX <= uwTileX && uwTileX <= uwEndX && uwTileX &&
		uwStartY <= uwTileY && uwTileY <= uwEndY && uwTileY
	) {
		return 1;
	}
	return 0;
}

void tileBufferSetTile(
	tTileBufferManager *pManager, UWORD uwX, UWORD uwY, UWORD uwIdx
) {
 	pManager->pTileData[uwX][uwY] = uwIdx;
	tileBufferInvalidateTile(pManager, uwX, uwY);
}

#endif // AMIGA
