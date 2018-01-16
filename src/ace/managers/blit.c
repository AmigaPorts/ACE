#include <ace/managers/blit.h>

#define BLIT_LINE_OR ((ABC | ABNC | NABC | NANBC) | (SRCA | SRCC | DEST))
#define BLIT_LINE_XOR ((ABNC | NABC | NANBC) | (SRCA | SRCC | DEST))
#define BLIT_LINE_ERASE ((NABC | NANBC | ANBC) | (SRCA | SRCC | DEST))

tBlitManager g_sBlitManager = {0};

#ifdef AMIGA
/**
 * Blit interrupt handler
 * Fetches next blit from queue and sets custom registers to its values
 * NOTE: Can't log inside this fn and all other called by it
 */
void INTERRUPT blitInterruptHandler(
	REGARG(struct Custom *cstm, "a0"), REGARG(tBlitManager *pBlitManager, "a1")
) {
	cstm->intreq = INTF_BLIT;
	INTERRUPT_END;
}
#endif // AMIGA

void blitManagerCreate(void) {
	logBlockBegin("blitManagerCreate");
#ifdef AMIGA
	OwnBlitter();
	blitWait();
#endif // AMIGA
	logBlockEnd("blitManagerCreate");
}

void blitManagerDestroy(void) {
	logBlockBegin("blitManagerDestroy");
#ifdef AMIGA
	blitWait();
	DisownBlitter();
#endif //AMIGA
	logBlockEnd("blitManagerDestroy");
}

/**
 * Checks if blit is allowable at coords at given source and destination
 */
UBYTE blitCheck(
	tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UWORD uwLine, char *szFile
) {
#ifdef GAME_DEBUG
	WORD wSrcWidth, wDstWidth;

	wSrcWidth = pSrc->BytesPerRow << 3;
	wDstWidth = pDst->BytesPerRow << 3;
	if(bitmapIsInterleaved(pSrc))
		wSrcWidth /= pSrc->Depth;
	if(bitmapIsInterleaved(pDst))
		wDstWidth /= pDst->Depth;

	if(pSrc && (wSrcX < 0 || wSrcWidth < wSrcX+wWidth || pSrc->Rows < wSrcY+wHeight)) {
		logWrite(
			"ILLEGAL BLIT Source out of range: "
			"source %p %dx%d, dest: %p %dx%d, blit: %d,%d -> %d,%d %dx%d %s@%u\n",
			pSrc,	wSrcWidth, pSrc->Rows, pDst, wDstWidth, pDst->Rows,
			wSrcX, wSrcY, wDstX, wDstY, wWidth, wHeight, szFile, uwLine
		);
		return 0;
	}
	if(pDst && (wDstY < 0 || wDstWidth < wDstX+wWidth || pDst->Rows < wDstY+wHeight)) {
		logWrite(
			"ILLEGAL BLIT Dest out of range: "
			"source %p %dx%d, dest: %p %dx%d, blit: %d,%d -> %d,%d %dx%d %s@%u\n",
			pSrc, wSrcWidth, pSrc->Rows, pDst, wDstWidth, pDst->Rows,
			wSrcX, wSrcY, wDstX, wDstY, wWidth, wHeight, szFile, uwLine
		);
		return 0;
	}
#endif
	return 1;
}

void blitWait(void) {
		custom.dmacon = BITSET | DMAF_BLITHOG;
		while(!blitIsIdle()) {}
		custom.dmacon = BITCLR | DMAF_BLITHOG;
}

/**
 * Checks if blitter is idle
 * Polls 2 times - A1000 Agnus bug workaround
 * @todo Make it inline assembly or dmaconr volatile so compiler won't
 * 'optimize' it.
 */
UBYTE blitIsIdle(void) {
	#ifdef AMIGA
	volatile UWORD * const pDmaConR = &custom.dmaconr;
	if(!(*pDmaConR & DMAF_BLTDONE))
		if(!(*pDmaConR & DMAF_BLTDONE))
			return 1;
	return 0;
	#else
		return 1;
	#endif // AMIGA
}

/**
 * Blit without mask - BltBitMap equivalent, but less safe
 * Channels:
 * 	A: mask const, read disabled
 * 	B: src read
 * 	C: dest read
 * 	D: dest write
 * Descending mode is used under 2 cases:
 * 	- Blit needs shifting to left with previous data coming from right (ubSrcDelta > ubDstDelta)
 * 	- Ascending right mask shifted more than 16 bits
 * Source and destination regions should not overlap.
 * Function is slightly slower (~0.5 - 1.5ms) than bltBitMap:
 * 	- Pre-loop calculations take ~50us on ASC mode, ~80us on DESC mode
 * 	- Rewriting to assembly could speed things up a bit
 */
