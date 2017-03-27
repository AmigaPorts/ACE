#ifndef GUARD_ACE_UTIL_BITMAP_H
#define GUARD_ACE_UTIL_BITMAP_H

#include <stdio.h> // FILE etc

#include <clib/exec_protos.h> // Amiga typedefs
#include <clib/graphics_protos.h> // BitMap etc

#include <ace/config.h>
#include <ace/managers/log.h>
#include <ace/managers/memory.h>
#include <ace/utils/custom.h>

#define BITMAP_INTERLEAVED 1

/* Types */
typedef struct BitMap tBitMap;

/* Globals */

/* Functions */

/**
 *  @brief Allocates bitmap of given dimensions and depth.
 *  OS' AllocBitMap is not present on KS1.3, hence this OS-compatible
 *  implementation.
 *  
 *  @param uwWidth  Desired bitmap width, in pixels.
 *  @param uwHeight Desired bitmap height, in pixels.
 *  @param ubDepth  Desired bitmap depth (bits per pixel)
 *  @param ubFlags  Bitmap creation flags, see BMF_* defines
 *  @return Newly created OS-compatible bitmap, 0 on error.
 *  
 *  @see bitmapDestroy
 *  @see bitmapCreateFromFile
 *  @see bitmapLoadFromFile
 */
tBitMap* bitmapCreate(
	IN UWORD uwWidth,
	IN UWORD uwHeight,
	IN UBYTE ubDepth,
	IN UBYTE ubFlags
);

/**
 *  @brief Loads bitmap data from file to already existing bitmap.
 *  If source is smaller than destination, you can use uwStartX & uwStartY
 *  params to load bitmap on given coords.
 *  
 *  @param pBitMap    Pointer to destination bitmap
 *  @param szFilePath Source bitmap file path.
 *  @param uwStartX   Start X-coordinate on destination bitmap, 8-pixel aligned.
 *  @param uwStartY   Start Y-coordinate on destination bitmap
 *  
 *  @see bitmapCreate
 *  @see bitmapCreateFromFile
 */
void bitmapLoadFromFile(
	IN tBitMap *pBitMap,
	IN char *szFilePath,
	IN UWORD uwStartX,
	IN UWORD uwStartY
);

/**
 *  @brief Creates bitmap and loads its data from file.
 *  As opposed to bitmapLoadFromFile, this function creates bitmap based
 *  on dimensions, BPP & flags stored in file.
 *  
 *  @param szFilePath Source bitmap file path.
 *  @return Pointer to newly created bitmap based on file, 0 on error.
 *  
 *  @see bitmapLoadFromFile
 *  @see bitmapCreate
 *  @see bitmapDestroy
 */
tBitMap* bitmapCreateFromFile(
	IN char *szFileName
);

/**
 *  @brief Destroys given bitmap, freeing its resources to OS.
 *  Be sure to end all blitter & display operations on this bitmap
 *  prior to calling this function.
 *  
 *  @param pBitMap Bitmap to be destroyed.
 *  
 *  @see bitmapCreate
 *  @see bitmapCreateFromFile
 */
void bitmapDestroy(
	IN tBitMap *pBitMap
);

/**
 *  @brief Checks if given bitmap is interleaved.
 *  Detection should work on any OS bitmap.
 *  
 *  @param pBitMap Bitmap to be checked.
 *  @return non-zero if bitmap is interleaved, otherwise zero.
 */
inline BYTE bitmapIsInterleaved(
	IN tBitMap *pBitMap
);

/**
 *  @brief Saves basic Bitmap information to log file.
 *  
 *  @param pBitMap Bitmap to be dumped.
 *  
 *  @see bitmapSaveBMP
 */
void bitmapDump(
	IN tBitMap *pBitMap
);

/**
 *  @brief Saves given Bitmap as BMP file.
 *  Use only for debug purposes, as conversion is outrageously slow.
 *  
 *  @param pBitMap    Bitmap to be dumped.
 *  @param pPalette   Palette to be used during export.
 *  @param szFilePath Destination file path.
 */
void bitmapSaveBmp(
	IN tBitMap *pBitMap,
	IN UWORD *pPalette,
	IN char *szFileName
);

/**
 *  @brief Returns bitmap width in bytes.
 *  Direct check to BytesPerRow may lead to errors as in interleaved mode it
 *  stores value multiplied by bitplane count.
 */
UWORD bitmapGetByteWidth(tBitMap *pBitMap);

#endif