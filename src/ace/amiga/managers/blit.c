/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/blit.h>
#include <ace/managers/system.h>

// BltCon0 channel enable bits
#define USEA 0x800
#define USEB 0x400
#define USEC 0x200
#define USED 0x100

#define BLIT_LINE_OR ((ABC | ABNC | NABC | NANBC) | (SRCA | SRCC | DEST))
#define BLIT_LINE_XOR ((ABNC | NABC | NANBC) | (SRCA | SRCC | DEST))
#define BLIT_LINE_ERASE ((NABC | NANBC | ANBC) | (SRCA | SRCC | DEST))

void blitManagerCreate(void) {
	logBlockBegin("blitManagerCreate");
	systemSetDmaBit(DMAB_BLITTER, 1);
	logBlockEnd("blitManagerCreate");
}

void blitManagerDestroy(void) {
	logBlockBegin("blitManagerDestroy");
	systemSetDmaBit(DMAB_BLITTER, 0);
	logBlockEnd("blitManagerDestroy");
}

void blitWait(void) {
	while(!blitIsIdle()) continue;
}

/**
 * Checks if blitter is idle
 * Polls 2 times - A1000 Agnus bug workaround
 */
UBYTE blitIsIdle(void) {
	if(!(g_pCustom->dmaconr & DMAF_BLTDONE)) {
		if(!(g_pCustom->dmaconr & DMAF_BLTDONE)) {
			return 1;
		}
	}
	return 0;
}

UBYTE blitUnsafeCopy(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubMinterm
) {
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

	ubPlane = MIN(pSrc->Depth, pDst->Depth);
	blitWait(); // Don't modify registers when other blit is in progress
	g_pCustom->bltcon0 = uwBltCon0;
	g_pCustom->bltcon1 = uwBltCon1;
	g_pCustom->bltafwm = uwFirstMask;
	g_pCustom->bltalwm = uwLastMask;
	g_pCustom->bltbmod = wSrcModulo;
	g_pCustom->bltcmod = wDstModulo;
	g_pCustom->bltdmod = wDstModulo;
	g_pCustom->bltadat = 0xFFFF;
	while(ubPlane--) {
		blitWait();
		// This hell of a casting must stay here or else large offsets get bugged!
		g_pCustom->bltbpt = pSrc->Planes[ubPlane] + ulSrcOffs;
		g_pCustom->bltcpt = pDst->Planes[ubPlane] + ulDstOffs;
		g_pCustom->bltdpt = pDst->Planes[ubPlane] + ulDstOffs;

		g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
	}

	return 1;
}

/**
 * Very restrictive and fast blit variant
 * Works only with src/dst/width divisible by 16
 * Does not check if destination has less bitplanes than source
 * Best for drawing tilemaps
 */
UBYTE blitUnsafeCopyAligned(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight
) {
	UWORD uwBlitWords, uwBltCon0;
	WORD wDstModulo, wSrcModulo;
	ULONG ulSrcOffs, ulDstOffs;

	// Use C channel instead of A - same speed, less regs to set up
	uwBlitWords = wWidth >> 4;
	uwBltCon0 = USEC|USED | MINTERM_C;

	wSrcModulo = bitmapGetByteWidth(pSrc) - (uwBlitWords<<1);
	wDstModulo = bitmapGetByteWidth(pDst) - (uwBlitWords<<1);
	ulSrcOffs = pSrc->BytesPerRow * wSrcY + (wSrcX>>3);
	ulDstOffs = pDst->BytesPerRow * wDstY + (wDstX>>3);

	if(bitmapIsInterleaved(pSrc) && bitmapIsInterleaved(pDst)) {
		wHeight *= pSrc->Depth;
		blitWait(); // Don't modify registers when other blit is in progress
		g_pCustom->bltcon0 = uwBltCon0;
		g_pCustom->bltcon1 = 0;

		g_pCustom->bltcmod = wSrcModulo;
		g_pCustom->bltdmod = wDstModulo;

		g_pCustom->bltcpt = pSrc->Planes[0] + ulSrcOffs;
		g_pCustom->bltdpt = pDst->Planes[0] + ulDstOffs;

		g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
	}
	else {
		UBYTE ubPlane;

		if(bitmapIsInterleaved(pSrc) || bitmapIsInterleaved(pDst)) {
			// Since you're using this fn for speed
			logWrite("WARN: Mixed interleaved - you're losing lots of performance here!\n");
		}
		if(bitmapIsInterleaved(pSrc)) {
			wSrcModulo += pSrc->BytesPerRow * (pSrc->Depth-1);
		}
		else if(bitmapIsInterleaved(pDst)) {
			wDstModulo += pDst->BytesPerRow * (pDst->Depth-1);
		}

		blitWait(); // Don't modify registers when other blit is in progress
		g_pCustom->bltcon0 = uwBltCon0;
		g_pCustom->bltcon1 = 0;

		g_pCustom->bltcmod = wSrcModulo;
		g_pCustom->bltdmod = wDstModulo;
		for(ubPlane = pSrc->Depth; ubPlane--;) {
			blitWait();
			g_pCustom->bltcpt = pSrc->Planes[ubPlane] + ulSrcOffs;
			g_pCustom->bltdpt = pDst->Planes[ubPlane] + ulDstOffs;
			g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
		}
	}
	return 1;
}

/**
 * Copies source data to destination over mask
 * Optimizations require following conditions:
 * - wSrcX <= wDstX (shifts to right)
 * - mask must have same dimensions as source bitplane
 */