UBYTE blitUnsafeCopy(
	tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubMinterm, UBYTE ubMask
) {
#ifdef AMIGA
	// Helper vars
	UWORD uwBlitWords, uwBlitWidth;
	ULONG ulSrcOffs, ulDstOffs;
	UBYTE ubShift, ubSrcDelta, ubDstDelta, ubWidthDelta, ubMaskFShift, ubMaskLShift, ubPlane;
	// Blitter register values
	UWORD uwBltCon0, uwBltCon1, uwFirstMask, uwLastMask;
	WORD wSrcModulo, wDstModulo;

	ubSrcDelta = wSrcX & 0xF;
	ubDstDelta = wDstX & 0xF;
	ubWidthDelta = (ubSrcDelta + wWidth) & 0xF;

	if(ubSrcDelta > ubDstDelta || ((wWidth+ubDstDelta+15) & 0xFFF0)-(wWidth+ubSrcDelta) > 16) {
		uwBlitWidth = (wWidth+(ubSrcDelta>ubDstDelta?ubSrcDelta:ubDstDelta)+15) & 0xFFF0;
		uwBlitWords = uwBlitWidth >> 4;

		ubMaskFShift = ((ubWidthDelta+15)&0xF0)-ubWidthDelta;
		ubMaskLShift = uwBlitWidth - (wWidth+ubMaskFShift);
		uwFirstMask = 0xFFFF << ubMaskFShift;
		uwLastMask = 0xFFFF >> ubMaskLShift;
		if(ubMaskLShift > 16) // Fix for 2-word blits
			uwFirstMask &= 0xFFFF >> (ubMaskLShift-16);

		ubShift = uwBlitWidth - (ubDstDelta+wWidth+ubMaskFShift);
		uwBltCon1 = ubShift << BSHIFTSHIFT | BLITREVERSE;

		ulSrcOffs = pSrc->BytesPerRow * (wSrcY+wHeight-1) + ((wSrcX+wWidth+ubMaskFShift-1)>>3);
		ulDstOffs = pDst->BytesPerRow * (wDstY+wHeight-1) + ((wDstX+wWidth+ubMaskFShift-1) >> 3);
	}
	else {
		uwBlitWidth = (wWidth+ubDstDelta+15) & 0xFFF0;
		uwBlitWords = uwBlitWidth >> 4;

		ubMaskFShift = ubSrcDelta;
		ubMaskLShift = uwBlitWidth-(wWidth+ubSrcDelta);

		uwFirstMask = 0xFFFF >> ubMaskFShift;
		uwLastMask = 0xFFFF << ubMaskLShift;

		ubShift = ubDstDelta-ubSrcDelta;
		uwBltCon1 = ubShift << BSHIFTSHIFT;

		ulSrcOffs = pSrc->BytesPerRow * wSrcY + (wSrcX>>3);
		ulDstOffs = pDst->BytesPerRow * wDstY + (wDstX>>3);
	}

	uwBltCon0 = (ubShift << ASHIFTSHIFT) | USEB|USEC|USED | ubMinterm;
	wSrcModulo = pSrc->BytesPerRow - (uwBlitWords<<1);
	wDstModulo = pDst->BytesPerRow - (uwBlitWords<<1);

	ubMask &= 0xFF >> (8- (pSrc->Depth < pDst->Depth? pSrc->Depth: pDst->Depth));
	ubPlane = 0;
	blitWait();
	custom.bltcon0 = uwBltCon0;
	custom.bltcon1 = uwBltCon1;
	custom.bltafwm = uwFirstMask;
	custom.bltalwm = uwLastMask;
	custom.bltbmod = wSrcModulo;
	custom.bltcmod = wDstModulo;
	custom.bltdmod = wDstModulo;
	custom.bltadat = 0xFFFF;
	while(ubMask) {
		if(ubMask & 1) {
			blitWait();
			// This hell of a casting must stay here or else large offsets get bugged!
			custom.bltbpt = (UBYTE*)((ULONG)pSrc->Planes[ubPlane] + ulSrcOffs);
			custom.bltcpt = (UBYTE*)((ULONG)pDst->Planes[ubPlane] + ulDstOffs);
			custom.bltdpt = (UBYTE*)((ULONG)pDst->Planes[ubPlane] + ulDstOffs);

			custom.bltsize = (wHeight << 6) | uwBlitWords;
		}
		ubMask >>= 1;
		++ubPlane;
	}
#endif // AMIGA
	return 1;
}

