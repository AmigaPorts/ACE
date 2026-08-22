/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_FETCHMODE_H_
#define _ACE_UTILS_FETCHMODE_H_

#include <ace/types.h>
#include <ace/macros.h>
#include <ace/generic/screen.h>
#include <ace/utils/extview.h>

static inline UBYTE fetchModeGetBitplaneFmode(const tVPort *pVPort) {
#ifdef ACE_USE_AGA_FEATURES
	return pVPort->ubFmode & 0x03;
#else
	(void)pVPort;
	return 0;
#endif
}

static inline UWORD fetchModeGetDDfStep(const tVPort *pVPort) {
	UWORD uwWidth = pVPort->pView->uwWidth;
	switch(fetchModeGetBitplaneFmode(pVPort)) {
		case 1:
		case 2:
			return ((uwWidth / 32) - 1) * 16;
		case 3:
			return ((uwWidth / 64) - 1) * 32;
		case 0:
		default:
			return ((uwWidth / 16) - 1) * 8;
	}
}

static inline UWORD fetchModeGetDDfStrt(const tVPort *pVPort) {
	// http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0085.html
	UWORD uwDDfStrt = (pVPort->pView->ubPosX + 15) / 2 - 16;
	if(pVPort->eFlags & VP_FLAG_HIRES) {
		uwDDfStrt += 4;
	}
	return uwDDfStrt;
}

static inline UWORD fetchModeGetDDfStop(const tVPort *pVPort) {
	// http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0085.html
	return fetchModeGetDDfStrt(pVPort) + fetchModeGetDDfStep(pVPort);
}

static inline UWORD fetchModeGetCopWaitX(const tVPort *pVPort) {
	UWORD uwWaitAfterFetch = fetchModeGetDDfStop(pVPort) + (pVPort->ubBpp << 1) + 2;
	UWORD uwLatestSafeWait = s_pCopperWaitXByBitplanes[pVPort->ubBpp];
	return MIN(uwWaitAfterFetch, uwLatestSafeWait);
}

static inline UWORD fetchModeGetScrollPrefetchBytes(const tVPort *pVPort) {
	switch(fetchModeGetBitplaneFmode(pVPort)) {
		case 1:
		case 2:
			return 4;
		case 3:
			return 8;
		case 0:
		default:
			return 2;
	}
}

#ifdef ACE_USE_AGA_FEATURES
static inline UBYTE fetchModeGetFineUnitsPerPixel(const tVPort *pVPort) {
	// BPLCON1 H0 is 35ns: 4 units per lores pixel, 2 per hires pixel.
	return (pVPort->eFlags & VP_FLAG_HIRES) ? 2 : 4;
}
#endif

static inline UWORD fetchModeGetScrollDDfStartAdjust(const tVPort *pVPort) {
	UWORD uwAdjust;
	switch(fetchModeGetBitplaneFmode(pVPort)) {
		case 1:
		case 2:
			uwAdjust = 16;
			break;
		case 3:
			// One full 64-bit fetch block = 64 px = 32 color clocks, matching
			// the 8-byte (4-word) prefetch that the modulo compensates for.
			uwAdjust = 32;
			break;
		case 0:
		default:
			uwAdjust = 8;
			break;
	}
#ifdef ACE_USE_AGA_FEATURES
	// Hires packs twice as many pixels per color clock, so one fetch-block
	// of prefetch is half the lores DDF adjustment.
	if((pVPort->eFlags & VP_FLAG_HIRES) && (pVPort->eFlags & VP_FLAG_AGA)) {
		uwAdjust >>= 1;
	}
#endif
	return uwAdjust;
}

static inline UBYTE fetchModeIsOcsHiresScroll(const tVPort *pVPort) {
	if(!(pVPort->eFlags & VP_FLAG_HIRES)) {
		return 0;
	}
#ifdef ACE_USE_AGA_FEATURES
	// FMODE 0 hires still uses the classic 32px / 4-byte prefetch. Only
	// wide fetch (FMODE 1-3) needs the lores-style FMODE helpers.
	if((pVPort->eFlags & VP_FLAG_AGA) && fetchModeGetBitplaneFmode(pVPort)) {
		return 0;
	}
#endif
	return 1;
}

static inline void fetchModeApplyXScrollCopper(
	const tVPort *pVPort, UWORD *pDDfStrt, UWORD *pModulo
) {
	if(fetchModeIsOcsHiresScroll(pVPort)) {
		*pDDfStrt -= 8; // two more hires 4-part bitplane fetch patterns
		*pModulo -= 4;
		return;
	}

	*pDDfStrt -= fetchModeGetScrollDDfStartAdjust(pVPort);
	*pModulo -= fetchModeGetScrollPrefetchBytes(pVPort);
}

static inline LONG fetchModeGetInitialBplOffset(const tVPort *pVPort) {
	if(fetchModeIsOcsHiresScroll(pVPort)) {
		return -4;
	}

	return -(LONG)fetchModeGetScrollPrefetchBytes(pVPort);
}

#ifdef ACE_USE_AGA_FEATURES
/**
 * @brief Packs an 8-bit 35ns delay into BPLCON1 for both playfields.
 * Delay bit 0 is H0 (35ns) -> BPLCON1 bit 8; bit 7 is H7 -> bit 11.
 * PF2 bits are PF1 << 4.
 */
