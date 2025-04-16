/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/blit.h>
#include <ace/managers/system.h>

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

#if defined(ACE_DEBUG)
UBYTE _blitCheck(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	const tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UWORD uwLine, const char *szFile
) {
	WORD wSrcWidth, wSrcHeight, wDstWidth, wDstHeight;

	UBYTE isErr = 0;
	if(pSrc) {
		if(!bitmapIsChip(pSrc)) {
			isErr = 1;
			logWrite("ERR: Source is in FAST mem: %p (%p)\n", pSrc, pSrc->Planes[0]);
		}
		wSrcWidth = pSrc->BytesPerRow << 3;
		if(bitmapIsInterleaved(pSrc)) {
			wSrcWidth /= pSrc->Depth;
		}
		wSrcHeight = pSrc->Rows;
	}
	else {
		wSrcWidth = 0;
		wSrcHeight = 0;
	}

	if(pDst) {
		if(!bitmapIsChip(pDst)) {
			isErr = 1;
			logWrite("ERR: Dest is in FAST mem: %p (%p)\n", pDst, pDst->Planes[0]);
		}
		wDstWidth = pDst->BytesPerRow << 3;
		if(bitmapIsInterleaved(pDst)) {
			wDstWidth /= pDst->Depth;
		}
		wDstHeight = pDst->Rows;
	}
	else {
		wDstWidth = 0;
		wDstHeight = 0;
	}

	if(isErr) {
		return 1;
	}

	if(pSrc && (wSrcX < 0 || wSrcWidth < wSrcX + wWidth || wSrcHeight < wSrcY + wHeight)) {
		logWrite(
			"ERR: ILLEGAL BLIT Source out of range: "
			"source %p %dx%d, dest: %p %dx%d, blit: %d,%d -> %d,%d %dx%d (%s:%u)\n",
			pSrc,	wSrcWidth, wSrcHeight, pDst, wDstWidth, wDstHeight,
			wSrcX, wSrcY, wDstX, wDstY, wWidth, wHeight, szFile, uwLine
		);
		return 0;
	}
	if(pDst && (wDstY < 0 || wDstWidth < wDstX + wWidth || wDstHeight < wDstY + wHeight)) {
		logWrite(
			"ERR: ILLEGAL BLIT Dest out of range: "
			"source %p %dx%d, dest: %p %dx%d, blit: %d,%d -> %d,%d %dx%d (%s:%u)\n",
			pSrc,	wSrcWidth, wSrcHeight, pDst, wDstWidth, wDstHeight,
			wSrcX, wSrcY, wDstX, wDstY, wWidth, wHeight, szFile, uwLine
		);
		return 0;
	}

#if defined(ACE_USE_ECS_FEATURES)
	UWORD uwMaxBlitWidth = 32768;
#else
	UWORD uwMaxBlitWidth = 1024;
#endif

	if(pSrc && pDst && bitmapIsInterleaved(pSrc) && bitmapIsInterleaved(pDst)) {
		if(wHeight * pSrc->Depth > uwMaxBlitWidth) {
			logWrite(
				"ERR: Blit too big: height %hd, depth: %hhu, interleaved: %d (%s:%u)\n",
				wHeight, pSrc->Depth, wHeight * pSrc->Depth, szFile, uwLine
			);
		}
	}

	return 1;
}
#endif // defined(ACE_DEBUG)

void blitWait(void) {
	// A1000 Blitter done bug:
	// The solution is to read hardware register before testing the bit.
	(void)g_pCustom->dmaconr;
	while(g_pCustom->dmaconr & DMAF_BLTDONE) continue;
}

UBYTE blitIsIdle(void) {
	// A1000 Blitter done bug:
	// The solution is to read hardware register before testing the bit.
	(void)g_pCustom->dmaconr;
	if(g_pCustom->dmaconr & DMAF_BLTDONE) {
		return 0;
	}
	return 1;
}