UBYTE blitUnsafeCopyMask(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, const UBYTE *pMsk
) {
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

		blitWait(); // Don't modify registers when other blit is in progress
		g_pCustom->bltcon0 = uwBltCon0;
		g_pCustom->bltcon1 = uwBltCon1;
		g_pCustom->bltafwm = uwFirstMask;
		g_pCustom->bltalwm = uwLastMask;

		g_pCustom->bltamod = wSrcModulo;
		g_pCustom->bltbmod = wSrcModulo;
		g_pCustom->bltcmod = wDstModulo;
		g_pCustom->bltdmod = wDstModulo;

		g_pCustom->bltapt = (UBYTE*)(pMsk + ulSrcOffs);
		g_pCustom->bltbpt = pSrc->Planes[0] + ulSrcOffs;
		g_pCustom->bltcpt = pDst->Planes[0] + ulDstOffs;
		g_pCustom->bltdpt = pDst->Planes[0] + ulDstOffs;

		g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
	}
	else {
#ifdef ACE_DEBUG
		if(
			(bitmapIsInterleaved(pSrc) && !bitmapIsInterleaved(pDst)) ||
			(!bitmapIsInterleaved(pSrc) && bitmapIsInterleaved(pDst))
		) {
			logWrite("WARN: Inefficient blit via mask with %p, %p\n", pSrc, pDst);
		}
#endif // ACE_DEBUG
		wSrcModulo = pSrc->BytesPerRow - (uwBlitWords<<1);
		wDstModulo = pDst->BytesPerRow - (uwBlitWords<<1);
		blitWait(); // Don't modify registers when other blit is in progress
		g_pCustom->bltcon0 = uwBltCon0;
		g_pCustom->bltcon1 = uwBltCon1;
		g_pCustom->bltafwm = uwFirstMask;
		g_pCustom->bltalwm = uwLastMask;

		g_pCustom->bltamod = wSrcModulo;
		g_pCustom->bltbmod = wSrcModulo;
		g_pCustom->bltcmod = wDstModulo;
		g_pCustom->bltdmod = wDstModulo;
		for(UBYTE ubPlane = pSrc->Depth; ubPlane--;) {
			blitWait();
			g_pCustom->bltapt = (UBYTE*)(pMsk + ulSrcOffs);
			g_pCustom->bltbpt = pSrc->Planes[ubPlane] + ulSrcOffs;
			g_pCustom->bltcpt = pDst->Planes[ubPlane] + ulDstOffs;
			g_pCustom->bltdpt = pDst->Planes[ubPlane] + ulDstOffs;

			g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
		}
	}
	return 1;
}

UBYTE blitUnsafeRect(
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubColor
) {
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

	blitWait(); // Don't modify registers when other blit is in progress
	g_pCustom->bltcon1 = uwBltCon1;
	g_pCustom->bltafwm = uwFirstMask;
	g_pCustom->bltalwm = uwLastMask;

	g_pCustom->bltcmod = wDstModulo;
	g_pCustom->bltdmod = wDstModulo;
	g_pCustom->bltadat = 0xFFFF;
	g_pCustom->bltbdat = 0;
	ubPlane = 0;

	do {
		// Assign minterm depending if bitplane area should be filled or erased
		ubMinterm = (ubColor & 1) ? MINTERM_A_OR_C : MINTERM_NAC;
		blitWait();
		g_pCustom->bltcon0 = uwBltCon0 | ubMinterm;
		// This hell of a casting must stay here or else large offsets get bugged!
		g_pCustom->bltcpt = pDst->Planes[ubPlane] + ulDstOffs;
		g_pCustom->bltdpt = pDst->Planes[ubPlane] + ulDstOffs;
		g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
		ubColor >>= 1;
		++ubPlane;
	}	while(ubPlane < pDst->Depth);

	return 1;
}

void blitLine(
	tBitMap *pDst, WORD x1, WORD y1, WORD x2, WORD y2,
	UBYTE ubColor, UWORD uwPattern, UBYTE isOneDot
) {
	// Based on Cahir's function from:
	// https://github.com/cahirwpz/demoscene/blob/master/a500/base/libsys/blt-line.c

	UWORD uwBltCon1 = LINEMODE;
	if(isOneDot) {
		uwBltCon1 |= ONEDOT;
	}

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

	UWORD uwBltSize = (wDx << HSIZEBITS) + 66;
	UWORD uwBltCon0 = ror16(x1&15, 4);
	ULONG ulDataOffs = pDst->BytesPerRow * y1 + ((x1 >> 3) & ~1);
	blitWait(); // Don't modify registers when other blit is in progress
	g_pCustom->bltafwm = -1;
	g_pCustom->bltalwm = -1;
	g_pCustom->bltadat = 0x8000;
	g_pCustom->bltbdat = uwPattern;
	g_pCustom->bltamod = wDerr - wDx;
	g_pCustom->bltbmod = wDy + wDy;
	g_pCustom->bltcmod = pDst->BytesPerRow;
	g_pCustom->bltdmod = pDst->BytesPerRow;
	g_pCustom->bltcon1 = uwBltCon1;
	g_pCustom->bltapt = (APTR)(LONG)wDerr;
	for(UBYTE ubPlane = 0; ubPlane != pDst->Depth; ++ubPlane) {
		UBYTE *pData = pDst->Planes[ubPlane] + ulDataOffs;
		UWORD uwOp = ((ubColor & BV(ubPlane)) ? BLIT_LINE_OR : BLIT_LINE_ERASE);

		blitWait();
		g_pCustom->bltcon0 = uwBltCon0 | uwOp;
		g_pCustom->bltcpt = pData;
		g_pCustom->bltdpt = (APTR)(isOneDot ? pDst->Planes[pDst->Depth] : pData);
		g_pCustom->bltsize = uwBltSize;
	}
}
