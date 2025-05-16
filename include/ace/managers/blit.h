/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_BLIT_H_
#define _ACE_MANAGERS_BLIT_H_

/**
 * @file "blit.h"
 * @brief The blitter manager. Provides the basic abstraction layer for common
 * blitter operations.
 *
 * @note Earlier versions of blitter manager implemented blit queues driven
 * by blitter interrupt. However, it yielded worse performance than manual
 * blitting and was removed.
 *
 * @todo Some convenience for async blitting - state machine?
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AMIGA
#include <exec/interrupts.h>  // struct Interrupt
#include <hardware/dmabits.h> // DMAF_BLITTER
#include <hardware/intbits.h> // INTB_BLIT
#endif // AMIGA

#include <ace/types.h>
#include <ace/managers/log.h>
#include <ace/managers/memory.h>
#include <ace/utils/custom.h>
#include <ace/utils/bitmap.h>

// BltCon0 channel enable bits
#define USEA 0x800
#define USEB 0x400
#define USEC 0x200
#define USED 0x100

// Minterm presets - OR unfriendly!
#define MINTERM_A 0xF0
#define MINTERM_B 0xCC
#define MINTERM_C 0xAA
#define MINTERM_A_OR_C 0xFA
#define MINTERM_NA_AND_C 0x0A
#define MINTERM_COOKIE 0xCA
#define MINTERM_REVERSE_COOKIE 0xAC
#define MINTERM_COPY 0xC0

typedef enum tBlitLineMode {
	BLIT_LINE_MODE_OR = ((ABC | ABNC | NABC | NANBC) | (SRCA | SRCC | DEST)),
	BLIT_LINE_MODE_XOR = ((ABNC | NABC | NANBC) | (SRCA | SRCC | DEST)),
	BLIT_LINE_MODE_ERASE = ((NABC | NANBC | ANBC) | (SRCA | SRCC | DEST)),
} tBlitLineMode;

/**
 * @brief Creates and initializes the blitter manager.
 *
 * Call this function before you use any other blitter-related one.
 */
void blitManagerCreate(void);

/**
 * @brief Cleans up and destroys the blitter manager.
 *
 * Call this function after you have finished using the blitter, e.g. when game
 * closes.
 */
void blitManagerDestroy(void);

/**
 * @brief Checks if blitter is idle.
 *
 * Polls 2 times, taking A1000 Agnus bug workaround into account.
 *
 * @return 1 if blitter is idle, otherwise 0.
 *
 * @see blitWait()
 */
UBYTE blitIsIdle(void);

/**
 * @brief Waits until blitter finishes its work.
 *
 * Polls at least 2 times, taking A1000 Agnus bug workaround into account.
 *
 * @todo Investigate if autosetting BLITHOG inside it is beneficial.
 *
 * @see blitIsIdle()
 */
void blitWait(void);

/**
 * @brief Performs the rectangular copy between two bitmap regions,
 * without any safety checks.
 *
 * @note The data regions should not overlap.
 *
 * @param pSrc Source bitmap.
 * @param wSrcX Source rectangle top-left position's X-coordinate.
 * @param wSrcY Source rectangle top-left position's Y-coordinate.
 * @param pDst Destination bitmap. Can be same as pSrc.
 * @param wDstX Destination rectangle top-left position's X-coordinate.
 * @param wDstY Destination rectangle top-left position's Y-coordinate.
 * @param wWidth Rectangle width.
 * @param wHeight Rectangle height.
 * @param ubMinterm Minterm to be used for blitter operation, usually MINTERM_COOKIE.
 * @return Always 1.
 *
 * @see blitCopyAligned()
 * @see blitSafeCopy()
 * @see blitCopy()
 */
UBYTE blitUnsafeCopy(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, UBYTE ubMinterm
);

/**
 * @brief Performs the safe version of the rectangular copy between two bitmap
 * regions.
 *
 * The safety comes from extra blitCheck() call within.
 *
 * @note This function is slower than blitUnsafeCopy() - it is recommended
 * to use blitCopy() for extra checks only when compiling in ACE_DEBUG mode.
 *
 * @param pSrc Source bitmap.
 * @param wSrcX Source rectangle top-left position's X-coordinate.
 * @param wSrcY Source rectangle top-left position's Y-coordinate.
 * @param pDst Destination bitmap. Can be same as pSrc.
 * @param wDstX Destination rectangle top-left position's X-coordinate.
 * @param wDstY Destination rectangle top-left position's Y-coordinate.
 * @param wWidth Rectangle width.
 * @param wHeight Rectangle height.
 * @param ubMinterm Minterm to be used for blitter operation, usually MINTERM_COOKIE.
 * @param uwLine Source code line for error message. Use blitCopy() for auto-fill.
 * @param szFile Source code file for error message. Use blitCopy() for auto-fill.
 * @return 1 if blit was successful, otherwise 0.
 *
 * @see blitCopyAligned()
 * @see blitUnsafeCopy()
 * @see blitCopy()
 * @see blitCheck()
 */