UBYTE blitSafeCopy(
	tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubMinterm, UBYTE ubMask, UWORD uwLine, char *szFile
) {
	if(!blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile))
		return 0;
	return blitUnsafeCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm, ubMask);
}

/**
 * Very restrictive and fast blit variant
 * Works only with src/dst/width divisible by 16
 * Does not check if destination has less bitplanes than source
 * Best for drawing tilemaps
 */
UBYTE blitUnsafeCopyAligned(
	tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight
) {
	#ifdef AMIGA
	UWORD uwBlitWords, uwBltCon0;
	WORD wDstModulo, wSrcModulo;
	ULONG ulSrcOffs, ulDstOffs;

	uwBlitWords = wWidth >> 4;
	uwBltCon0 = USEA|USED | MINTERM_A;

	wSrcModulo = bitmapGetByteWidth(pSrc) - (uwBlitWords<<1);
	wDstModulo = bitmapGetByteWidth(pDst) - (uwBlitWords<<1);
	ulSrcOffs = pSrc->BytesPerRow * wSrcY + (wSrcX>>3);
	ulDstOffs = pDst->BytesPerRow * wDstY + (wDstX>>3);

	if(bitmapIsInterleaved(pSrc) && bitmapIsInterleaved(pDst)) {
		wHeight *= pSrc->Depth;
		blitWait();
		custom.bltcon0 = uwBltCon0;
		custom.bltcon1 = 0;
		custom.bltafwm = 0xFFFF;
		custom.bltalwm = 0xFFFF;

		custom.bltamod = wSrcModulo;
		custom.bltdmod = wDstModulo;

		// This hell of a casting must stay here or else large offsets get bugged!
		custom.bltapt = (UBYTE*)((ULONG)pSrc->Planes[0] + ulSrcOffs);
		custom.bltdpt = (UBYTE*)((ULONG)pDst->Planes[0] + ulDstOffs);

		custom.bltsize = (wHeight << 6) | uwBlitWords;
	}
	else {
		UBYTE ubPlane;

		if(bitmapIsInterleaved(pSrc) || bitmapIsInterleaved(pDst)) {
			// Since you're using this fn for speed
			logWrite("WARN: Mixed interleaved - you're losing lots of performance here!\n");
		}
		if(bitmapIsInterleaved(pSrc))
			wSrcModulo += pSrc->BytesPerRow * (pSrc->Depth-1);
		else if(bitmapIsInterleaved(pDst))
			wDstModulo += pDst->BytesPerRow * (pDst->Depth-1);

		blitWait();
		custom.bltcon0 = uwBltCon0;
		custom.bltcon1 = 0;
		custom.bltafwm = 0xFFFF;
		custom.bltalwm = 0xFFFF;

		custom.bltamod = wSrcModulo;
		custom.bltdmod = wDstModulo;
		for(ubPlane = pSrc->Depth; ubPlane--;) {
			blitWait();
			// This hell of a casting must stay here or else large offsets get bugged!
			custom.bltapt = (UBYTE*)(((ULONG)(pSrc->Planes[ubPlane])) + ulSrcOffs);
			custom.bltdpt = (UBYTE*)(((ULONG)(pDst->Planes[ubPlane])) + ulDstOffs);
			custom.bltsize = (wHeight << 6) | uwBlitWords;
		}
	}
#endif // AMIGA
	return 1;
}

UBYTE blitSafeCopyAligned(
	tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UWORD uwLine, char *szFile
) {
	if(!blitCheck(
		pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile
	))
		return 0;
	if((wSrcX | wDstX | wWidth) & 0x000F) {
		logWrite("Dimensions are not divisible by 16!\n");
		return 0;
	}
	return blitUnsafeCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight);
}

/**
 * Copies source data to destination over mask
 * Optimizations require following conditions:
 * - wSrcX < wDstX (shifts to right)
 * - mask must have same dimensions as source bitplane
 */
