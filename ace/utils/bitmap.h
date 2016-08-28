#ifndef GUARD_ACE_UTIL_BITMAP_H
#define GUARD_ACE_UTIL_BITMAP_H

#include <stdio.h> // FILE etc

#include <clib/exec_protos.h> // Amiga typedefs
#include <clib/graphics_protos.h> // BitMap etc

#include <ace/config.h>
#include <ace/managers/log.h>
#include <ace/managers/memory.h>
#include <ace/utils/custom.h>

/* Types */
typedef struct BitMap tBitMap;

/* Globals */

/* Functions */

/**
 *  @brief Creates new bitmap of specified dimensions and depth.
 *  
 *  @param uwWidth  Bitmap width.
 *  @param uwHeight Bitmap height.
 *  @param ubDepth  Bitmap depth - bits per pixel.
 *  @param ubFlags  Bitmap flags - compatible with BMF_* from OS functions.
 *  @return Pointer to newly created bitmap, 0 on error.
 *  
 *  @see bitmapCreateFromFile()
 *  @see bitmapDestroy()
 */
tBitMap* bitmapCreate(
	IN UWORD uwWidth,
	IN UWORD uwHeight,
	IN UBYTE ubDepth,
	IN UBYTE ubFlags
);

/**
 *  @brief Creates new bitmap from bitmap file (.bm).
 *  
 *  @param szFileName File to be read from.
 *  @return Pointer to newly created bitmap, 0 on error.
 *  
 *  @see bitmapCreate()
 */
tBitMap* bitmapCreateFromFile(
	IN char *szFileName
);

/**
 *  @brief Destroys supplied bitmap.
 *  
 *  @param pBitMap Bitmap to be destroyed.
 *  
 *  @see bitmapCreate()
 *  @see bitmapCreateFromFile()
 */
void bitmapDestroy(
	IN tBitMap *pBitMap
);

/**
 *  @brief Checks whether supplied bitmap is interleaved
 *  
 *  @param pBitMap Bitmap to be checked.
 *  @return Zero when bitmap is not interleaved, otherwise non-zero.
 */
inline BYTE bitmapIsInterleaved(
	IN tBitMap *pBitMap
);

/**
 *  @brief Dumps bitmap information to log file.
 *  
 *  @param pBitMap Bitmap to be dumped.
 */
void bitmapDump(
	IN tBitMap *pBitMap
);

/**
 *  @brief Saves bitmap as .bmp file.
 *  This function should be used mainly for debug purposes.
 *  
 *  @param pBitMap    Bitmap to be saved.
 *  @param pPalette   Palette to be used during bitmap save.
 *  @param szFileName Destination file path.
 */
void bitmapSaveBMP(
	IN tBitMap *pBitMap,
	IN UWORD *pPalette,
	IN char *szFileName
);

#endif