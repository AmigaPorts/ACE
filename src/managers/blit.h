#ifndef GUARD_ACE_MANAGER_BLIT_H
#define GUARD_ACE_MANAGER_BLIT_H

/**
 * The mighty blitter queue manager
 * Allows complete async between CPU and blitter by adding blits to queue
 */

#include <clib/exec_protos.h> // Amiga typedefs
#include <exec/interrupts.h>  // struct Interrupt
#include <hardware/dmabits.h> // DMAF_BLITTER
#include <hardware/intbits.h> // INTB_BLIT

#include "config.h"
#include "managers/log.h"
#include "managers/memory.h"
#include "utils/custom.h"
#include "utils/bitmap.h"

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

// 34 bajty na strukturê
// przy kolejce 1024 wpisów obci¹¿enie 34KB
// Villages do odrysowania pe³nej ramki tilemapy potrzebuje WxHxBPP 14x12x5 = 840 blitów (27,9KB)
// W przypadku grafiki interleaved bêdzie to 14x12 = 168 blitów (5,6KB)

typedef void fnBlitterFill(
	UWORD bltcon0, UWORD bltcon1, UWORD bltafwm, UWORD bltalwm,
	WORD bltamod, WORD bltbmod, WORD bltcmod, WORD bltdmod,
	UBYTE *bltapt, UBYTE *bltbpt, UBYTE *bltcpt, UBYTE *bltdpt,
	UWORD bltadat, UWORD bltbdat, UWORD bltcdat,
	UWORD bltsize
);

/**
 * Single blit data - mirror of custom chip registers
 * Fields are aligned in same order as in Custom struct
 */
typedef struct {
	WORD bltcmod; /// Bitplane C Modulo
	WORD bltbmod; /// Bitplane B Modulo
	WORD bltamod; /// Bitplane A Modulo
	WORD bltdmod; /// Bitplane D Modulo
	
	UWORD bltcon0; /// Blit control 0
	UWORD bltcon1; /// Blit control 1
	
	UWORD bltafwm; /// First word mask
	UWORD bltalwm; /// Last word mask
	
	UBYTE *bltcpt; /// Bitplane C ptr
	UBYTE *bltbpt; /// Bitplane B ptr
	UBYTE *bltapt; /// Bitplane A ptr
	UBYTE *bltdpt; /// Bitplane D ptr
	
	UWORD bltsize; /// Blit size
	
	UWORD bltcdat; /// Bitplane C data
	UWORD bltbdat; /// Bitplane B data
	UWORD bltadat; /// Bitplane A data
} tBlitData;

/**
 * Blit manager struct
 */
typedef struct {
	UWORD uwQueueLength;          /// Length of blit queue
	UWORD uwAddPos;               /// Queue pos at which next blit will be added
	UWORD uwBlitPos;              /// Queue pos which blitter currently processes
	tBlitData *pBlitData;         /// Blit queue array
	fnBlitterFill *pBlitterSetFn;
	char *szHandlerName;          /// For interrupt handler, meaningful description
	struct Interrupt *pInt;       /// Interrupt structure of manager, must be PUBLIC type
	                              // Previous interrupt data
	struct Interrupt *pPrevInt;   /// Previous registered interrupt handler
	UWORD uwOldIntEna;            /// Old intEna
	UWORD uwOldDmaCon;            /// Old dmaCon
	UBYTE ubBlitStarted;          /// 1 if last blitter op was blit from queue
} tBlitManager;

extern tBlitManager g_sBlitManager;

void blitManagerCreate(
	IN UWORD uwQueueLength,
	IN UWORD uwFlags
);

void blitManagerDestroy(void);

void blitQueued(
	IN UWORD bltcon0,
	IN UWORD bltcon1,
	IN UWORD bltafwm,
	IN UWORD bltalwm,
	IN WORD bltamod,
	IN WORD bltbmod,
	IN WORD bltcmod,
	IN WORD bltdmod,
	IN UBYTE *bltapt,
	IN UBYTE *bltbpt,
	IN UBYTE *bltcpt,
	IN UBYTE *bltdpt,
	IN UWORD bltadat,
	IN UWORD bltbdat,
	IN UWORD bltcdat,
	IN UWORD bltsize
);

void blitNotQueued(
	IN UWORD bltcon0,
	IN UWORD bltcon1,
	IN UWORD bltafwm,
	IN UWORD bltalwm,
	IN WORD bltamod,
	IN WORD bltbmod,
	IN WORD bltcmod,
	IN WORD bltdmod,
	IN UBYTE *bltapt,
	IN UBYTE *bltbpt,
	IN UBYTE *bltcpt,
	IN UBYTE *bltdpt,
	IN UWORD bltadat,
	IN UWORD bltbdat,
	IN UWORD bltcdat,
	IN UWORD bltsize
);

BYTE blitUnsafeCopy(
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

BYTE blitSafeCopy(
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
#else
# define blitCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm, ubMask) blitUnsafeCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm, ubMask)
#endif

BYTE blitUnsafeCopyAligned(
	IN tBitMap *pSrc,
	IN WORD wSrcX,
	IN WORD wSrcY,
	IN tBitMap *pDst,
	IN WORD wDstX,
	IN WORD wDstY,
	IN WORD wWidth,
	IN WORD wHeight
);

BYTE blitSafeCopyAligned(
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
# define blitCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight) blitSafeCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, __LINE__, __FILE__)
#else
#define blitCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight) blitUnsafeCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight)
#endif

BYTE blitUnsafeCopyMask(
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

BYTE blitSafeCopyMask(
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

BYTE _blitRect(
	IN tBitMap *pDst,
	IN WORD wDstX,
	IN WORD wDstY,
	IN WORD wWidth,
	IN WORD wHeight,
	IN UBYTE ubColor,
	IN UWORD uwLine,
	IN char *szFile
);

#define blitRect(pDst, wDstX, wDstY, wWidth, wHeight, ubColor) _blitRect(pDst, wDstX, wDstY, wWidth, wHeight, ubColor, __LINE__, __FILE__)

#endif