UBYTE blitUnsafeCopy(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubMinterm
) {
	// Helper vars
	UWORD uwBlitWords, uwBlitWidth;
	ULONG ulSrcOffs, ulDstOffs;
	UBYTE ubShift, ubSrcDelta, ubDstDelta, ubWidthDelta, ubMaskFShift, ubMaskLShift;
	// Blitter register values
	UWORD uwBltCon0, uwBltCon1, uwFirstMask, uwLastMask;
	WORD wSrcModulo, wDstModulo;

	ubSrcDelta = wSrcX & 0xF;
	ubDstDelta = wDstX & 0xF;
	ubWidthDelta = (ubSrcDelta + wWidth) & 0xF;
	UBYTE isBlitInterleaved = (
		bitmapIsInterleaved(pSrc) && bitmapIsInterleaved(pDst) &&
		pSrc->Depth == pDst->Depth
	);

	if(ubSrcDelta > ubDstDelta || ((wWidth+ubDstDelta+15) & 0xFFF0)-(wWidth+ubSrcDelta) > 16) {
		uwBlitWidth = (wWidth+(ubSrcDelta>ubDstDelta?ubSrcDelta:ubDstDelta)+15) & 0xFFF0;
		uwBlitWords = uwBlitWidth >> 4;

		ubMaskFShift = ((ubWidthDelta+15)&0xF0)-ubWidthDelta;
		ubMaskLShift = uwBlitWidth - (wWidth+ubMaskFShift);
		uwFirstMask = 0xFFFF << ubMaskFShift;
		uwLastMask = 0xFFFF >> ubMaskLShift;
		if(ubMaskLShift > 16) { // Fix for 2-word blits
			uwFirstMask &= 0xFFFF >> (ubMaskLShift-16);
		}

		ubShift = uwBlitWidth - (ubDstDelta+wWidth+ubMaskFShift);
		uwBltCon1 = (ubShift << BSHIFTSHIFT) | BLITREVERSE;

		// Position on the end of last row of the bitmap.
		// For interleaved, position on the last row of last bitplane.
		if(isBlitInterleaved) {
			// TODO: fix duplicating bitmapIsInterleaved() check inside bitmapGetByteWidth()
			ulSrcOffs = pSrc->BytesPerRow * (wSrcY + wHeight) - bitmapGetByteWidth(pSrc) + ((wSrcX + wWidth + ubMaskFShift - 1) / 16) * 2;
			ulDstOffs = pDst->BytesPerRow * (wDstY + wHeight) - bitmapGetByteWidth(pDst) + ((wDstX + wWidth + ubMaskFShift - 1) / 16) * 2;
		}
		else {
			ulSrcOffs = pSrc->BytesPerRow * (wSrcY + wHeight - 1) + ((wSrcX + wWidth + ubMaskFShift - 1) / 16) * 2;
			ulDstOffs = pDst->BytesPerRow * (wDstY + wHeight - 1) + ((wDstX + wWidth + ubMaskFShift - 1) / 16) * 2;
		}
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

		ulSrcOffs = pSrc->BytesPerRow * wSrcY + (wSrcX >> 3);
		ulDstOffs = pDst->BytesPerRow * wDstY + (wDstX >> 3);
	}

	uwBltCon0 = (ubShift << ASHIFTSHIFT) | USEB|USEC|USED | ubMinterm;

	if(isBlitInterleaved) {
		wHeight *= pSrc->Depth;
		wSrcModulo = bitmapGetByteWidth(pSrc) - uwBlitWords * 2;
		wDstModulo = bitmapGetByteWidth(pDst) - uwBlitWords * 2;

		blitWait(); // Don't modify registers when other blit is in progress
		g_pCustom->bltcon0 = uwBltCon0;
		g_pCustom->bltcon1 = uwBltCon1;
		g_pCustom->bltafwm = uwFirstMask;
		g_pCustom->bltalwm = uwLastMask;
		g_pCustom->bltbmod = wSrcModulo;
		g_pCustom->bltcmod = wDstModulo;
		g_pCustom->bltdmod = wDstModulo;
		g_pCustom->bltadat = 0xFFFF;
		g_pCustom->bltbpt = &pSrc->Planes[0][ulSrcOffs];
		g_pCustom->bltcpt = &pDst->Planes[0][ulDstOffs];
		g_pCustom->bltdpt = &pDst->Planes[0][ulDstOffs];
#if defined(ACE_USE_ECS_FEATURES)
		g_pCustom->bltsizv = wHeight;
		g_pCustom->bltsizh = uwBlitWords;
#else
		g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
#endif
	}
	else {
		wSrcModulo = pSrc->BytesPerRow - uwBlitWords * 2;
		wDstModulo = pDst->BytesPerRow - uwBlitWords * 2;
		UBYTE ubPlane = MIN(pSrc->Depth, pDst->Depth);

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
			g_pCustom->bltbpt = &pSrc->Planes[ubPlane][ulSrcOffs];
			g_pCustom->bltcpt = &pDst->Planes[ubPlane][ulDstOffs];
			g_pCustom->bltdpt = &pDst->Planes[ubPlane][ulDstOffs];

#if defined(ACE_USE_ECS_FEATURES)
			g_pCustom->bltsizv = wHeight;
			g_pCustom->bltsizh = uwBlitWords;
#else
			g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
#endif
		}
	}

	return 1;
}

