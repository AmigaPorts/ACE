#include <ace/managers/blit.h>

tBlitManager g_sBlitManager = {0};

/**
 * Sets blitter registers to next blit data
 * NOTE: Can't log inside it because it's used by blitInterruptHandler
 */
inline void blitSetRegs(tBlitData *pData) {
	custom.bltcon0 = pData->bltcon0;
	custom.bltcon1 = pData->bltcon1;

	custom.bltafwm = pData->bltafwm;
	custom.bltalwm = pData->bltalwm;
	// shift & mask before ptr & data

	custom.bltcmod = pData->bltcmod;
	custom.bltbmod = pData->bltbmod;
	custom.bltamod = pData->bltamod;
	custom.bltdmod = pData->bltdmod;

	custom.bltcdat = pData->bltcdat;
	custom.bltbdat = pData->bltbdat;
	custom.bltadat = pData->bltadat;

	custom.bltcpt  = pData->bltcpt;
	custom.bltbpt  = pData->bltbpt;
	custom.bltapt  = pData->bltapt;
	custom.bltdpt  = pData->bltdpt;

	custom.bltsize = pData->bltsize;
	// TODO: zamiana na copymemy dopiero jak wszystko inne b�dzie dzia�a�
}

/**
 * Blit interrupt handler
 * Fetches next blit from queue and sets custom registers to its values
 * NOTE: Can't log inside this fn and all other called by it
 */
__amigainterrupt __saveds void blitInterruptHandler(__reg("a0") struct Custom *cstm, __reg("a1") tBlitManager *pBlitManager) {
	if(pBlitManager->ubBlitStarted) {
		// If triggered by blit from queue => clear blit size and move to next blit
		pBlitManager->ubBlitStarted = 0;
		pBlitManager->pBlitData[pBlitManager->uwBlitPos].bltdpt = 0;

		++pBlitManager->uwBlitPos;
		if(pBlitManager->uwBlitPos == pBlitManager->uwQueueLength) {
			pBlitManager->uwBlitPos = 0;
		}
	}

	tBlitData *pData;
	pData = &pBlitManager->pBlitData[pBlitManager->uwBlitPos];

	if(pData->bltdpt) {
		// If next blit in queue has non-zero size => process it
		blitSetRegs(pData);
		pBlitManager->ubBlitStarted = 1;
	}
	cstm->intreq = INTF_BLIT;
}

/**
 * Waits until queue stops working
 */
void blitQueueWait(void) {
	while(g_sBlitManager.ubBlitStarted) {
		WaitBlit();
	};
}

void blitQueueEnable(UWORD uwQueueLength) {
	logBlockBegin("blitQueueEnable");

	// Set manager fields
	g_sBlitManager.uwQueueLength = uwQueueLength;
	g_sBlitManager.pBlitterSetFn = blitQueued;
	g_sBlitManager.pBlitData = memAllocFastClear(sizeof(tBlitData) * uwQueueLength);
	g_sBlitManager.szHandlerName = memAllocFast(17);
	strcpy(g_sBlitManager.szHandlerName, "ACE Blit Manager");

	// Interrupt struct setup
	g_sBlitManager.pInt = memAllocChipClear(sizeof(struct Interrupt)); // CHIP is PUBLIC
	g_sBlitManager.pInt->is_Node.ln_Type = NT_INTERRUPT;
	g_sBlitManager.pInt->is_Node.ln_Pri = 10;
	g_sBlitManager.pInt->is_Node.ln_Name = g_sBlitManager.szHandlerName;
	g_sBlitManager.pInt->is_Data = &g_sBlitManager;
	g_sBlitManager.pInt->is_Code = blitInterruptHandler;

	// Enable interrupt handler
	g_sBlitManager.pPrevInt = SetIntVector(INTB_BLIT, g_sBlitManager.pInt);
	g_sBlitManager.uwOldDmaCon = custom.dmaconr;
	g_sBlitManager.uwOldIntEna = custom.intenar;
	custom.dmacon = DMAF_SETCLR | DMAF_BLITTER | DMAF_BLITHOG;
	custom.intreq = INTF_SETCLR | INTF_BLIT;
	custom.intena = INTF_SETCLR | INTF_BLIT;

	logBlockEnd("blitQueueEnable");
	logWrite("blitter queue enabled! Length: %u\n", uwQueueLength);
}

