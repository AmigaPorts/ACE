/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_BLIT_H_
#define _ACE_MANAGERS_BLIT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The mighty blitter manager
 * There was a queue mechanism, it's gone now.
 * @todo Some convenience for async - state machine?
 */

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
#define MINTERM_COOKIE 0xCA
#define MINTERM_COPY 0xC0

/**
 * Blit manager struct
 */
typedef struct {
	FUBYTE fubDummy;
} tBlitManager;

extern tBlitManager g_sBlitManager;

void blitManagerCreate(void);
void blitManagerDestroy(void);

/**
 * Checks if blitter is idle
 * Polls 2 times - A1000 Agnus bug workaround
 */
UBYTE blitIsIdle(void);

/**
 * Waits until blitter finishes its work.
 * In the meantime, BLITHOG bit is set.
 */
void blitWait(void);

/**
 * Blit without mask. Channels:
 * 	A: mask const, read disabled
 * 	B: src read
 * 	C: dest read
 * 	D: dest write
 *
 * @param ubMinterm Blitter logic operation to be used. Usually set to MINTERM_COOKIE.
 * @note Source and destination regions must not overlap.
 */
UBYTE blitUnsafeCopy(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, UBYTE ubMinterm
);

UBYTE blitSafeCopy(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, UBYTE ubMinterm,
	UWORD uwLine, const char *szFile
);

#ifdef ACE_DEBUG

UBYTE _blitCheck(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	const tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UWORD uwLine, const char *szFile
);

# define blitCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm) blitSafeCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm, __LINE__, __FILE__)
#define blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile) _blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile)
#else
# define blitCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm) blitUnsafeCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm)
#define blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile) 1
#endif

UBYTE blitUnsafeCopyAligned(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight
);

UBYTE blitSafeCopyAligned(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight,
	UWORD uwLine, const char *szFile
);

#ifdef ACE_DEBUG
#define blitCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight) blitSafeCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, __LINE__, __FILE__)
#else
#define blitCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight) blitUnsafeCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight)
#endif

UBYTE blitUnsafeCopyMask(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, const UWORD *pMsk
);

UBYTE blitSafeCopyMask(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, const UWORD *pMsk,
	UWORD uwLine, const char *szFile
);

#ifdef ACE_DEBUG
# define blitCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk) blitSafeCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk, __LINE__, __FILE__)
#else
# define blitCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk) blitUnsafeCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk)
#endif

UBYTE _blitRect(
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, UBYTE ubColor,
	UWORD uwLine, const char *szFile
);

#define blitRect(pDst, wDstX, wDstY, wWidth, wHeight, ubColor) \
	_blitRect(pDst, wDstX, wDstY, wWidth, wHeight, ubColor, __LINE__, __FILE__)

/**
 * @brief Draws line of given color between two points.
 * This function writes through all bitmap's bitplanes, so it's quite slow.
 * You can speed up your lines by drawing on only single bitplane - that's what
 * most demos do.
 *
 * @param pDst: Destination bitmap.
 * @param x1: Line start position's X coordinate.
 * @param y1: Line start position's Y coordinate.
 * @param x2: Line end position's X coordinate.
 * @param y2: Line end position's Y coordinate.
 * @param ubColor: Line's color index.
 * @param uwPattern: 16-bit pattern to be used. 1: filled pixel, 0: omitted.
 * @param isOneDot: If set to 1, draws fill-friendly lines.
 */
void blitLine(
	tBitMap *pDst, WORD x1, WORD y1, WORD x2, WORD y2,
	UBYTE ubColor, UWORD uwPattern, UBYTE isOneDot
);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_BLIT_H_
