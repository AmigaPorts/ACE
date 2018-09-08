/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_VIEWPORT_TILEBUFFER_H_
#define _ACE_MANAGERS_VIEWPORT_TILEBUFFER_H_

#ifdef AMIGA

/**
 * Tilemap buffer manager
 * Provides speed- and memory-efficient tilemap buffer
 * Redraws only 1-tile margin beyond viewport in all dirs
 * Requires viewport managers:
 * 	- camera
 * 	- scroll
 */

#include <ace/types.h>
#include <ace/utils/extview.h>
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

/**
 * Tilemap buffer manager create fn
 * Be sure to set camera pos, load tile data & then call tileBufferRedraw()!
 */
tTileBufferManager *tileBufferCreate(
	tVPort *pVPort, UWORD uwTileX, UWORD uwTileY, UBYTE ubTileShift,
	tBitMap *pTileset, tCbTileDraw cbTileDraw
);

void tileBufferDestroy(tTileBufferManager *pManager);

/**
 * Redraws one tile for each margin - X and Y
 * Redraws all remaining margin's tiles when margin is about to be displayed
 */
void tileBufferProcess(tTileBufferManager *pManager);

void tileBufferReset(
	tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY, tBitMap *pTileset
);

/**
 * Redraws tiles on whole screen
 * Use for init or something like that, as it's slooooooooow
 */
void tileBufferRedraw(tTileBufferManager *pManager);

/**
 * Redraws selected tile, calls custom redraw callback
 * Calculates destination on buffer
 * Use for single redraws
 */
void tileBufferDrawTile(
	tTileBufferManager *pManager, UWORD uwTileIdxX, UWORD uwTileIdxY
);

/**
 * Redraws selected tile, calls custom redraw callback
 * Destination coord on buffer must be calculated externally - avoids recalc
 * Use for batch redraws with smart uwBfrXY update
 */
void tileBufferDrawTileQuick(
	tTileBufferManager *pManager,
	UWORD uwTileIdxX, UWORD uwTileIdxY, UWORD uwBfrX, UWORD uwBfrY
);

/**
 * Redraws all tiles intersecting with given rectangle
 * Only tiles currently on buffer are redrawn
 */
void tileBufferInvalidateRect(
	tTileBufferManager *pManager, UWORD uwX, UWORD uwY,
	UWORD uwWidth, UWORD uwHeight
);

#endif // AMIGA
#endif // _ACE_MANAGERS_VIEWPORT_TILEBUFFER_H_