UBYTE blitSafeCopy(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, UBYTE ubMinterm,
	UWORD uwLine, const char *szFile
);

/**
 * @brief Performs the optimized rectangular copy between two bitmap regions,
 * without any safety checks.
 *
 * @note This function requires that X-coordinates of copy regions as well
 * as width are multiples of 16. If you need more generic blit, use blitCopy()
 * instead.
 *
 * @param pSrc Source bitmap.
 * @param wSrcX Source rectangle top-left position's X-coordinate.
 * @param wSrcY Source rectangle top-left position's Y-coordinate.
 * @param pDst Destination bitmap. Can be same as pSrc.
 * @param wDstX Destination rectangle top-left position's X-coordinate.
 * @param wDstY Destination rectangle top-left position's Y-coordinate.
 * @param wWidth Rectangle width.
 * @param wHeight Rectangle height.
 * @return Always 1.
 *
 * @see blitSafeCopyAligned()
 * @see blitCopyAligned()
 * @see blitCopy()
 */
UBYTE blitUnsafeCopyAligned(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight
);

/**
 * @brief Performs the safe version of optimized rectangular copy between
 * two bitmap regions.
 *
 * @note This function requires that X-coordinates of copy regions as well
 * as width are multiples of 16. If you need more generic blit, use blitCopy()
 * instead.
 *
 * The safety comes from extra blitCheck() call within.
 *
 * @note This function is slower than blitUnsafeCopyAligned() - it is recommended
 * to use blitCopyAligned() for extra checks only when compiling in ACE_DEBUG mode.
 *
 * @param pSrc Source bitmap.
 * @param wSrcX Source rectangle top-left position's X-coordinate.
 * @param wSrcY Source rectangle top-left position's Y-coordinate.
 * @param pDst Destination bitmap. Can be same as pSrc.
 * @param wDstX Destination rectangle top-left position's X-coordinate.
 * @param wDstY Destination rectangle top-left position's Y-coordinate.
 * @param wWidth Rectangle width.
 * @param wHeight Rectangle height.
 * @param uwLine Source code line for error message. Use blitCopyAligned() for auto-fill.
 * @param szFile Source code file for error message. Use blitCopyAligned() for auto-fill.
 * @return 1 if blit was successful, otherwise 0.
 *
 * @see blitUnsafeCopyAligned()
 * @see blitCopyAligned()
 * @see blitCopy()
 */
UBYTE blitSafeCopyAligned(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight,
	UWORD uwLine, const char *szFile
);

/**
 * @brief Performs the rectangular copy between two bitmap regions using
 * the mask data.
 *
 * @note For regular bitmaps, mask data represents an alpha channel for single
 * bitplane of pSrc.
 * For interleaved blits, the mask must also have the interleaved format,
 * repeating its data for each line of each bitplane.
 *
 * @param pSrc Source bitmap.
 * @param wSrcX Source rectangle top-left position's X-coordinate.
 * @param wSrcY Source rectangle top-left position's Y-coordinate.
 * @param pDst Destination bitmap. Can be same as pSrc.
 * @param wDstX Destination rectangle top-left position's X-coordinate.
 * @param wDstY Destination rectangle top-left position's Y-coordinate.
 * @param wWidth Rectangle width.
 * @param wHeight Rectangle height.
 * @param pMsk Raw mask bitplane data.
 * @return Always 1.
 *
 * @see blitSafeCopyMask()
 * @see blitCopyMask()
 */
UBYTE blitUnsafeCopyMask(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, const UBYTE *pMsk
);

/**
 * @brief Performs the safe version of rectangular copy between two bitmap
 * regions using the mask data.
 *
 * The safety comes from extra blitCheck() call within.
 *
 * @note This function is slower than blitUnsafeCopyMask() - it is recommended
 * to use blitCopyMask() for extra checks only when compiling in ACE_DEBUG mode.
 *
 * @note For regular bitmaps, mask data represents an alpha channel for single
 * bitplane of pSrc.
 * For interleaved blits, the mask must also have the interleaved format,
 * repeating its data for each line of each bitplane.
 *
 * @param pSrc Source bitmap.
 * @param wSrcX Source rectangle top-left position's X-coordinate.
 * @param wSrcY Source rectangle top-left position's Y-coordinate.
 * @param pDst Destination bitmap. Can be same as pSrc.
 * @param wDstX Destination rectangle top-left position's X-coordinate.
 * @param wDstY Destination rectangle top-left position's Y-coordinate.
 * @param wWidth Rectangle width.
 * @param wHeight Rectangle height.
 * @param pMsk Raw mask bitplane data.
 * @param uwLine Source code line for error message. Use blitCopyMask() for auto-fill.
 * @param szFile Source code file for error message. Use blitCopyMask() for auto-fill.
 * @return 1 if blit was successful, otherwise 0.
 *
 * @see blitUnsafeCopyMask()
 * @see blitCopyMask()
 */