UBYTE blitUnsafeCopyMask(
	tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, UWORD *pMsk
) {
#ifdef AMIGA
	WORD wDstModulo, wSrcModulo;

	UBYTE ubSrcOffs = wSrcX & 0xF;
	UBYTE ubDstOffs = wDstX & 0xF;

	UWORD uwBlitWidth = (wWidth+ubDstOffs+15) & 0xFFF0;
	UWORD uwBlitWords = uwBlitWidth >> 4;

	UWORD uwFirstMask = 0xFFFF >> ubSrcOffs;
	UWORD uwLastMask = 0xFFFF << (uwBlitWidth-(wWidth+ubSrcOffs));

	UWORD uwBltCon1 = (ubDstOffs-ubSrcOffs) << BSHIFTSHIFT;
	UWORD uwBltCon0 = uwBltCon1 | USEA|USEB|USEC|USED | MINTERM_COOKIE;

	ULONG ulSrcOffs = pSrc->BytesPerRow * wSrcY + (wSrcX>>3);
	ULONG ulDstOffs = pDst->BytesPerRow * wDstY + (wDstX>>3);
	if(bitmapIsInterleaved(pSrc) && bitmapIsInterleaved(pDst)) {
		wSrcModulo = bitmapGetByteWidth(pSrc) - (uwBlitWords<<1);
		wDstModulo = bitmapGetByteWidth(pDst) - (uwBlitWords<<1);
		wHeight *= pSrc->Depth;

		blitWait();
		custom.bltcon0 = uwBltCon0;
		custom.bltcon1 = uwBltCon1;
		custom.bltafwm = uwFirstMask;
		custom.bltalwm = uwLastMask;

		custom.bltamod = wSrcModulo;
		custom.bltbmod = wSrcModulo;
		custom.bltcmod = wDstModulo;
		custom.bltdmod = wDstModulo;

		custom.bltapt = (UBYTE*)((ULONG)pMsk + ulSrcOffs);
		custom.bltbpt = (UBYTE*)((ULONG)pSrc->Planes[0] + ulSrcOffs);
		custom.bltcpt = (UBYTE*)((ULONG)pDst->Planes[0] + ulDstOffs);
		custom.bltdpt = (UBYTE*)((ULONG)pDst->Planes[0] + ulDstOffs);

		custom.bltsize = (wHeight << 6) | uwBlitWords;
	}
	else {
#ifdef GAME_DEBUG
		if(
			(bitmapIsInterleaved(pSrc) && !bitmapIsInterleaved(pDst)) ||
			(!bitmapIsInterleaved(pSrc) && bitmapIsInterleaved(pDst))
		) {
			logWrite("WARN: Inefficient blit via mask with %p, %p\n", pSrc, pDst);
		}
#endif // GAME_DEBUG
		wSrcModulo = pSrc->BytesPerRow - (uwBlitWords<<1);
		wDstModulo = pDst->BytesPerRow - (uwBlitWords<<1);
		blitWait();
		custom.bltcon0 = uwBltCon0;
		custom.bltcon1 = uwBltCon1;
		custom.bltafwm = uwFirstMask;
		custom.bltalwm = uwLastMask;

		custom.bltamod = wSrcModulo;
		custom.bltbmod = wSrcModulo;
		custom.bltcmod = wDstModulo;
		custom.bltdmod = wDstModulo;
		for(UBYTE ubPlane = pSrc->Depth; ubPlane--;) {
			blitWait();
			custom.bltapt = (UBYTE*)((ULONG)pMsk + ulSrcOffs);
			custom.bltbpt = (UBYTE*)((ULONG)pSrc->Planes[ubPlane] + ulSrcOffs);
			custom.bltcpt = (UBYTE*)((ULONG)pDst->Planes[ubPlane] + ulDstOffs);
			custom.bltdpt = (UBYTE*)((ULONG)pDst->Planes[ubPlane] + ulDstOffs);

			custom.bltsize = (wHeight << 6) | uwBlitWords;
		}
	}
	#endif // AMIGA
	return 1;
}

UBYTE blitSafeCopyMask(
	tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, UWORD *pMsk, UWORD uwLine, char *szFile
) {
	if(!blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile))
		return 0;
	return blitUnsafeCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk);
}

/**
 * Fills rectangular area with selected color
 * A - rectangle mask, read disabled
 * C - destination read
 * D - destination write
 * Each bitplane has minterm depending if rectangular area should be filled or erased:
 * 	- fill: D = A+C
 * - erase: D = (~A)*C
 */
