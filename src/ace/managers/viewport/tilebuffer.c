/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/viewport/tilebuffer.h>
#include <ace/macros.h>
#include <ace/managers/blit.h>

#ifdef AMIGA

tTileBufferManager *tileBufferCreate(
	tVPort *pVPort, UWORD uwTileX, UWORD uwTileY, UBYTE ubTileShift,
	tBitMap *pTileset, tCbTileDraw cbTileDraw
) {
	logBlockBegin(
		"tileBufferCreate(pVPort: %p, uwTileX: %u, uwTileY: %u, ubTileShift: %hu, "
		"pTileset: %p, cbTileDraw: %p)",
		pVPort, uwTileX, uwTileY, ubTileShift, pTileset, cbTileDraw
	);
	tTileBufferManager *pManager;

	// Feed struct with args
	pManager = memAllocFast(sizeof(tTileBufferManager));
	pManager->sCommon.pNext = 0;
	pManager->sCommon.process = (tVpManagerFn)tileBufferProcess;
	pManager->sCommon.destroy = (tVpManagerFn)tileBufferDestroy;
	pManager->sCommon.ubId = VPM_TILEBUFFER;
	pManager->sCommon.pVPort = pVPort;

	pManager->ubTileShift = ubTileShift;
	pManager->ubTileSize = 1 << ubTileShift;
	pManager->cbTileDraw = cbTileDraw;

	pManager->pTileData = 0;
	pManager->pTileSet = 0;
	tileBufferReset(pManager, uwTileX, uwTileY, pTileset);

	vPortAddManager(pVPort, (tVpManager*)pManager);

	// find camera manager, create if not exists
	// camera created in scroll bfr
	pManager->pCameraManager = (tCameraManager*)vPortGetManager(pVPort, VPM_CAMERA);
	// if(!(pManager->pCameraManager = (tCameraManager*)extVPortGetManager(pVPort, VPM_CAMERA))) {
		// pManager->pCameraManager = cameraCreate(pVPort, 0, 0, uwTileX << ubTileShift, uwTileY << ubTileShift);
	// }
	// TODO: reset camera bounds?

	// nie tutaj bo kamera jeszcze musi zosta� ustawiona
	// i pTileData ustawione
	// a bez sensu dwa razy odrysowywa� ca�y ekran
	// tileBufferRedraw(pManager);

	logBlockEnd("tileBufferCreate");
	return pManager;
}

void tileBufferDestroy(tTileBufferManager *pManager) {
	UWORD uwCol;
	logBlockBegin("tileBufferDestroy(pManager: %p)", pManager);

	// Free tileset
	bitmapDestroy(pManager->pTileSet);

	// Free tile data
	for (uwCol = pManager->uTileBounds.sUwCoord.uwX; uwCol--;)
		memFree(pManager->pTileData[uwCol], pManager->uTileBounds.sUwCoord.uwY * sizeof(UBYTE));
	memFree(pManager->pTileData, pManager->uTileBounds.sUwCoord.uwX * sizeof(UBYTE *));

	// Free manager
	memFree(pManager, sizeof(tTileBufferManager));

	logBlockEnd("tileBufferDestroy");
}