UBYTE blitSafeCopyMask(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, const UBYTE *pMsk,
	UWORD uwLine, const char *szFile
);

/**
 * @brief Performs the rectangular fill with selected color.
 *
 * The blit is configured bitplane-by-bitplane as follows:
 * - A: rectangle mask, memory read disabled
 * - C: destination read
 * - D: destination write
 *
 * @note This can't be used for large blits - OCS blitter limits apply.
 * Maximum blit size is 1024x1024 pixels. For interleaved bitmaps, divide
 * max height by bitmap's depth.
 *
 * @param pDst Destination bitmap.
 * @param wDstX Destination rectangle top-left position's X-coordinate.
 * @param wDstY Destination rectangle top-left position's Y-coordinate.
 * @param wWidth Rectangle width.
 * @param wHeight Rectangle height.
 * @param ubColor Target color index.
 * @return Always 1.
 *
 * @see blitSafeRect()
 * @see blitRect()
 */
UBYTE blitUnsafeRect(
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubColor
);

/**
 * @brief Performs the safe version of rectangular fill with selected color.
 *
 * The blit is configured bitplane-by-bitplane as follows:
 * - A: rectangle mask, memory read disabled
 * - C: destination read
 * - D: destination write
 *
 * The safety comes from extra blitCheck() call within.
 *
 * @note This function is slower than blitUnsafeRect() - it is recommended
 * to use blitRect() for extra checks only when compiling in ACE_DEBUG mode.
 *
 * @note This can't be used for large blits - OCS blitter limits apply.
 * Maximum blit size is 1024x1024 pixels. For interleaved bitmaps, divide
 * max height by bitmap's depth.
 *
 * @param pDst Destination bitmap.
 * @param wDstX Destination rectangle top-left position's X-coordinate.
 * @param wDstY Destination rectangle top-left position's Y-coordinate.
 * @param wWidth Rectangle width.
 * @param wHeight Rectangle height.
 * @param ubColor Target color index.
 * @param uwLine Source code line for error message. Use blitRect() for auto-fill.
 * @param szFile Source code file for error message. Use blitRect() for auto-fill.
 * @return 1 if blit was successful, otherwise 0.
 *
 * @see blitUnsafeRect()
 * @see blitRect()
 */
UBYTE blitSafeRect(
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubColor, UWORD uwLine, const char *szFile
);

/**
 * @brief Performs the shape fill on a single bitplane inside given rectangle.
 *
 * @note IF ACE is not in ECS mode, OCS blitter limits apply.
 * Maximum blit size is 1024x1024 pixels.
 *
 * @note This function requires that X-coordinate as well as width
 * of the fill region are multiples of 16.
 *
 * @param pDst Destination bitmap.
 * @param wDstX Destination rectangle top-left position's X-coordinate.
 * @param wDstY Destination rectangle top-left position's Y-coordinate.
 * @param wWidth Rectangle width.
 * @param wHeight Rectangle height.
 * @param ubPlane Bitplane to be used.
 * @param ubFillMode A combination of `FILL_OR`, `FILL_XOR` and `FILL_CARRYIN`.
 * @return Always 1.
 *
 * @see blitSafeFillAligned()
 */
void blitUnsafeFillAligned(
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubPlane, UBYTE ubFillMode
);

/**
 * @brief Performs the safe version of shape fill on a single bitplane inside
 * given rectangle.
 *
 * The safety comes from extra blitCheck() call within.
 *
 * @note IF ACE is not in ECS mode, OCS blitter limits apply.
 * Maximum blit size is 1024x1024 pixels.
 *
 * @note This function requires that X-coordinate as well as width
 * of the fill region are multiples of 16.
 *
 * @param pDst Destination bitmap.
 * @param wDstX Destination rectangle top-left position's X-coordinate.
 * @param wDstY Destination rectangle top-left position's Y-coordinate.
 * @param wWidth Rectangle width.
 * @param wHeight Rectangle height.
 * @param ubPlane Bitplane to be used.
 * @param ubFillMode A combination of `FILL_OR`, `FILL_XOR` and `FILL_CARRYIN`.
 * @param uwLine Source code line for error message. Use blitFillAligned() for auto-fill.
 * @param szFile Source code file for error message. Use blitFillAligned() for auto-fill.
 * @return Always 1.
 *
 * @see blitUnsafeFillAligned()
 */
