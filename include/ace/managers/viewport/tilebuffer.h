/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_VIEWPORT_TILEBUFFER_H_
#define _ACE_MANAGERS_VIEWPORT_TILEBUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

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

// vPort ptr
#define TAG_TILEBUFFER_VPORT (TAG_USER|1)
// Scrollable area bounds, in pixels
#define TAG_TILEBUFFER_BOUND_TILE_X     (TAG_USER|2)
#define TAG_TILEBUFFER_BOUND_TILE_Y     (TAG_USER|3)
#define TAG_TILEBUFFER_TILE_SHIFT (TAG_USER|4)
// Buffer bitmap creation flags
#define TAG_TILEBUFFER_BITMAP_FLAGS (TAG_USER|5)
#define TAG_TILEBUFFER_IS_DBLBUF    (TAG_USER|6)
// If in raw mode, offset on copperlist for placing required copper
// instructions, specified in copper instruction count since beginning.
#define TAG_TILEBUFFER_COPLIST_OFFSET_START (TAG_USER|7)
#define TAG_TILEBUFFER_COPLIST_OFFSET_BREAK (TAG_USER|8)
// Callbacks, tileset
#define TAG_TILEBUFFER_TILESET             (TAG_USER|9)
#define TAG_TILEBUFFER_CALLBACK_TILE_DRAW  (TAG_USER|10)
#define TAG_TILEBUFFER_REDRAW_QUEUE_LENGTH (TAG_USER|11)

/* types */

typedef void (*tTileDrawCallback)(
	UWORD uwTileX, UWORD uwTileY,
	tBitMap *pBitMap, UWORD uwBitMapX, UWORD uwBitMapY
);

typedef struct _tMarginState {
	WORD wTilePos; ///< Index of row/col to update
	WORD wTileCurr; ///< Index of current tile to update in row/col
	WORD wTileEnd;  ///< Index of last+1  tile to update in row/col
} tMarginState;

typedef struct _tRedrawState {
	tMarginState sMarginL; ///< Data for left margin
	tMarginState sMarginR; ///< Ditto, right
	tMarginState sMarginU; ///< Ditto, up
	tMarginState sMarginD; ///< Ditto, down
	// Vars needed in Process, reset in Create
	tMarginState *pMarginX;         ///< Idx of X margin to be redrawn
	tMarginState *pMarginOppositeX; ///< Opposite margin of pMarginX
	tMarginState *pMarginY;         ///< Idx of Y margin to be redrawn
	tMarginState *pMarginOppositeY; ///< Opposite margin of pMarginY
	// Tile redraw queue
	tUwCoordYX *pPendingQueue;
	UBYTE ubPendingCount;
} tRedrawState;

typedef struct _tTileBufferManager {
	tVpManager sCommon;
	tCameraManager *pCamera;       ///< Quick ref to Camera
	tScrollBufferManager *pScroll; ///< Quick ref to Scroll
	// Manager vars
	tUwCoordYX uTileBounds;       ///< Tile count in x,y
	UBYTE ubTileSize;             ///< Tile size in pixels
	UBYTE ubTileShift;            ///< Tile size in shift, e.g. 4 for 16: 1 << 4 == 16
	UWORD uwMarginedWidth;        ///< Width of visible area + margins
	UWORD uwMarginedHeight;       ///< Height of visible area + margins
	                              ///  TODO: refresh when scrollbuffer changes
	tTileDrawCallback cbTileDraw; ///< Called when tile is redrawn
	UBYTE **pTileData;            ///< 2D array of tile indices
	tBitMap *pTileSet;            ///< Tileset - one tile beneath another
	// Margin & queue geometry
	UBYTE ubMarginXLength; ///< Tile number in margins: left & right
	UBYTE ubMarginYLength; ///< Ditto, up & down
	UBYTE ubQueueSize;
	// Redraw state and double buffering
	tRedrawState pRedrawStates[2];
	UBYTE ubStateIdx;
	UBYTE ubWidthShift;
} tTileBufferManager;

/* globals */

/* functions */

void tileBufferQueueProcess(tTileBufferManager *pManager);

/**
 * Tilemap buffer manager create fn
 * Be sure to set camera pos, load tile data & then call tileBufferRedraw()!
 */
tTileBufferManager *tileBufferCreate(void *pTags, ...);

void tileBufferDestroy(tTileBufferManager *pManager);

/**
 * Redraws one tile for each margin - X and Y
 * Redraws all remaining margin's tiles when margin is about to be displayed
 */
void tileBufferProcess(tTileBufferManager *pManager);

void tileBufferReset(
	tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY,
	UBYTE ubBitmapFlags, UBYTE isDblBuf
);

/**
 * Redraws tiles on whole screen.
 * Use for init or something like that, as it's slooooooooow.
 * Be sure to have display turned off or palette dimmed since even on double
 * buffering it will redraw both buffers.
 */
void tileBufferRedrawAll(tTileBufferManager *pManager);

/**
 * Redraws selected tile, calls custom redraw callback
 * Calculates destination on buffer
 * Use for single redraws
 */
void tileBufferDrawTile(
	const tTileBufferManager *pManager, UWORD uwTileIdxX, UWORD uwTileIdxY
);

/**
 * Redraws selected tile, calls custom redraw callback
 * Destination coord on buffer must be calculated externally - avoids recalc
 * Use for batch redraws with smart uwBfrXY update
 */
void tileBufferDrawTileQuick(
	const tTileBufferManager *pManager,
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

void tileBufferInvalidateTile(
	tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY
);

UBYTE tileBufferIsTileOnBuffer(
	const tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY
);

void tileBufferSetTile(
	tTileBufferManager *pManager, UWORD uwX, UWORD uwY, UWORD uwIdx
);

#endif // AMIGA

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_VIEWPORT_TILEBUFFER_H_
