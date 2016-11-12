#ifndef GUARD_ACE_UTIL_PALETTE_H
#define GUARD_ACE_UTIL_PALETTE_H

/**
 *  Palette utilities.
 */

#include <ace/config.h>
#include <ace/utils/extview.h>

/* Types */

/* Globals */

/* Functions */

/**
 *  @brief Loads palette from supplied .plt file to given address.
 *  @param szFileName  Palette source path.
 *  @param pPalette    Palette destination pointer.
 *  @param ubMaxLength Maximum number of colors in palette.
 */
void paletteLoad(
	IN char *szFileName,
	OUT UWORD *pPalette,
	IN UBYTE ubMaxLength
);

/**
 *  @brief Dims palette to given brightness level.
 *  @param pSource      Pointer to source palette.
 *  @param pDest        Pointer to destination palette. May be same as pSource.
 *  @param ubColorCount Number of colors in palette.
 *  @param ubLevel      Brightness level - 15 for no dim, 0 for total blackness.
 */
void paletteDim(
	IN UWORD *pSource,
	OUT UWORD *pDest,
	IN UBYTE ubColorCount,
	IN UBYTE ubLevel
);

#endif