UBYTE blitSafeFillAligned(
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubPlane, UBYTE ubFillMode, UWORD uwLine, const char *szFile
);

/**
 * @brief Draws line of given color between two points.
 *
 * This function writes through all bitmap's bitplanes, so it's quite slow.
 * You can speed up your lines by drawing on only single bitplane - that's what
 * most demos do.
 *
 * @param pDst: Destination bitmap.
 * @param wX1: Line start position's X-coordinate.
 * @param wY1: Line start position's Y-coordinate.
 * @param wX2: Line end position's X-coordinate.
 * @param wY2: Line end position's Y-coordinate.
 * @param ubColor: Line's color index.
 * @param uwPattern: 16-bit pattern to be used. 1: filled pixel, 0: omitted.
 * @param isOneDot: If set to 1, draws fill-friendly lines.
 *
 * @see blitLinePlane()
 */
void blitLine(
	tBitMap *pDst, WORD x1, WORD y1, WORD x2, WORD y2,
	UBYTE ubColor, UWORD uwPattern, UBYTE isOneDot
);

/**
 * @brief Draws line between two points on a single bitplane.
 *
 * This is way faster than drawing a line across multiple bitplanes.
 *
 * @param pDst: Destination bitmap.
 * @param wX1: Line start position's X-coordinate.
 * @param wY1: Line start position's Y-coordinate.
 * @param wX2: Line end position's X-coordinate.
 * @param wY2: Line end position's Y-coordinate.
 * @param ubPlane Bitplane index to use.
 * @param uwPattern: 16-bit pattern to be used. 1: filled pixel, 0: omitted.
 * @param eMode Set to one of tBlitLineMode values.
 * @param isOneDot: If set to 1, draws fill-friendly lines.
 *
 * @see blitLine()
 */
void blitLinePlane(
	tBitMap *pDst, WORD wX1, WORD wY1, WORD wX2, WORD wY2,
	UBYTE ubPlane, UWORD uwPattern, tBlitLineMode eMode, UBYTE isOneDot
);

#ifdef ACE_DEBUG

/**
 * @brief Checks if blit is legal at coords at given source and destination.
 *
 * @param pSrc Source bitmap.
 * @param wSrcX Source rectangle top-left position's X-coordinate.
 * @param wSrcY Source rectangle top-left position's Y-coordinate.
 * @param pDst Destination bitmap. Can be same as pSrc.
 * @param wDstX Destination rectangle top-left position's X-coordinate.
 * @param wDstY Destination rectangle top-left position's Y-coordinate.
 * @param wWidth Rectangle width.
 * @param wHeight Rectangle height.
 * @param uwLine Source code line for error message. Use blitCheck() for auto-fill.
 * @param szFile Source code file for error message. Use blitCheck() for auto-fill.
 * @return 1 if blit can safely be made, otherwise 0.
 */
UBYTE _blitCheck(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	const tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UWORD uwLine, const char *szFile
);

#define blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile) \
	_blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile)
#define blitCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm) \
	blitSafeCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm, __LINE__, __FILE__)
#define blitCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight) \
	blitSafeCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, __LINE__, __FILE__)
#define blitCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk) \
	blitSafeCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk, __LINE__, __FILE__)
#define blitRect(pDst, wDstX, wDstY, wWidth, wHeight, ubColor) \
	blitSafeRect(pDst, wDstX, wDstY, wWidth, wHeight, ubColor, __LINE__, __FILE__)
#define blitFillAligned(pDst, wDstX, wDstY, wWidth, wHeight, ubPlane, ubFillMode) \
	blitSafeFillAligned(pDst, wDstX, wDstY, wWidth, wHeight, ubPlane, ubFillMode, __LINE__, __FILE__)

#else

#define blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile) \
	({(void)(uwLine); (void)(szFile); 1;})
#define blitCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm) \
	blitUnsafeCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm)
#define blitCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight) \
	blitUnsafeCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight)
#define blitCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk) \
	blitUnsafeCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk)
#define blitRect(pDst, wDstX, wDstY, wWidth, wHeight, ubColor) \
	blitUnsafeRect(pDst, wDstX, wDstY, wWidth, wHeight, ubColor)
#define blitFillAligned(pDst, wDstX, wDstY, wWidth, wHeight, ubPlane, ubFillMode) \
	blitUnsafeFillAligned(pDst, wDstX, wDstY, wWidth, wHeight, ubPlane, ubFillMode)

#endif // ACE_DEBUG

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_BLIT_H_