UBYTE blitSafeCopy(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubMinterm, UWORD uwLine, const char *szFile
) {
	if(!blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile)) {
		return 0;
	}
	return blitUnsafeCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm);
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
	// Use C channel instead of A - same speed, less regs to set up
	UWORD uwBlitWords = wWidth / 16;
	UWORD uwBltCon0 = USEC|USED | MINTERM_C;
	ULONG ulSrcOffs = pSrc->BytesPerRow * wSrcY + (wSrcX / 8);
	ULONG ulDstOffs = pDst->BytesPerRow * wDstY + (wDstX / 8);

	if(bitmapIsInterleaved(pSrc) && bitmapIsInterleaved(pDst)) {
		WORD wSrcModulo = bitmapGetByteWidth(pSrc) - uwBlitWords * 2;
		WORD wDstModulo = bitmapGetByteWidth(pDst) - uwBlitWords * 2;
		wHeight *= pSrc->Depth;

		blitWait(); // Don't modify registers when other blit is in progress
		g_pCustom->bltcon0 = uwBltCon0;
		g_pCustom->bltcon1 = 0;
		g_pCustom->bltcmod = wSrcModulo;
		g_pCustom->bltdmod = wDstModulo;
		g_pCustom->bltcpt = &pSrc->Planes[0][ulSrcOffs];
		g_pCustom->bltdpt = &pDst->Planes[0][ulDstOffs];
#if defined(ACE_USE_ECS_FEATURES)
		g_pCustom->bltsizv = wHeight;
		g_pCustom->bltsizh = uwBlitWords;
#else
		g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
#endif

	}
	else {
		if(bitmapIsInterleaved(pSrc) || bitmapIsInterleaved(pDst)) {
			// Since you're using this fn for speed
			logWrite("WARN: Mixed interleaved - you're losing lots of performance here\n");
		}

		WORD wSrcModulo = pSrc->BytesPerRow - (uwBlitWords<<1);
		WORD wDstModulo = pDst->BytesPerRow - (uwBlitWords<<1);
		UBYTE ubPlane = pSrc->Depth;

		blitWait(); // Don't modify registers when other blit is in progress
		g_pCustom->bltcon0 = uwBltCon0;
		g_pCustom->bltcon1 = 0;
		g_pCustom->bltcmod = wSrcModulo;
		g_pCustom->bltdmod = wDstModulo;
		while(ubPlane--) {
			blitWait();
			g_pCustom->bltcpt = &pSrc->Planes[ubPlane][ulSrcOffs];
			g_pCustom->bltdpt = &pDst->Planes[ubPlane][ulDstOffs];
#if defined(ACE_USE_ECS_FEATURES)
			g_pCustom->bltsizv = wHeight;
			g_pCustom->bltsizh = uwBlitWords;
#else
			g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
#endif
		}
	}

	return 1;
}

