/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_VIEWPORT_TILEBUFFER_H_
#define _ACE_MANAGERS_VIEWPORT_TILEBUFFER_H_

#ifdef AMIGA

/**
 * Tilemap buffer manager
 * Provides memory-efficient tilemap buffer
 * Redraws only 1-tile margin beyond viewport in all dirs
 * Author: KaiN
 * Requires viewport managers:
 * 	- camera
 * 	- scroll
 */

#include <ace/types.h>
#include <ace/macros.h>
#include <ace/types.h>

#include <ace/utils/extview.h>
#include <ace/utils/bitmap.h>

#include <ace/managers/blit.h>
#include <ace/managers/viewport/camera.h>
#include <ace/managers/viewport/scrollbuffer.h>

/* types */

typedef void (*tCbTileDraw)(
	UWORD uwTileX, UWORD uwTileY,
	tBitMap *pBitMap, UWORD uwBitMapX, UWORD uwBitMapY
);

typedef struct {
	WORD wTileOffs; /// Index of row/col to update
	WORD wTileCurr; /// Index of current tile to update in row/col
	WORD wTileEnd;  /// Index of last+1  tile to update in row/col
} tTileMarginData;

typedef struct {
	tVpManager sCommon;
	tCameraManager *pCameraManager;       /// Quick ref to Camera
	tScrollBufferManager *pScrollManager; /// Quick ref to Scroll
	                                      // Manager vars
	tUwCoordYX uTileBounds;               /// Tile count in x,y
	UBYTE ubTileSize;                     /// Tile size in pixels
	UBYTE ubTileShift;                    /// Tile size in shift, e.g. 4 for 16: 1 << 4 == 16
	UWORD uwMarginedWidth;                /// Width of visible area + margins
	UWORD uwMarginedHeight;               /// Height of visible area + margins
	                                      /// TODO: refresh when scrollbuffer changes
	tCbTileDraw cbTileDraw;              /// Called when tile is redrawn
	UBYTE **pTileData;                    /// 2D array of tile indices
	tBitMap *pTileSet;                    /// Tileset
	                                      // Margin vars
	UBYTE ubMarginXLength;                /// Tile number in margins: left & right
	UBYTE ubMarginYLength;                /// Ditto, up & down
	tTileMarginData sMarginL;             /// Data for left margin
	tTileMarginData sMarginR;             /// Ditto, right
	tTileMarginData sMarginU;             /// Ditto, up
	tTileMarginData sMarginD;             /// Ditto, down
	                                      // Vars needed in Process, reset in Create
	tTileMarginData *pMarginX;            /// Idx of X margin to be redrawn
	tTileMarginData *pMarginOppositeX;    /// Opposite margin of pMarginX
	tTileMarginData *pMarginY;            /// Idx of Y margin to be redrawn
	tTileMarginData *pMarginOppositeY;    /// Opposite margin of pMarginY
} tTileBufferManager;

/* globals */

/* functions */

tTileBufferManager *tileBufferCreate(
	tVPort *pVPort,
	UWORD uwTileX, UWORD uwTileY, UBYTE ubTileShift,
	char *szTileSetFileName, tCbTileDraw cbTileDraw
);

void tileBufferDestroy(tTileBufferManager *pManager);

void tileBufferProcess(tTileBufferManager *pManager);

void tileBufferReset(
	tTileBufferManager *pManager,
	UWORD uwTileX, UWORD uwTileY, char *szTileSetFileName
);

void tileBufferRedraw(tTileBufferManager *pManager);

void tileBufferDrawTile(
	tTileBufferManager *pManager, UWORD uwTileIdxX, UWORD uwTileIdxY
);

void tileBufferDrawTileQuick(
	tTileBufferManager *pManager,
	UWORD uwTileIdxX, UWORD uwTileIdxY, UWORD uwBfrX, UWORD uwBfrY
);

void tileBufferInvalidateRect(
	tTileBufferManager *pManager, UWORD uwX, UWORD uwY,
	UWORD uwWidth, UWORD uwHeight
);

#endif // AMIGA
#endif // _ACE_MANAGERS_VIEWPORT_TILEBUFFER_H_
