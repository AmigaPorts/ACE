/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_FETCHMODE_H_
#define _ACE_UTILS_FETCHMODE_H_

#include <ace/types.h>
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
			return ((uwWidth / 16) - 1) * 6;
		case 0:
		default:
			return ((uwWidth / 16) - 1) * 8;
	}
}

static inline UWORD fetchModeGetDDfStrt(const tVPort *pVPort) {
	UWORD uwDDfStrt = (pVPort->pView->ubPosX + 15) / 2 - 16;
	if(pVPort->eFlags & VP_FLAG_HIRES) {
		uwDDfStrt += 4;
	}
	return uwDDfStrt;
}

static inline UWORD fetchModeGetDDfStop(const tVPort *pVPort) {
	return fetchModeGetDDfStrt(pVPort) + fetchModeGetDDfStep(pVPort);
}

static inline UWORD fetchModeGetCopWaitX(const tVPort *pVPort) {
	if (fetchModeGetBitplaneFmode(pVPort) == 3) {
		UWORD uwDDfStop = fetchModeGetDDfStop(pVPort);
		UWORD uwFetchClocks = pVPort->ubBpp;
		return uwDDfStop + (uwFetchClocks << 1) + 2;
	}

	static const UWORD pCopperWaitXByBitplanes[9] = {0x00, 0xDC, 0xDC, 0xDC, 0xDC, 0xDC, 0xD9, 0xC6, 0xA0};
	return pCopperWaitXByBitplanes[pVPort->ubBpp];
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

static inline UWORD fetchModeGetScrollDDfStartAdjust(const tVPort *pVPort) {
	switch(fetchModeGetBitplaneFmode(pVPort)) {
		case 1:
		case 2:
			return 16;
		case 3:
			// One full 64-bit fetch block = 64 px = 32 color clocks, matching
			// the 8-byte (4-word) prefetch that the modulo compensates for.
			return 32;
		case 0:
		default:
			return 8;
	}
}

static inline void fetchModeApplyXScrollCopper(
	const tVPort *pVPort, UWORD *pDDfStrt, UWORD *pModulo
) {
	if(pVPort->eFlags & VP_FLAG_HIRES) {
		*pDDfStrt -= 8; // two more hires 4-part bitplane fetch patterns
		*pModulo -= 4;
		return;
	}

	*pDDfStrt -= fetchModeGetScrollDDfStartAdjust(pVPort);
	*pModulo -= fetchModeGetScrollPrefetchBytes(pVPort);
}

static inline LONG fetchModeGetInitialBplOffset(const tVPort *pVPort) {
	if(pVPort->eFlags & VP_FLAG_HIRES) {
		return -4;
	}

	return -(LONG)fetchModeGetScrollPrefetchBytes(pVPort);
}

static inline UWORD fetchModeCalcBplShift(const tVPort *pVPort, UWORD uwScrollX) {
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

static inline LONG fetchModeCalcBplOffsetX(const tVPort *pVPort, UWORD uwScrollX) {
	if(pVPort->eFlags & VP_FLAG_HIRES) {
		return ((((LONG)uwScrollX - 1) >> 4) << 1) - 2;
	}

	// AGA wide fetch: the bitplane pointer must advance in whole fetch blocks
	// (word/long/quad aligned). A 2-byte step in 32/64-bit fetch gets its low
	// address bits truncated by the hardware -> "cut" picture every block.
	// At uwScrollX == 0 this yields -block/8 bytes, i.e. the one-block prefetch.
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