void blitQueueDisable(void) {

	logBlockBegin("blitQueueDisable");
	// Wait for all blits to finish
	blitQueueWait();
	// Disable queue
	if(g_sBlitManager.uwQueueLength) {
		// Remove interrupt handler
		custom.intena = 0x7FFF;
		custom.dmacon = 0x7FFF;
		custom.intreq = 0x7FFF;
		custom.intena = g_sBlitManager.uwOldIntEna | 0xC000;
		custom.dmacon = g_sBlitManager.uwOldDmaCon | 0x8000;
		SetIntVector(INTB_BLIT, g_sBlitManager.pPrevInt);
		logWrite("int ok\n");
		// Cleanup
		memFree(g_sBlitManager.pInt, sizeof(struct Interrupt));
		memFree(g_sBlitManager.szHandlerName, 17);
		memFree(g_sBlitManager.pBlitData, sizeof(tBlitData) * g_sBlitManager.uwQueueLength);
		g_sBlitManager.uwQueueLength = 0;
		g_sBlitManager.uwAddPos = 0;
		g_sBlitManager.uwBlitPos = 0;
	}
	g_sBlitManager.pBlitData = 0;
	g_sBlitManager.szHandlerName = 0;
	g_sBlitManager.pBlitterSetFn = blitNotQueued;
	logBlockEnd("blitQueueDisable");
}

void blitManagerCreate(UWORD uwQueueLength, UWORD uwFlags) {
	logBlockBegin("blitManagerCreate");

	g_sBlitManager.uwQueueLength = 0;
	g_sBlitManager.uwAddPos = 0;
	g_sBlitManager.uwBlitPos = 0;
	g_sBlitManager.ubBlitStarted = 0;
	if(!uwQueueLength)
		blitQueueDisable();
	else
		blitQueueEnable(uwQueueLength);

	logBlockEnd("blitManagerCreate");
}

void blitManagerDestroy(void) {
	logBlockBegin("blitManagerDestroy");
	logWrite("blitPos: %u\n", g_sBlitManager.uwBlitPos);

	WaitBlit();
	WaitTOF();
	blitQueueDisable();

	logBlockEnd("blitManagerDestroy");
}

/**
 * Checks if blit is allowable at coords at given source and destination
 */
BYTE blitCheck(
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
			"source %dx%d, dest: %dx%d, blit: %d,%d -> %d,%d %dx%d %s@%u\n",
			wSrcWidth, pSrc->Rows, wDstWidth, pDst->Rows,
			wSrcX, wSrcY, wDstX, wDstY, wWidth, wHeight, szFile, uwLine
		);
		return 0;
	}
	if(pDst && (wDstY < 0 || wDstWidth < wDstX+wWidth || pDst->Rows < wDstY+wHeight)) {
		logWrite(
			"ILLEGAL BLIT Dest out of range: "
			"source %dx%d, dest: %dx%d, blit: %d,%d -> %d,%d %dx%d %s@%u\n",
			wSrcWidth, pSrc->Rows, wDstWidth, pDst->Rows,
			wSrcX, wSrcY, wDstX, wDstY, wWidth, wHeight, szFile, uwLine
		);
		return 0;
	}
#endif
	return 1;
}

/**
 *  Checks if blitter is idle
 *  Polls 2 times - A1000 Agnus bug workaround
 *  @todo Make it inline assembly or dmaconr volatile so compiler won't
 *  'optimize' it.
 */
inline BYTE blitIsIdle(void) {
	if(!(custom.dmaconr & DMAF_BLTDONE))
		if(!(custom.dmaconr & DMAF_BLTDONE))
			return 1;
	return 0;
}

