/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_PALETTE_H
#define _ACE_UTILS_PALETTE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Palette utilities.
 */

#include <ace/types.h>
#include <ace/utils/extview.h>

/** First byte of .plt v2: ECS/OCS packed entries (2 bytes per colour). */
#define PLT_NEW_ECS 0
/** First byte of .plt v2: AGA entries (4 bytes per colour, alpha+R+G+B). */
#define PLT_NEW_AGA 1

/**
 * @brief Loads palette from supplied .plt file to given address.
 *
 * Supports **v2** only (.plt starting with PLT_NEW_ECS or PLT_NEW_AGA + big-endian UWORD count).
 * Legacy **v1** `.plt` (first byte ≥ 2) is rejected (`ACE_DEBUG`: log error; @p pPalette unchanged).
 * For PLT_NEW_AGA, @p pPalette must point at storage suitable for ULONG-sized colours
 * (same layout as an AGA viewport palette).
 *
 * @param szPath Palette source path.
 * @param pPalette Palette destination pointer.
 * @param uwMaxLength Maximum number of colors to read.
 *
 * @see paletteLoadFromFd()
 */
void paletteLoadFromPath(const char *szPath, UWORD *pPalette, UWORD uwMaxLength);

/**
 * @brief Saves ECS/OCS palette into .plt v2 file (PLT_NEW_ECS + BE count + packed colours).
 */
void paletteSave(const UWORD *pPalette, UWORD uwColorCnt, char *szPath);

#ifdef ACE_USE_AGA_FEATURES
/**
 * @brief Saves AGA palette into .plt v2 file (PLT_NEW_AGA + BE count + 4 bytes per colour).
 */
void paletteSaveAGA(const ULONG *pPalette, UWORD uwColorCnt, char *szPath);
#endif

/**
 * @brief Loads palette from supplied .plt file to given address.
 * @param pFile Handle to the palette file. Will be closed on function return.
 * @param pPalette Palette destination pointer. For v2 AGA files, this must be
 *        storage suitable for `ULONG` per entry (e.g. AGA viewport palette).
 * @param uwMaxLength Maximum number of colors to read.
 *
 * @see paletteLoadFromPath()
 */
void paletteLoadFromFd(tFile *pFile, UWORD *pPalette, UWORD uwMaxLength);

/**
 * @brief Dims palette to given brightness level.
 * @param pSource Pointer to source palette.
 * @param pDest Pointer to destination palette. Can be same as pSource.
 * Can also be pointing directly on chipset palette registers.
 * @param uwColorCount Number of colors in palette.
 * @param ubLevel Brightness level - 15 for no dim, 0 for total blackness.
 *
 * @warning DON'T point pSource to chipset palette registers, they are read-only!
 */
void paletteDim(
	UWORD *pSource, volatile UWORD *pDest, UWORD uwColorCount, UBYTE ubLevel
);

#ifdef ACE_USE_AGA_FEATURES
void paletteDimAGA(
    ULONG *pSource, volatile ULONG *pDest, UWORD uwColorCount, UBYTE ubLevel
);
#endif

/**
 * @brief Dims a single input color to given brightness level.
 * @param uwFullColor Full color used as a base to calculate percentage.
 * @param ubLevel Brightness level - 15 for no dim, 0 for total blackness.
 *
 * @see paletteColorMix()
 */
UWORD paletteColorDim(UWORD uwFullColor, UBYTE ubLevel);
#ifdef ACE_USE_AGA_FEATURES
ULONG paletteColorDimAGA(ULONG ulFullColor, UBYTE ubLevel);
#endif

/**
 * @brief Interpolates two colors at given level.
 * @param uwColorPrimary Primary color in the mix.
 * @param uwColorSecondary Secondary color in the mix.
 * @param ubLevel Mix ratio - 15 results in primary color, 0 in secondary.
 * @return Mixed color between uwColorPrimary and uwColorSecondary.
 *
 * @note This function is slower than paletteColorDim().
 * @see paletteColorDim()
 */
UWORD paletteColorMix(UWORD uwColorPrimary, UWORD uwColorSecondary, UBYTE ubLevel);

#ifdef ACE_USE_AGA_FEATURES
/**
 * @brief Interpolates two AGA colors at given level.
 * @param ulColorPrimary Primary AGA color in the mix.
 * @param ulColorSecondary Secondary AGA color in the mix.
 * @param ubLevel Mix ratio - 255 results in primary color, 0 in secondary.
 * @return Mixed AGA color between ulColorPrimary and ulColorSecondary.
 *
 * @note This function is slower than paletteColorDimAGA().
 * @see paletteColorDimAGA()
 */
ULONG paletteColorMixAGA(ULONG ulColorPrimary, ULONG ulColorSecondary, UBYTE ubLevel);
#endif

/**
 * @brief Writes given palette to debug .bmp file.
 *
 * This function is horribly slow and should only be used for debug purposes.
 *
 * @param pPalette Palette to be dumped.
 * @param fubColorCnt Number of colors in palette.
 * @param szPath Destination path for .bmp file.
 */
void paletteDump(UWORD *pPalette, UWORD uwColorCnt, char *szPath);

#ifdef ACE_USE_AGA_FEATURES
/**
 * @brief Writes given AGA palette to debug .bmp file.
 *
 * This function is horribly slow and should only be used for debug purposes.
 *
 * @param pPalette AGA palette to be dumped.
 * @param uwColorCnt Number of colors in palette.
 * @param szPath Destination path for .bmp file.
 */
void paletteDumpAGA(ULONG *pPalette, UWORD uwColorCnt, char *szPath);
#endif

#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_PALETTE_H
