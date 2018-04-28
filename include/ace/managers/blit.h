#ifndef GUARD_ACE_MANAGER_BLIT_H
#define GUARD_ACE_MANAGER_BLIT_H

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

UBYTE blitUnsafeCopy(
	IN tBitMap *pSrc,
	IN WORD wSrcX,
	IN WORD wSrcY,
	IN tBitMap *pDst,
	IN WORD wDstX,
	IN WORD wDstY,
	IN WORD wWidth,
	IN WORD wHeight,
	IN UBYTE ubMinterm,
	IN UBYTE ubMask
);

UBYTE blitSafeCopy(
	IN tBitMap *pSrc,
	IN WORD wSrcX,
	IN WORD wSrcY,
	IN tBitMap *pDst,
	IN WORD wDstX,
	IN WORD wDstY,
	IN WORD wWidth,
	IN WORD wHeight,
	IN UBYTE ubMinterm,
	IN UBYTE ubMask,
	IN UWORD uwLine,
	IN char *szFile
);

#ifdef GAME_DEBUG
# define blitCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm, ubMask) blitSafeCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm, ubMask, __LINE__, __FILE__)
#define blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile) _blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile)
#else
# define blitCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm, ubMask) blitUnsafeCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm, ubMask)
#define blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile) 1
#endif

UBYTE blitUnsafeCopyAligned(
	IN tBitMap *pSrc,
	IN WORD wSrcX,
	IN WORD wSrcY,
	IN tBitMap *pDst,
	IN WORD wDstX,
	IN WORD wDstY,
	IN WORD wWidth,
	IN WORD wHeight
);

UBYTE blitSafeCopyAligned(
	IN tBitMap *pSrc,
	IN WORD wSrcX,
	IN WORD wSrcY,
	IN tBitMap *pDst,
	IN WORD wDstX,
	IN WORD wDstY,
	IN WORD wWidth,
	IN WORD wHeight,
	IN UWORD uwLine,
	IN char *szFile
);

#ifdef GAME_DEBUG
#define blitCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight) blitSafeCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, __LINE__, __FILE__)
#else
#define blitCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight) blitUnsafeCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight)
#endif

UBYTE blitUnsafeCopyMask(
	IN tBitMap *pSrc,
	IN WORD wSrcX,
	IN WORD wSrcY,
	IN tBitMap *pDst,
	IN WORD wDstX,
	IN WORD wDstY,
	IN WORD wWidth,
	IN WORD wHeight,
	IN UWORD *pMsk
);

UBYTE blitSafeCopyMask(
	IN tBitMap *pSrc,
	IN WORD wSrcX,
	IN WORD wSrcY,
	IN tBitMap *pDst,
	IN WORD wDstX,
	IN WORD wDstY,
	IN WORD wWidth,
	IN WORD wHeight,
	IN UWORD *pMsk,
	IN UWORD uwLine,
	IN char *szFile
);

#ifdef GAME_DEBUG
# define blitCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk) blitSafeCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk, __LINE__, __FILE__)
#else
# define blitCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk) blitUnsafeCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk)
#endif

UBYTE _blitRect(
	IN tBitMap *pDst,
	IN WORD wDstX,
	IN WORD wDstY,
	IN WORD wWidth,
	IN WORD wHeight,
	IN UBYTE ubColor,
	IN UWORD uwLine,
	IN char *szFile
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
	IN tBitMap *pDst,
	IN WORD x1,
	IN WORD y1,
	IN WORD x2,
	IN WORD y2,
	IN UBYTE ubColor,
	IN UWORD uwPattern,
	IN UBYTE isOneDot
);

#endif // GUARD_ACE_MANAGER_BLIT_H