void blitNotQueued(
	UWORD bltcon0, UWORD bltcon1, UWORD bltafwm, UWORD bltalwm,
	WORD bltamod, WORD bltbmod, WORD bltcmod, WORD bltdmod,
	UBYTE *bltapt, UBYTE *bltbpt, UBYTE *bltcpt, UBYTE *bltdpt,
	UWORD bltadat, UWORD bltbdat, UWORD bltcdat,
	UWORD bltsize
) {
	OwnBlitter();
	WaitBlit();

	custom.bltcon0 = bltcon0;
	custom.bltcon1 = bltcon1;

	custom.bltafwm = bltafwm;
	custom.bltalwm = bltalwm;

	custom.bltcmod = bltcmod;
	custom.bltbmod = bltbmod;
	custom.bltamod = bltamod;
	custom.bltdmod = bltdmod;

	custom.bltcdat = bltcdat;
	custom.bltbdat = bltbdat;
	custom.bltadat = bltadat;

	custom.bltcpt  = bltcpt;
	custom.bltbpt  = bltbpt;
	custom.bltapt  = bltapt;
	custom.bltdpt  = bltdpt;

	custom.bltsize = bltsize;

	DisownBlitter();
}
/**
 * Adds blit to queue
 */
void blitQueued(
	UWORD bltcon0, UWORD bltcon1, UWORD bltafwm, UWORD bltalwm,
	WORD bltamod, WORD bltbmod, WORD bltcmod, WORD bltdmod,
	UBYTE *bltapt, UBYTE *bltbpt, UBYTE *bltcpt, UBYTE *bltdpt,
	UWORD bltadat, UWORD bltbdat, UWORD bltcdat,
	UWORD bltsize
) {
	// Add new entry to blitter queue
	tBlitData *pData = &g_sBlitManager.pBlitData[g_sBlitManager.uwAddPos];

	pData->bltamod = bltamod;
	pData->bltbmod = bltbmod;
	pData->bltcmod = bltcmod;
	pData->bltdmod = bltdmod;

	pData->bltcon0 = bltcon0;
	pData->bltcon1 = bltcon1;
	pData->bltafwm = bltafwm;
	pData->bltalwm = bltalwm;

	pData->bltcpt  = bltcpt;
	pData->bltbpt  = bltbpt;
	pData->bltapt  = bltapt;
	pData->bltdpt  = bltdpt;
	pData->bltsize = bltsize;

	pData->bltcdat = bltcdat;
	pData->bltbdat = bltbdat;
	pData->bltadat = bltadat;

	// logWrite("Added blit pos %u: pt: %p %p %p %p\n", g_sBlitManager.uwAddPos, bltapt, bltbpt, bltcpt, bltdpt);

	// Execute blit if all queue entries were processed
	if(g_sBlitManager.uwBlitPos == g_sBlitManager.uwAddPos) {
		while(!blitIsIdle())
			WaitBlit();
		// if(blitIsIdle()) {
				OwnBlitter();
				// logWrite("Blitter is idle - starting blit\n");
				blitSetRegs(pData);
				DisownBlitter();
				g_sBlitManager.ubBlitStarted = 1;
		// }
	}

	++g_sBlitManager.uwAddPos;
	if(g_sBlitManager.uwAddPos == g_sBlitManager.uwQueueLength)
		g_sBlitManager.uwAddPos = 0;
}