void tileBufferReset(
	tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY,
	tBitMap *pTileset
) {
	UWORD uwCol;
	UBYTE ubTileShift;
	logBlockBegin("tileBufferReset()");

	// Free old tile data
	if(pManager->pTileData) {
		for (uwCol = pManager->uTileBounds.sUwCoord.uwX; uwCol--;) {
			memFree(pManager->pTileData[uwCol], pManager->uTileBounds.sUwCoord.uwY * sizeof(UBYTE));
		}
		memFree(pManager->pTileData, pManager->uTileBounds.sUwCoord.uwX * sizeof(UBYTE *));
		pManager->pTileData = 0;
	}

	// Init new tile data
	pManager->uTileBounds.sUwCoord.uwX = uwTileX;
	pManager->uTileBounds.sUwCoord.uwY = uwTileY;
	if(uwTileX && uwTileY) {
		pManager->pTileData = memAllocFast(uwTileX * sizeof(BYTE*));
		for(uwCol = uwTileX; uwCol--;) {
			pManager->pTileData[uwCol] = memAllocFastClear(uwTileY * sizeof(UBYTE));
		}
	}

	// Load new tileset
	pManager->pTileSet = pTileset;

	// Reset margin redraw structs
	ubTileShift = pManager->ubTileShift;
	memset(&pManager->sMarginL, 0, sizeof(tTileMarginData));
	memset(&pManager->sMarginR, 0, sizeof(tTileMarginData));
	memset(&pManager->sMarginU, 0, sizeof(tTileMarginData));
	memset(&pManager->sMarginD, 0, sizeof(tTileMarginData));
	pManager->ubMarginXLength = (pManager->sCommon.pVPort->uwHeight >> ubTileShift) + 4;
	pManager->ubMarginYLength = (pManager->sCommon.pVPort->uwWidth >> ubTileShift) + 4;
	pManager->pMarginX         = &pManager->sMarginR;
	pManager->pMarginOppositeX = &pManager->sMarginL;
	pManager->pMarginY         = &pManager->sMarginD;
	pManager->pMarginOppositeY = &pManager->sMarginU;

	// Reset scrollManager, create if not exists
	pManager->pScrollManager = (tScrollBufferManager*)vPortGetManager(
		pManager->sCommon.pVPort, VPM_SCROLL
	);
	if(!(pManager->pScrollManager)) {
		pManager->pScrollManager = scrollBufferCreate(
			pManager->sCommon.pVPort, pManager->ubTileSize,
			uwTileX << ubTileShift, uwTileY << ubTileShift
		);
	}
	else {
		scrollBufferReset(
			pManager->pScrollManager, pManager->ubTileSize,
			uwTileX << ubTileShift, uwTileY << ubTileShift, BMF_CLEAR, 0
		);
	}

	pManager->uwMarginedWidth = pManager->sCommon.pVPort->uwWidth + (4 << ubTileShift);
	pManager->uwMarginedHeight = pManager->pScrollManager->uwBmAvailHeight;

	logBlockEnd("tileBufferReset()");
}