UBYTE blitSafeCopyAligned(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UWORD uwLine, const char *szFile
) {
	if((wSrcX | wDstX | wWidth) & 0x000F) {
		logWrite("ERR: Dimensions are not divisible by 16\n");
		return 0;
	}
	if(!blitCheck(
		pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile
	)) {
		return 0;
	}

	if(bitmapIsInterleaved(pSrc) && bitmapIsInterleaved(pDst) && pSrc->Depth != pDst->Depth) {
		logWrite(
			"ERR: bitmap BPP mismatch on interleaved blit! src: %hhu, dst: %hhu\n",
			pSrc->Depth, pDst->Depth
		);
		return 0;
	}

	return blitUnsafeCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight);
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
	// Helper vars
	UWORD uwBlitWords, uwBlitWidth;
	ULONG ulSrcOffs, ulDstOffs;
	UBYTE ubShift, ubSrcDelta, ubDstDelta, ubWidthDelta, ubMaskFShift, ubMaskLShift;
	// Blitter register values
	UWORD uwBltCon0, uwBltCon1, uwFirstMask, uwLastMask;
	WORD wSrcModulo, wDstModulo;

	ubSrcDelta = wSrcX & 0xF;
	ubDstDelta = wDstX & 0xF;
	ubWidthDelta = (ubSrcDelta + wWidth) & 0xF;
	UBYTE isBlitInterleaved = (
		bitmapIsInterleaved(pSrc) && bitmapIsInterleaved(pDst) &&
		pSrc->Depth == pDst->Depth
	);

	if(ubSrcDelta > ubDstDelta || ((wWidth+ubDstDelta+15) & 0xFFF0)-(wWidth+ubSrcDelta) > 16) {
		uwBlitWidth = (wWidth+(ubSrcDelta>ubDstDelta?ubSrcDelta:ubDstDelta)+15) & 0xFFF0;
		uwBlitWords = uwBlitWidth >> 4;

		ubMaskFShift = ((ubWidthDelta+15)&0xF0)-ubWidthDelta;
		ubMaskLShift = uwBlitWidth - (wWidth+ubMaskFShift);

		// Position on the end of last row of the bitmap.
		// For interleaved, position on the last row of last bitplane.
		if(isBlitInterleaved) {
			// TODO: fix duplicating bitmapIsInterleaved() check inside bitmapGetByteWidth()
			ulSrcOffs = pSrc->BytesPerRow * (wSrcY + wHeight) - bitmapGetByteWidth(pSrc) + ((wSrcX + wWidth + ubMaskFShift - 1) / 16) * 2;
			ulDstOffs = pDst->BytesPerRow * (wDstY + wHeight) - bitmapGetByteWidth(pDst) + ((wDstX + wWidth + ubMaskFShift - 1) / 16) * 2;
		}
		else {
			ulSrcOffs = pSrc->BytesPerRow * (wSrcY + wHeight - 1) + ((wSrcX + wWidth + ubMaskFShift - 1) / 16) * 2;
			ulDstOffs = pDst->BytesPerRow * (wDstY + wHeight - 1) + ((wDstX + wWidth + ubMaskFShift - 1) / 16) * 2;
		}

		uwFirstMask = 0xFFFF << ubMaskFShift;
		uwLastMask = 0xFFFF >> ubMaskLShift;
		if(ubMaskLShift > 16) { // Fix for 2-word blits
			uwFirstMask &= 0xFFFF >> (ubMaskLShift-16);
		}

		ubShift = uwBlitWidth - (ubDstDelta+wWidth+ubMaskFShift);
		uwBltCon1 = (ubShift << BSHIFTSHIFT) | BLITREVERSE;
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

		ulSrcOffs = pSrc->BytesPerRow * wSrcY + (wSrcX >> 3);
		ulDstOffs = pDst->BytesPerRow * wDstY + (wDstX >> 3);
	}

	 uwBltCon0 = uwBltCon1 |USEA|USEB|USEC|USED | MINTERM_COOKIE;

	if(isBlitInterleaved) {
		wHeight *= pSrc->Depth;
		wSrcModulo = bitmapGetByteWidth(pSrc) - uwBlitWords * 2;
		wDstModulo = bitmapGetByteWidth(pDst) - uwBlitWords * 2;

		blitWait(); // Don't modify registers when other blit is in progress
		g_pCustom->bltcon0 = uwBltCon0;
		g_pCustom->bltcon1 = uwBltCon1;
		g_pCustom->bltafwm = uwFirstMask;
		g_pCustom->bltalwm = uwLastMask;
		g_pCustom->bltamod = wSrcModulo;
		g_pCustom->bltbmod = wSrcModulo;
		g_pCustom->bltcmod = wDstModulo;
		g_pCustom->bltdmod = wDstModulo;
		g_pCustom->bltapt = (APTR)&pMsk[ulSrcOffs];
		g_pCustom->bltbpt = &pSrc->Planes[0][ulSrcOffs];
		g_pCustom->bltcpt = &pDst->Planes[0][ulDstOffs];
		g_pCustom->bltdpt = &pDst->Planes[0][ulDstOffs];
#if defined(ACE_USE_ECS_FEATURES)
		g_pCustom->bltsizv = wHeight;
		g_pCustom->bltsizh = uwBlitWords;
#else
		g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
#endif
	}
	else {
		wSrcModulo = pSrc->BytesPerRow - uwBlitWords * 2;
		wDstModulo = pDst->BytesPerRow - uwBlitWords * 2;
		UBYTE ubPlane = MIN(pSrc->Depth, pDst->Depth);

		blitWait(); // Don't modify registers when other blit is in progress
		g_pCustom->bltcon0 = uwBltCon0;
		g_pCustom->bltcon1 = uwBltCon1;
		g_pCustom->bltafwm = uwFirstMask;
		g_pCustom->bltalwm = uwLastMask;
		g_pCustom->bltapt = (APTR)&pMsk[ulSrcOffs];
		g_pCustom->bltamod = wSrcModulo;
		g_pCustom->bltbmod = wSrcModulo;
		g_pCustom->bltcmod = wDstModulo;
		g_pCustom->bltdmod = wDstModulo;

		while(ubPlane--) {
			blitWait();
			// This hell of a casting must stay here or else large offsets get bugged!
			g_pCustom->bltapt = (APTR)&pMsk[ulSrcOffs];
			g_pCustom->bltbpt = &pSrc->Planes[ubPlane][ulSrcOffs];
			g_pCustom->bltcpt = &pDst->Planes[ubPlane][ulDstOffs];
			g_pCustom->bltdpt = &pDst->Planes[ubPlane][ulDstOffs];

#if defined(ACE_USE_ECS_FEATURES)
			g_pCustom->bltsizv = wHeight;
			g_pCustom->bltsizh = uwBlitWords;
#else
			g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
#endif
		}
	}

	return 1;
}

