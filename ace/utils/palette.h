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

#endif