static inline UWORD fetchModeEncodeBplcon1FromDelayFine(UWORD uwDelayFine) {
	UWORD uwPf1 = 0;
	uwPf1 |= (uwDelayFine & 0x01) << 8; // H0 -> bit 8
	uwPf1 |= (uwDelayFine & 0x02) << 8; // H1 -> bit 9
	uwPf1 |= (uwDelayFine & 0x3C) >> 2; // H2-H5 -> bits 0-3
	uwPf1 |= (uwDelayFine & 0x40) << 4; // H6 -> bit 10
	uwPf1 |= (uwDelayFine & 0x80) << 4; // H7 -> bit 11
	return (UWORD)(uwPf1 | (uwPf1 << 4));
}

static inline ULONG fetchModeCombineScrollFine(
	const tVPort *pVPort, UWORD uwScrollX, UBYTE ubFineX
) {
	UWORD uwUnitsPerPx = fetchModeGetFineUnitsPerPixel(pVPort);
	return (ULONG)uwScrollX * uwUnitsPerPx + (ubFineX & (uwUnitsPerPx - 1));
}

static inline UWORD fetchModeGetScrollBlockFine(const tVPort *pVPort) {
	return (UWORD)(fetchModeGetScrollPrefetchBytes(pVPort) << 3) *
		fetchModeGetFineUnitsPerPixel(pVPort);
}
#endif

static inline UWORD fetchModeCalcBplShift(
	const tVPort *pVPort, UWORD uwScrollX, UBYTE ubFineX
) {
#ifdef ACE_USE_AGA_FEATURES
	{
		UWORD uwBlockFine = fetchModeGetScrollBlockFine(pVPort);
		ULONG ulScrollFine = fetchModeCombineScrollFine(pVPort, uwScrollX, ubFineX);
		UWORD uwDelayFine = (UWORD)(
			(uwBlockFine - (ulScrollFine & (uwBlockFine - 1))) & (uwBlockFine - 1)
		);
		return fetchModeEncodeBplcon1FromDelayFine(uwDelayFine);
	}
#endif
	(void)ubFineX;
	if(pVPort->eFlags & VP_FLAG_HIRES) {
		UWORD uwShift = ((16 - (uwScrollX & 0xF)) & 0xF) >> 1; // 0..7, 2 px/value
		return (uwShift << 4) | uwShift;
	}

	// AGA wide fetch needs scroll over the whole fetch block (16/32/64 px), not
	// just 16 px. The delay therefore spans 0..block-1 lo-res pixels and must be
	// encoded into the extended BPLCON1 scroll bits (8 bits per playfield):
	//   lo-res bits 0-3 -> PF1H2-H5 (bits 0-3) / PF2H2-H5 (bits 4-7)
	//   lo-res bit  4   -> PF1H6 (bit 10) / PF2H6 (bit 14)  (16 px)
	//   lo-res bit  5   -> PF1H7 (bit 11) / PF2H7 (bit 15)  (32 px)
	UWORD uwBlock = fetchModeGetScrollPrefetchBytes(pVPort) << 3; // 16 / 32 / 64
	UWORD uwDelay = (uwBlock - (uwScrollX & (uwBlock - 1))) & (uwBlock - 1);

	UWORD uwLow = uwDelay & 0x0F;
	UWORD uwBplcon1 = (uwLow << 4) | uwLow;
	if(uwDelay & 0x10) {
		uwBplcon1 |= (1 << 14) | (1 << 10);
	}
	if(uwDelay & 0x20) {
		uwBplcon1 |= (1 << 15) | (1 << 11);
	}
	return uwBplcon1;
}

static inline LONG fetchModeCalcBplOffsetX(
	const tVPort *pVPort, UWORD uwScrollX, UBYTE ubFineX
) {
#ifdef ACE_USE_AGA_FEATURES
	{
		// BPLCON1 wraps every fetch-block in 35ns units. The bitplane pointer
		// must step on that same wrap, not on integer X, otherwise a non-zero
		// fine remainder jumps ~15px each time delayFine overflows.
		ULONG ulScrollFine = fetchModeCombineScrollFine(pVPort, uwScrollX, ubFineX);
		UWORD uwBlockFine = fetchModeGetScrollBlockFine(pVPort);
		UBYTE ubPrefetch = fetchModeGetScrollPrefetchBytes(pVPort);
		UBYTE ubBlockShift = 0;
		UWORD uwV = uwBlockFine;
		while(uwV > 1) {
			uwV >>= 1;
			++ubBlockShift;
		}
		LONG lOffs = (((LONG)ulScrollFine - 1) >> ubBlockShift) * (LONG)ubPrefetch;
		if(fetchModeIsOcsHiresScroll(pVPort)) {
			lOffs -= 2;
		}
		return lOffs;
	}
#endif
	(void)ubFineX;
	if(fetchModeIsOcsHiresScroll(pVPort)) {
		return ((((LONG)uwScrollX - 1) >> 4) << 1) - 2;
	}

	// AGA wide fetch: the bitplane pointer must advance in whole fetch blocks
	// (word/long/quad aligned). A 2-byte step in 32/64-bit fetch gets its low
	// address bits truncated by the hardware -> "cut" picture every block.
	// At uwScrollX == 0 this yields -block/8 bytes, i.e. the one-block prefetch.
	// Hires AGA uses the same byte steps: one fetch still covers 16/32/64 pixels.
	switch(fetchModeGetBitplaneFmode(pVPort)) {
		case 1:
		case 2:
			return (((LONG)uwScrollX - 1) >> 5) << 2; // 32 px block, 4-byte step
		case 3:
			return (((LONG)uwScrollX - 1) >> 6) << 3; // 64 px block, 8-byte step
		case 0:
		default:
			return (((LONG)uwScrollX - 1) >> 4) << 1; // 16 px block, 2-byte step
	}
}

#endif // _ACE_UTILS_FETCHMODE_H_
