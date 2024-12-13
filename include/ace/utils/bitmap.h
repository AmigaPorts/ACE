/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_BITMAP_H_
#define _ACE_UTILS_BITMAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>
#include <ace/utils/file.h>

// File has its own 'flags' field - could be used in new ACE bitmap struct
#define BITMAP_INTERLEAVED 1
// FEATURE PROPOSAL:
// If set, ubBpp shows how many bitplanes are in bitmap, but after last one
// there is mask attached. Could be useful with c2p transforms - mask could be
// rotated with bitmap. Mask would be attached and detached with
// bitmapAttachMask() and bitmapDetachMask() fns.
#define BITMAP_MASK_ATTACHED 2

/* Types */

#ifdef AMIGA
#include <clib/graphics_protos.h> // BitMap etc
typedef struct BitMap tBitMap;
#else
typedef struct _tBitMap {
	UWORD BytesPerRow;
	UWORD Rows;
	UBYTE Flags;
	UBYTE Depth;
	UWORD pad;
	UWORD *Planes[8];
} tBitMap;
#define BMF_CLEAR       (1 << 0)
#define BMF_DISPLAYABLE (1 << 1)
#define BMF_INTERLEAVED (1 << 2)
#define BMF_STANDARD    (1 << 3)
#define BMF_MINPLANES   (1 << 4)
#endif // AMIGA

/**
 * @brief Stores bitplanes in FAST memory. Can be handy to load extra bitmaps
 * to non-CHIP mem and copy in their contents when needed.
 */
#define BMF_FASTMEM (1 << 5)

/**
 * @brief Stores bitplanes in one contiguous chunk of memory, one after another.
 *  This is needed for some optimized routines, e.g. Kalms C2P. *
 */
#define BMF_CONTIGUOUS (1 << 6)

/**
 * @brief New bitmap format.
 * Don't use until adopted into entire engine - this struct is more like feature
 * request or memo.
 */
typedef struct _tAceBitmap {
	UWORD uwWidth; ///< Actual width, in pixels.
	UWORD uwHeight;
	UWORD uwBytesPerRow; ///< Useful during raw ops, faster than (uwWidth+15)>>3.
	                     /// Perhaps uwWordsPerRow would be more useful?
	UBYTE ubBpp;
	UBYTE ubFlags; ///< Interleaved or not
	UWORD *pPlanes[8]; ///< OCS uses up to 6, AGA up to 8
} tAceBitmap;

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
 *  @see bitmapCreateFromFd
 *  @see bitmapLoadFromFd
 */
tBitMap* bitmapCreate(
	UWORD uwWidth, UWORD uwHeight, UBYTE ubDepth, UBYTE ubFlags
);

/**
 *  @brief Loads bitmap data from file to already existing bitmap.
 *  If source is smaller than destination, you can use uwStartX & uwStartY
 *  params to load bitmap on given coords.
 *
 *  @param pBitMap Pointer to destination bitmap
 *  @param szFilePath Source bitmap file path.
 *  @param uwStartX Start X-coordinate on destination bitmap, 8-pixel aligned.
 *  @param uwStartY Start Y-coordinate on destination bitmap
 *
 *  @see bitmapCreate
 *  @see bitmapCreateFromFd
 *  @see bitmapCreateFromPath
 *  @see bitmapLoadFromFd
 */
void bitmapLoadFromPath(
	tBitMap *pBitMap, const char *szPath, UWORD uwStartX, UWORD uwStartY
);

/**
 *  @brief Loads bitmap data from file to already existing bitmap.
 *  If source is smaller than destination, you can use uwStartX & uwStartY
 *  params to load bitmap on given coords.
 *
 *  @param pBitMap Pointer to destination bitmap
 *  @param pFile Handle to the bitmap file. Will be closed on function return.
 *  @param uwStartX Start X-coordinate on destination bitmap, 8-pixel aligned.
 *  @param uwStartY Start Y-coordinate on destination bitmap
 *
 *  @see bitmapCreate
 *  @see bitmapCreateFromFd
 *  @see bitmapCreateFromPath
 *  @see bitmapLoadFromPath
 */