void tileBufferProcess(tTileBufferManager *pManager) {
	WORD wDeltaX, wDeltaY;
	WORD wTileIdxX, wTileIdxY;
	UWORD uwTileOffsX, uwTileOffsY;
	UBYTE ubTileSize, ubTileShift;
	UBYTE ubAddY;

	ubTileSize = pManager->ubTileSize;
	ubTileShift = pManager->ubTileShift;
	wDeltaX = cameraGetDeltaX(pManager->pCameraManager);
	wDeltaY = cameraGetDeltaY(pManager->pCameraManager);
	// X movement
	if (wDeltaX) {
		// determine movement direction - right or left
		if (wDeltaX > 0) {
			wTileIdxX = ((pManager->pCameraManager->uPos.sUwCoord.uwX + pManager->sCommon.pVPort->uwWidth) >> ubTileShift) +1; // delete +1 to see redraw
			pManager->pMarginX = &pManager->sMarginR;
			pManager->pMarginOppositeX = &pManager->sMarginL;
		}
		else {
			wTileIdxX = (pManager->pCameraManager->uPos.sUwCoord.uwX >> ubTileShift) -1;
			pManager->pMarginX = &pManager->sMarginL;
			pManager->pMarginOppositeX = &pManager->sMarginR;
		}
		// Not redrawing same column on movement side?
		if (wTileIdxX != pManager->pMarginX->wTileOffs) {
			// Not finished redrawing all column tiles?
			if(pManager->pMarginX->wTileCurr < pManager->pMarginX->wTileEnd) {
				uwTileOffsY = (pManager->pMarginX->wTileCurr << ubTileShift) % pManager->uwMarginedHeight;
				ubAddY =      (pManager->pMarginX->wTileOffs << ubTileShift) / pManager->uwMarginedWidth;
				uwTileOffsX = (pManager->pMarginX->wTileOffs << ubTileShift) % pManager->uwMarginedWidth;
				// Redraw remaining tiles
				while (pManager->pMarginX->wTileCurr < pManager->pMarginX->wTileEnd) {
					tileBufferDrawTileQuick(
						pManager,
						pManager->pMarginX->wTileOffs, pManager->pMarginX->wTileCurr,
						uwTileOffsX, uwTileOffsY+ubAddY
					);
					++pManager->pMarginX->wTileCurr;
					uwTileOffsY = (uwTileOffsY + ubTileSize);
					if(uwTileOffsY >= pManager->uwMarginedHeight) {
						uwTileOffsY -= pManager->uwMarginedHeight;
					}
				}
			}
			// Prepare new column redraw data
			pManager->pMarginX->wTileOffs = wTileIdxX;
			if (wTileIdxX < 0 || wTileIdxX >= pManager->uTileBounds.sUwCoord.uwX) {
				// Don't redraw if new column is out of map bounds
				pManager->pMarginX->wTileCurr = 0;
				pManager->pMarginX->wTileEnd = 0;
			}
			else {
				// Prepare new column for redraw
				pManager->pMarginX->wTileCurr = (pManager->pCameraManager->uPos.sUwCoord.uwY >> ubTileShift) - 2;
				pManager->pMarginX->wTileEnd = pManager->pMarginX->wTileCurr + pManager->ubMarginXLength;
				if(pManager->pMarginX->wTileCurr < 0) {
					pManager->pMarginX->wTileCurr = 0;
				}
			}
			// Modify margin data on opposite side
			if(wDeltaX < 0) {
				--pManager->pMarginOppositeX->wTileCurr;
			}
			else {
				++pManager->pMarginOppositeX->wTileCurr;
			}
			pManager->pMarginOppositeX->wTileCurr = 0;
			pManager->pMarginOppositeX->wTileEnd = 0;
		}
	}

	// Redraw another X tile - regardless of movement in that direction
	if (pManager->pMarginX->wTileCurr < pManager->pMarginX->wTileEnd) {
		uwTileOffsX = (pManager->pMarginX->wTileOffs << ubTileShift) % pManager->uwMarginedWidth;
		uwTileOffsY = (pManager->pMarginX->wTileCurr << ubTileShift) % pManager->uwMarginedHeight;
		ubAddY = (pManager->pMarginX->wTileOffs << ubTileShift) / pManager->uwMarginedWidth;
		tileBufferDrawTileQuick(
			pManager,
			pManager->pMarginX->wTileOffs, pManager->pMarginX->wTileCurr,
			uwTileOffsX, uwTileOffsY+ubAddY
		);
		++pManager->pMarginX->wTileCurr;
	}

	// Y movement
	if (wDeltaY) {
		// determine redraw row - down or up
		if (wDeltaY > 0) {
			wTileIdxY = ((pManager->pCameraManager->uPos.sUwCoord.uwY + pManager->sCommon.pVPort->uwHeight) >> ubTileShift) +1; // Delete +1 to see redraw
			pManager->pMarginY = &pManager->sMarginD;
			pManager->pMarginOppositeY = &pManager->sMarginU;
		}
		else {
			wTileIdxY = (pManager->pCameraManager->uPos.sUwCoord.uwY >> ubTileShift) -1;
			pManager->pMarginY = &pManager->sMarginU;
			pManager->pMarginOppositeY = &pManager->sMarginD;
		}
		// Not drawing same row?
		if (wTileIdxY != pManager->pMarginY->wTileOffs) {
			// Not finished redrawing all row tiles?
			if(pManager->pMarginY->wTileCurr < pManager->pMarginY->wTileEnd) {
				uwTileOffsY = (pManager->pMarginY->wTileOffs << ubTileShift) % pManager->uwMarginedHeight;
				ubAddY =      (pManager->pMarginY->wTileCurr << ubTileShift) / pManager->uwMarginedWidth;
				uwTileOffsX = (pManager->pMarginY->wTileCurr << ubTileShift) % pManager->uwMarginedWidth;
				// Redraw remaining tiles
				while(pManager->pMarginY->wTileCurr < pManager->pMarginY->wTileEnd) {
					tileBufferDrawTileQuick(
						pManager,
						pManager->pMarginY->wTileCurr, pManager->pMarginY->wTileOffs,
						uwTileOffsX, uwTileOffsY+ubAddY
					);
					++pManager->pMarginY->wTileCurr;
					uwTileOffsX += ubTileSize;
					if(uwTileOffsX >= pManager->uwMarginedWidth) {
						uwTileOffsX -= pManager->uwMarginedWidth;
						++ubAddY;
					}
				}
			}
			// Prepare new row redraw data
			pManager->pMarginY->wTileOffs = wTileIdxY;
			if (wTileIdxY < 0 || wTileIdxY >= pManager->uTileBounds.sUwCoord.uwY) {
				// Don't redraw if new row is out of map bounds
				pManager->pMarginY->wTileCurr = 0;
				pManager->pMarginY->wTileEnd = 0;
			}
			else {
				// Prepare new row for redraw
				pManager->pMarginY->wTileCurr = (pManager->pCameraManager->uPos.sUwCoord.uwX >> ubTileShift) - 2;
				pManager->pMarginY->wTileEnd = pManager->pMarginY->wTileCurr + pManager->ubMarginYLength;
				if(pManager->pMarginY->wTileCurr < 0) {
					pManager->pMarginY->wTileCurr = 0;
				}
				if(pManager->pMarginY->wTileEnd >= pManager->uTileBounds.sUwCoord.uwX) {
					pManager->pMarginY->wTileEnd = pManager->uTileBounds.sUwCoord.uwX-1;
				}
			}
			// Modify opposite margin data
			if(wDeltaY > 0) {
				++pManager->pMarginOppositeY->wTileOffs;
			}
			else {
				--pManager->pMarginOppositeY->wTileOffs;
			}
			pManager->pMarginOppositeY->wTileCurr = 0;
			pManager->pMarginOppositeY->wTileEnd = 0;
		}
	}

	// Redraw another Y tile - regardless of movement in that direction
	if (pManager->pMarginY->wTileCurr < pManager->pMarginY->wTileEnd) {
		ubAddY =      (pManager->pMarginY->wTileCurr << ubTileShift) / pManager->uwMarginedWidth;
		uwTileOffsX = (pManager->pMarginY->wTileCurr << ubTileShift) % pManager->uwMarginedWidth;
		uwTileOffsY = (pManager->pMarginY->wTileOffs << ubTileShift) % pManager->uwMarginedHeight;
		tileBufferDrawTileQuick(
			pManager,
			pManager->pMarginY->wTileCurr, pManager->pMarginY->wTileOffs,
			uwTileOffsX, uwTileOffsY+ubAddY
		);
		++pManager->pMarginY->wTileCurr;
	}
}