UBYTE _blitRect(
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubColor, UWORD uwLine, char *szFile
) {
	if(!blitCheck(0,0,0,pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile))
		return 0;
#ifdef AMIGA

	// Helper vars
	UWORD uwBlitWords, uwBlitWidth;
	ULONG ulDstOffs;
	UBYTE ubDstDelta, ubMinterm, ubPlane;
	// Blitter register values
	UWORD uwBltCon0, uwBltCon1, uwFirstMask, uwLastMask;
	WORD wDstModulo;

	ubDstDelta = wDstX & 0xF;
	uwBlitWidth = (wWidth+ubDstDelta+15) & 0xFFF0;
	uwBlitWords = uwBlitWidth >> 4;

	uwFirstMask = 0xFFFF >> ubDstDelta;
	uwLastMask = 0xFFFF << (uwBlitWidth-(wWidth+ubDstDelta));
	uwBltCon1 = 0;
	ulDstOffs = pDst->BytesPerRow * wDstY + (wDstX>>3);
	wDstModulo = pDst->BytesPerRow - (uwBlitWords<<1);
	uwBltCon0 = USEC|USED;

	blitWait();
	custom.bltcon1 = uwBltCon1;
	custom.bltafwm = uwFirstMask;
	custom.bltalwm = uwLastMask;

	custom.bltcmod = wDstModulo;
	custom.bltdmod = wDstModulo;
	custom.bltadat = 0xFFFF;
	custom.bltbdat = 0;
	ubPlane = 0;

	do {
		if(ubColor & 1)
			ubMinterm = 0xFA;
		else
			ubMinterm = 0x0A;
		blitWait();
		custom.bltcon0 = uwBltCon0 | ubMinterm;
		// This hell of a casting must stay here or else large offsets get bugged!
		custom.bltcpt = (UBYTE*)((ULONG)pDst->Planes[ubPlane] + ulDstOffs);
		custom.bltdpt = (UBYTE*)((ULONG)pDst->Planes[ubPlane] + ulDstOffs);
		custom.bltsize = (wHeight << 6) | uwBlitWords;
		ubColor >>= 1;
		++ubPlane;
	}	while(ubPlane != pDst->Depth);

#endif // AMIGA
	return 1;
}

void blitLine(
	tBitMap *pDst, WORD x1, WORD y1, WORD x2, WORD y2,
	UBYTE ubColor, UWORD uwPattern, UBYTE isOneDot
) {
#ifdef AMIGA
	// Based on Cahir's function from:
	// https://github.com/cahirwpz/demoscene/blob/master/a500/base/libsys/blt-line.c

	UWORD uwBltCon1 = LINEMODE;
	if(isOneDot)
		uwBltCon1 |= ONEDOT;

	// Always draw the line downwards.
	if (y1 > y2) {
		SWAP(x1, x2);
		SWAP(y1, y2);
	}

	// Word containing the first pixel of the line.
	WORD wDx = x2 - x1;
	WORD wDy = y2 - y1;

	// Setup octant bits
	if (wDx < 0) {
		wDx = -wDx;
		if (wDx >= wDy) {
			uwBltCon1 |= AUL | SUD;
		}
		else {
			uwBltCon1 |= SUL;
			SWAP(wDx, wDy);
		}
	}
	else {
		if (wDx >= wDy) {
			uwBltCon1 |= SUD;
		}
		else {
			SWAP(wDx, wDy);
		}
	}

	WORD wDerr = wDy + wDy - wDx;
	if (wDerr < 0) {
		uwBltCon1 |= SIGNFLAG;
	}

	UWORD uwBltSize = (wDx << 6) + 66;
	UWORD uwBltCon0 = ror16(x1&15, 4);
	ULONG ulDataOffs = pDst->BytesPerRow * y1 + (x1 >> 3) & ~1;
	blitWait();
	custom.bltafwm = -1;
	custom.bltalwm = -1;
	custom.bltadat = 0x8000;
	custom.bltbdat = uwPattern;
	custom.bltamod = wDerr - wDx;
	custom.bltbmod = wDy + wDy;
	custom.bltcmod = pDst->BytesPerRow;
	custom.bltdmod = pDst->BytesPerRow;
	custom.bltcon1 = uwBltCon1;
	custom.bltapt = (APTR)(LONG)wDerr;
	for(UBYTE ubPlane = 0; ubPlane != pDst->Depth; ++ubPlane) {
		UBYTE *pData = pDst->Planes[ubPlane] + ulDataOffs;
		UWORD uwOp = ((ubColor & BV(ubPlane)) ? BLIT_LINE_OR : BLIT_LINE_ERASE);

		blitWait();
		custom.bltcon0 = uwBltCon0 | uwOp;
		custom.bltcpt = pData;
		custom.bltdpt = (APTR)(isOneDot ? pDst->Planes[pDst->Depth] : pData);
		custom.bltsize = uwBltSize;
	}
#else
#error "Unimplemented: blitLine()"
#endif // AMIGa
}