void bitmapLoadFromFd(
	tBitMap *pBitMap, tFile *pFile, UWORD uwStartX, UWORD uwStartY
);

/**
 *  @brief Creates bitmap and loads its data from file.
 *  As opposed to bitmapLoadFromPath, this function creates bitmap based
 *  on dimensions, BPP & flags stored in file.
 *
 *  @param szFilePath Source bitmap file path.
 *  @param isFast True to allocate bitmap in FAST RAM
 *  @return Pointer to newly created bitmap based on file, 0 on error.
 *
 *  @see bitmapCreateFromFd
 *  @see bitmapLoadFromFd
 *  @see bitmapLoadFromPath
 *  @see bitmapCreate
 *  @see bitmapDestroy
 */
tBitMap* bitmapCreateFromPath(const char *szPath, UBYTE isFast);

/**
 *  @brief Creates bitmap and loads its data from file.
 *  As opposed to bitmapLoadFromFd, this function creates bitmap based
 *  on dimensions, BPP & flags stored in file.
 *
 *  @param pFile Handle to the bitmap file. Will be closed on function return.
 *  @param isFast True to allocate bitmap in FAST RAM
 *  @return Pointer to newly created bitmap based on file, 0 on error.
 *
 *  @see bitmapCreateFromPath
 *  @see bitmapLoadFromFd
 *  @see bitmapLoadFromPath
 *  @see bitmapCreate
 *  @see bitmapDestroy
 */
tBitMap* bitmapCreateFromFd(tFile *pFile, UBYTE isFast);

/**
 *  @brief Destroys given bitmap, freeing its resources to OS.
 *  Be sure to end all blitter & display operations on this bitmap
 *  prior to calling this function.
 *
 *  @param pBitMap Bitmap to be destroyed.
 *
 *  @see bitmapCreate
 *  @see bitmapCreateFromFd
 */
void bitmapDestroy(tBitMap *pBitMap);

/**
 *  @brief Checks if given bitmap is interleaved.
 *  Detection should work on any OS bitmap.
 *
 *  @param pBitMap Bitmap to be checked.
 *  @return non-zero if bitmap is interleaved, otherwise zero.
 */
UBYTE bitmapIsInterleaved(const tBitMap *pBitMap);

/**
 * @brief Checks if given bitmap is in CHIP memory.
 *
 * @param pBitMap Bitmap to be checked.
 * @return 1 if bitmap is in CHIP memory, otherwise zero.
 */
UBYTE bitmapIsChip(const tBitMap *pBitMap);

/**
 *  @brief Saves basic Bitmap information to log file.
 *
 *  @param pBitMap Bitmap to be dumped.
 *
 *  @see bitmapSaveBMP
 */
void bitmapDump(const tBitMap *pBitMap);

/**
 * Saves bitmap in ACE .bm format.
 * @param pBitMap Bitmap to be saved.
 * @param szPath  Path of destination file.
 */
void bitmapSave(const tBitMap *pBitMap, const char *szPath);

/**
 *  @brief Saves given Bitmap as BMP file.
 *  Use only for debug purposes, as conversion is outrageously slow.
 *
 *  @param pBitMap    Bitmap to be dumped.
 *  @param pPalette   Palette to be used during export.
 *  @param szFilePath Destination file path.
 */
void bitmapSaveBmp(
	const tBitMap *pBitMap, const UWORD *pPalette, const char *szFileName
);

/**
 *  @brief Returns bitmap width in bytes.
 *  Direct check to BytesPerRow may lead to errors as in interleaved mode it
 *  stores value multiplied by bitplane count.
 */
UWORD bitmapGetByteWidth(const tBitMap *pBitMap);

#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_BITMAP_H_