void tileBufferRedraw(tTileBufferManager *pManager) {
	UWORD i,j;
	UWORD uwTileOffsX, uwTileOffsY;
	WORD wTileIdxX, wTileIdxY;
	UBYTE ubAddY;
	UBYTE ubTileSize, ubTileShift;

	logBlockBegin("tileBufferRedraw(pManager: %p)", pManager);
	ubTileSize = pManager->ubTileSize;
	ubTileShift = pManager->ubTileShift;
	pManager->uwMarginedWidth = pManager->sCommon.pVPort->uwWidth + (4 << ubTileShift);
	pManager->uwMarginedHeight = pManager->pScrollManager->uwBmAvailHeight;

	wTileIdxY = (pManager->pCameraManager->uPos.sUwCoord.uwY >> ubTileShift) -1;
	if (wTileIdxY < 0)
		wTileIdxY = 0;
	uwTileOffsY = (wTileIdxY << ubTileShift) % pManager->uwMarginedHeight;

	for (j = 0; j < pManager->uwMarginedHeight; j += ubTileSize) {

		wTileIdxX = (pManager->pCameraManager->uPos.sUwCoord.uwX >> ubTileShift) -1;
		if (wTileIdxX < 0)
			wTileIdxX = 0;
		ubAddY =      (wTileIdxX << ubTileShift) / pManager->uwMarginedWidth;
		uwTileOffsX = (wTileIdxX << ubTileShift) % pManager->uwMarginedWidth;

		for (i = 0; i < pManager->uwMarginedWidth; i+= ubTileSize) {
			tileBufferDrawTileQuick(pManager, wTileIdxX, wTileIdxY, uwTileOffsX, uwTileOffsY+ubAddY);
			++wTileIdxX;
			uwTileOffsX += ubTileSize;
			if(uwTileOffsX >= pManager->uwMarginedWidth) {
				++ubAddY;
				uwTileOffsX -= pManager->uwMarginedWidth;
			}
		}

		++wTileIdxY;
		uwTileOffsY = (uwTileOffsY + ubTileSize) % pManager->uwMarginedHeight;
	}
	logBlockEnd("tileBufferRedraw()");
}