UBYTE blitSafeCopyMask(
	const tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, const UBYTE *pMsk, UWORD uwLine, const char *szFile
) {
	if(!blitCheck(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile)) {
		return 0;
	}
	return blitUnsafeCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk);
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
		ubMinterm = (ubColor & 1) ? MINTERM_A_OR_C : MINTERM_NA_AND_C;
		blitWait();
		g_pCustom->bltcon0 = uwBltCon0 | ubMinterm;
		// This hell of a casting must stay here or else large offsets get bugged!
		g_pCustom->bltcpt = pDst->Planes[ubPlane] + ulDstOffs;
		g_pCustom->bltdpt = pDst->Planes[ubPlane] + ulDstOffs;
#if defined(ACE_USE_ECS_FEATURES)
		g_pCustom->bltsizv = wHeight;
		g_pCustom->bltsizh = uwBlitWords;
#else
		g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
#endif
		ubColor >>= 1;
		++ubPlane;
	}	while(ubPlane < pDst->Depth);
	return 1;
}

UBYTE blitSafeRect(
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubColor, UWORD uwLine, const char *szFile
) {
	if(!blitCheck(0,0,0,pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile)) {
		return 0;
	}

	return blitUnsafeRect(pDst, wDstX, wDstY, wWidth, wHeight, ubColor);
}

void blitUnsafeFillAligned(
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubPlane, UBYTE ubFillMode
) {
	UWORD uwBlitWords = wWidth / 16;

	UWORD uwWordMaskA = 0xFFFF;
	ULONG ulDstOffs = pDst->BytesPerRow * (wDstY + wHeight - 1) + ((wDstX + wWidth) / 8) - sizeof(UWORD);
	WORD wDstModulo = pDst->BytesPerRow - (uwBlitWords * 2);
	UWORD uwBltCon0 = USEA | USED | MINTERM_A;
	UWORD uwBltCon1 = BLITREVERSE | ubFillMode;
	UBYTE *pPlaneOffset = pDst->Planes[ubPlane] + ulDstOffs;

	blitWait(); // Don't modify registers when other blit is in progress
	g_pCustom->bltcon1 = uwBltCon1;
	g_pCustom->bltafwm = uwWordMaskA;
	g_pCustom->bltalwm = uwWordMaskA;

	g_pCustom->bltamod = wDstModulo;
	g_pCustom->bltdmod = wDstModulo;

	g_pCustom->bltcon0 = uwBltCon0;
	// This hell of a casting must stay here or else large offsets get bugged!
	g_pCustom->bltapt = pPlaneOffset;
	g_pCustom->bltdpt = pPlaneOffset;
#if defined(ACE_USE_ECS_FEATURES)
	g_pCustom->bltsizv = wHeight;
	g_pCustom->bltsizh = uwBlitWords;
#else
	g_pCustom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
#endif
}

