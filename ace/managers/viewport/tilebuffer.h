#ifndef GUARD_ACE_MANAGER_VIEWPORT_TILEBUFFER_H
#define GUARD_ACE_MANAGER_VIEWPORT_TILEBUFFER_H

/**
 * Tilemap buffer manager
 * Provides memory-efficient tilemap buffer
 * Redraws only 1-tile margin beyond viewport in all dirs
 * Author: KaiN
 * Requires viewport managers:
 * 	- camera
 * 	- scroll
 */

#include "types.h"
#include "macros.h"
#include "config.h"

#include "utils/extview.h"
#include "utils/bitmap.h"

#include "managers/blit.h"
#include "managers/viewport/camera.h"
#include "managers/viewport/scrollBuffer.h"

/* types */

typedef void (*fnTileDrawCallback)(UWORD uwTileX, UWORD uwTileY, struct BitMap *pBitMap, UWORD uwBitMapX, UWORD uwBitMapY);

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
	UWORD uwMarginedHeight;               /// Height of visible area + margins - TODO: refresh when scrollbuffer changes
	fnTileDrawCallback pTileDrawCallback; /// Called when tile is redrawn
	UBYTE **pTileData;                    /// 2D array of tile indices
	struct BitMap *pTileSet;              /// Tileset
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
	IN tVPort *pVPort,
	IN UWORD uwTileX,
	IN UWORD uwTileY,
	// IN UWORD uwCameraX,
	// IN UWORD uwCameraY,
	IN UBYTE ubTileShift,
	IN char *szTileSetFileName,
	IN fnTileDrawCallback pTileDrawCallback
);

void tileBufferDestroy(
	IN tTileBufferManager *pManager
);

void tileBufferProcess(
	IN tTileBufferManager *pManager
);

void tileBufferReset(
	IN tTileBufferManager *pManager,
	IN UWORD uwTileX,
	IN UWORD uwTileY,
	// IN UWORD uwCameraX,
	// IN UWORD uwCameraY,
	IN char *szTileSetFileName
);

void tileBufferRedraw(
	IN tTileBufferManager *pManager
);

void tileBufferDrawTile(
	IN tTileBufferManager *pManager,
	IN UWORD uwTileIdxX,
	IN UWORD uwTileIdxY
);

void tileBufferDrawTileQuick(
	IN tTileBufferManager *pManager,
	IN UWORD uwTileIdxX,
	IN UWORD uwTileIdxY,
	IN UWORD uwBfrX,
	IN UWORD uwBfrY
);

void tileBufferInvalidateRect(
	IN tTileBufferManager *pManager,
	IN UWORD uwX,
	IN UWORD uwY,
	IN UWORD uwWidth,
	IN UWORD uwHeight
);

#endif