/**
 * Blit without mask - BltBitMap equivalent, but less safe
 * Channels:
 * 	A: mask const, read disabled
 * 	B: src  read
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
BYTE blitUnsafeCopy(
	tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubMinterm, UBYTE ubMask
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

	ubMask &= 0xFF >> (8- (pSrc->Depth < pDst->Depth? pSrc->Depth: pDst->Depth));
	ubPlane = 0;
	while(ubMask) {
		if(ubMask & 1)
			g_sBlitManager.pBlitterSetFn(
				uwBltCon0, uwBltCon1,                  // bltconX
				uwFirstMask, uwLastMask,               // bltaXwm
				0, wSrcModulo, wDstModulo, wDstModulo, // bltXmod
				// This hell of a casting must stay here or else large offsets get bugged!
				0,                                     // bltapt
				(UBYTE*)(((ULONG)(pSrc->Planes[ubPlane])) + ulSrcOffs), // bltbpt
				(UBYTE*)(((ULONG)(pDst->Planes[ubPlane])) + ulDstOffs), // bltcpt
				(UBYTE*)(((ULONG)(pDst->Planes[ubPlane])) + ulDstOffs), // bltdpt
				0xFFFF, 0, 0,                          // bltXdat
				(wHeight << 6) | uwBlitWords           // bltsize
			);
		ubMask >>= 1;
		++ubPlane;
	}
	return 1;
}

BYTE blitSafeCopy(
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
BYTE blitUnsafeCopyAligned(
	tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight
) {
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

		g_sBlitManager.pBlitterSetFn(
			uwBltCon0, 0,                 // bltconX
			0xFFFF, 0xFFFF,               // bltaXwm
			wSrcModulo, 0, 0, wDstModulo, // bltXmod
			// This hell of a casting must stay here or else large offsets get bugged!
			(UBYTE*)(((ULONG)(pSrc->Planes[0])) + ulSrcOffs), // bltapt
			0, 0,                         // bltbpt, bltcpt
			(UBYTE*)(((ULONG)(pDst->Planes[0])) + ulDstOffs), // bltdpt
			0, 0, 0,                      // bltXdat
			(wHeight << 6) | uwBlitWords  // bltsize
		);
	}
	else {
		UBYTE ubPlane;

		if(bitmapIsInterleaved(pSrc))
			wSrcModulo += pSrc->BytesPerRow * (pSrc->Depth-1);
		else if(bitmapIsInterleaved(pDst))
			wDstModulo += pDst->BytesPerRow * (pDst->Depth-1);

		for(ubPlane = pSrc->Depth; ubPlane--;) {
			g_sBlitManager.pBlitterSetFn(
				uwBltCon0, 0,                      // bltconX
				0xFFFF, 0xFFFF,                    // bltaXwm
				wSrcModulo, 0, 0, wDstModulo,      // bltXmod
				// This hell of a casting must stay here or else large offsets get bugged!
				(UBYTE*)(((ULONG)(pSrc->Planes[ubPlane])) + ulSrcOffs), // bltapt
				0, 0,                              // bltbpt, bltcpt
				(UBYTE*)(((ULONG)(pDst->Planes[ubPlane])) + ulDstOffs), // bltdpt
				0, 0, 0,                           // bltXdat
				(wHeight << 6) | uwBlitWords       // bltsize
			);
		}
	}
	return 1;
}

BYTE blitSafeCopyAligned(
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
BYTE blitUnsafeCopyMask(
	tBitMap *pSrc, WORD wSrcX, WORD wSrcY,
	tBitMap *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, UWORD *pMsk
) {
	UWORD uwBlitWidth, uwBlitWords, uwBltCon0, uwBltCon1;
	UWORD uwFirstMask, uwLastMask
	ULONG ulSrcOffs, ulDstOffs;
	WORD wDstModulo, wSrcModulo;
	UBYTE ubSrcDelta, ubDstDelta, ubMaskFShift, ubMaskLShift, ubPlane;

	ubSrcDelta = wSrcX & 0xF;
	ubDstDelta = wDstX & 0xF;

	uwBlitWidth = (wWidth+ubDstDelta+15) & 0xFFF0;
	uwBlitWords = uwBlitWidth >> 4;

	ubMaskFShift = ubSrcDelta;
	ubMaskLShift = uwBlitWidth-(wWidth+ubSrcDelta);

	uwFirstMask = 0xFFFF >> ubMaskFShift;
	uwLastMask = 0xFFFF << ubMaskLShift;

	uwBltCon1 = (ubDstDelta-ubSrcDelta) << BSHIFTSHIFT;
	uwBltCon0 = uwBltCon1 | USEA|USEB|USEC|USED | MINTERM_COOKIE;

	wSrcModulo = pSrc->BytesPerRow - (uwBlitWords<<1);
	wDstModulo = pDst->BytesPerRow - (uwBlitWords<<1);
	ulSrcOffs = pSrc->BytesPerRow * wSrcY + (wSrcX>>3);
	ulDstOffs = pDst->BytesPerRow * wDstY + (wDstX>>3);

	for(ubPlane = pSrc->Depth; ubPlane--;) {
		g_sBlitManager.pBlitterSetFn(
			// BltCon & Masks
			uwBltCon0, uwBltCon1,
			uwFirstMask, uwLastMask,
			// Modulos
			wSrcModulo, wSrcModulo, wDstModulo, wDstModulo, // A, B, C, D
			// Channel ptrs - in bytes, blitter ignores LSB, thus makes even addrs
			// This hell of a casting must stay here or else large offsets get bugged!
			(UBYTE*)((ULONG)pMsk) + ulSrcOffs,                      // bltapt
			(UBYTE*)(((ULONG)(pSrc->Planes[ubPlane])) + ulSrcOffs), // bltbpt
			(UBYTE*)(((ULONG)(pDst->Planes[ubPlane])) + ulDstOffs), // bltcpt
			(UBYTE*)(((ULONG)(pDst->Planes[ubPlane])) + ulDstOffs), // bltdpt
			0, 0, 0,                                                // bltXdat
			// BLTSIZE
			(wHeight << 6) | uwBlitWords // custom.bltsize
		);
	}
	return 1;
}

BYTE blitSafeCopyMask(
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
BYTE _blitRect(
	tBitMap *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubColor, UWORD uwLine, char *szFile
) {
	if(!blitCheck(0,0,0,pDst, wDstX, wDstY, wWidth, wHeight, uwLine, szFile))
		return 0;

	// Helper vars
	UWORD uwBlitWords, uwBlitWidth, uwDstOffs;
	UBYTE ubDstDelta, ubMinterm, ubPlane;
	// Blitter register values
	UWORD uwBltCon0, uwBltCon1, uwFirstMask, uwLastMask;
	WORD wSrcModulo, wDstModulo;

	ubDstDelta = wDstX & 0xF;
	uwBlitWidth = (wWidth+ubDstDelta+15) & 0xFFF0;
	uwBlitWords = uwBlitWidth >> 4;

	uwFirstMask = 0xFFFF >> ubDstDelta;
	uwLastMask = 0xFFFF << (uwBlitWidth-(wWidth+ubDstDelta));
	uwBltCon1 = 0;
	uwDstOffs = pDst->BytesPerRow * wDstY + (wDstX>>3);
	wDstModulo = pDst->BytesPerRow - (uwBlitWords<<1);
	uwBltCon0 = USEC|USED;

	ubPlane = 0;
	do {
		if(ubColor & 1)
			ubMinterm = 0xFA;
		else
			ubMinterm = 0x0A;
		g_sBlitManager.pBlitterSetFn(
			uwBltCon0 | ubMinterm, uwBltCon1,                       // bltconX
			uwFirstMask, uwLastMask,                                // bltaXwm
			0, 0, wDstModulo, wDstModulo,                           // bltXmod
			// This hell of a casting must stay here or else large offsets get bugged!
			0, 0,                                                   // bltapt, bltbpt
			(UBYTE*)(((ULONG)(pDst->Planes[ubPlane])) + uwDstOffs), // bltcpt
			(UBYTE*)(((ULONG)(pDst->Planes[ubPlane])) + uwDstOffs), // bltdpt
			0xFFFF, 0, 0,                                           // bltXdat
			(wHeight << 6) | uwBlitWords                            // bltsize
		);
		ubColor >>= 1;
		++ubPlane;
	}	while(ubPlane != pDst->Depth);
	return 1;
}