UBYTE blitSafeFillAligned(
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubPlane, UBYTE ubFillMode, UWORD uwLine, const char *szFile
) {
	if((wDstX | wWidth) & 0x000F) {
		logWrite("ERR: Dimensions are not divisible by 16\n");
		return 0;
	}
	if(!blitCheck(0,0,0,pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile)) {
		return 0;
	}

	blitUnsafeFillAligned(pDst, wDstX, wDstY, wWidth, wHeight, ubPlane, ubFillMode);
	return 1;
}

void blitLine(
	tBitMap *pDst, WORD wX1, WORD wY1, WORD wX2, WORD wY2,
	UBYTE ubColor, UWORD uwPattern, UBYTE isOneDot
) {
	// Based on Cahir's function from:
	// https://github.com/cahirwpz/demoscene/blob/master/a500/base/libsys/blt-line.c

	UWORD uwBltCon1 = LINEMODE;
	if(isOneDot) {
		uwBltCon1 |= ONEDOT;
	}

	// Always draw the line downwards.
	if (wY1 > wY2) {
		SWAP(wX1, wX2);
		SWAP(wY1, wY2);
	}

	// Setup octant bits
	WORD wDx = wX2 - wX1;
	WORD wDy = wY2 - wY1;
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
	UWORD uwBltCon0 = ror16(wX1 & 15, 4);
	ULONG ulDataOffs = pDst->BytesPerRow * wY1 + ((wX1 / 8) & ~1);

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
		UBYTE *pFirstLineWord = pDst->Planes[ubPlane] + ulDataOffs;
		UWORD uwOp = ((ubColor & BV(ubPlane)) ? BLIT_LINE_MODE_OR : BLIT_LINE_MODE_ERASE);

		blitWait();
		g_pCustom->bltcon0 = uwBltCon0 | uwOp;
		g_pCustom->bltcpt = pFirstLineWord;
		g_pCustom->bltdpt = (APTR)(isOneDot ? pDst->Planes[pDst->Depth] : pFirstLineWord);
		g_pCustom->bltsize = uwBltSize;
	}
}

void blitLinePlane(
	tBitMap *pDst, WORD wX1, WORD wY1, WORD wX2, WORD wY2,
	UBYTE ubPlane, UWORD uwPattern, tBlitLineMode eMode, UBYTE isOneDot
) {
	// Based on Cahir's function from:
	// https://github.com/cahirwpz/demoscene/blob/master/a500/base/libsys/blt-line.c
	UWORD uwBltCon1 = LINEMODE;
	if(isOneDot) {
		uwBltCon1 |= ONEDOT;
	}

	// Always draw the line downwards.
	if (wY1 > wY2) {
		SWAP(wX1, wX2);
		SWAP(wY1, wY2);
	}

	// Setup octant bits
	WORD wDx = wX2 - wX1;
	WORD wDy = wY2 - wY1;
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
	UWORD uwBltCon0 = ror16(wX1 & 15, 4);
	ULONG ulDataOffs = pDst->BytesPerRow * wY1 + ((wX1 / 8) & ~1);
	UBYTE *pFirstLineWord = pDst->Planes[ubPlane] + ulDataOffs;
	UBYTE *pD = (APTR)(isOneDot ? pDst->Planes[pDst->Depth] : pFirstLineWord);

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
	g_pCustom->bltcon0 = uwBltCon0 | (UWORD)eMode;
	g_pCustom->bltcpt = pFirstLineWord;
	g_pCustom->bltdpt = pD;
	g_pCustom->bltsize = uwBltSize;
}