void tileBufferDrawTile(tTileBufferManager *pManager, UWORD uwTileIdxX, UWORD uwTileIdxY) {
	UWORD uwBfrX, uwBfrY;
	UBYTE ubAddY;

	uwBfrY = (uwTileIdxY << pManager->ubTileShift) % pManager->uwMarginedHeight;
	ubAddY = (uwTileIdxX << pManager->ubTileShift) / pManager->uwMarginedWidth;
	uwBfrX = (uwTileIdxX << pManager->ubTileShift) % pManager->uwMarginedWidth;

	tileBufferDrawTileQuick(pManager, uwTileIdxX, uwTileIdxY, uwBfrX, uwBfrY+ubAddY);
}

void tileBufferDrawTileQuick(tTileBufferManager *pManager, UWORD uwTileIdxX, UWORD uwTileIdxY, UWORD uwBfrX, UWORD uwBfrY) {
	blitCopyAligned(
		pManager->pTileSet,
		0, pManager->pTileData[uwTileIdxX][uwTileIdxY] << pManager->ubTileShift,
		pManager->pScrollManager->pBack, uwBfrX, uwBfrY,
		pManager->ubTileSize, pManager->ubTileSize
	);
	if(pManager->cbTileDraw) {
		pManager->cbTileDraw(uwTileIdxX, uwTileIdxY, pManager->pScrollManager->pBack, uwBfrX, uwBfrY);
	}
}

void tileBufferInvalidateRect(tTileBufferManager *pManager, UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight) {
	UWORD uwStartX, uwEndX, uwStartY, uwEndY;             /// Invalidate tile rect
	UWORD uwVisStartX, uwVisEndX, uwVisStartY, uwVisEndY; /// Visible tile rect (excluding invisible margins)
	UWORD uwVisX, uwVisY;
	UBYTE ubAddY;

	// graniczne indeksy kafli
	uwStartX = uwX >> pManager->ubTileShift;
	uwEndX = (uwX+uwWidth) >> pManager->ubTileShift;
	uwStartY = uwY >> pManager->ubTileShift;
	uwEndY = (uwY+uwHeight) >> pManager->ubTileShift;

	uwVisStartX = pManager->pCameraManager->uPos.sUwCoord.uwX >> pManager->ubTileShift;
	uwVisStartY = pManager->pCameraManager->uPos.sUwCoord.uwY >> pManager->ubTileShift;
	uwVisEndX = uwVisStartX + (pManager->sCommon.pVPort->uwWidth >> pManager->ubTileShift);
	uwVisEndY = uwVisStartY + (pManager->sCommon.pVPort->uwHeight >> pManager->ubTileShift);

	for(uwX = uwStartX; uwX <= uwEndX; ++uwX) {
		if(uwX < uwVisStartX) {
			continue;
		}
		if(uwX > uwVisEndX) {
			break;
		}
		ubAddY = (uwX << pManager->ubTileShift) / pManager->uwMarginedWidth;
		uwVisX = (uwX << pManager->ubTileShift) % pManager->uwMarginedWidth;
		uwVisY = (uwStartY << pManager->ubTileShift) % pManager->uwMarginedHeight;
		for(uwY = uwStartY; uwY <= uwEndY; ++uwY) {
			if(uwY < uwVisStartY) {
				continue;
			}
			if(uwY > uwVisEndY) {
				logWrite("\n");
				logWrite("camera Y: %u, uwMarginedHeight: %u\n", pManager->pCameraManager->uPos.sUwCoord.uwY, pManager->uwMarginedHeight);
				logWrite("uwStartY: %u, uwY: %u, uwEndY: %u\n", uwStartY, uwY, uwEndY);
				logWrite("uwVisStartY: %u, uwVisY: %u, uwVisEndY: %u\n", uwVisStartY, uwVisY, uwVisEndY);
				logWrite("uwY > uwVisEndY (%u > %u)\n", uwY, uwVisEndY);
				break;
			}
			tileBufferDrawTileQuick(pManager, uwX, uwY, uwVisX, uwVisY + ubAddY);
			uwVisY += pManager->ubTileSize;
			if(uwVisY >= pManager->uwMarginedHeight) {
				uwVisY -= pManager->uwMarginedHeight;
			}
		}
	}
}

#endif // AMIGA
