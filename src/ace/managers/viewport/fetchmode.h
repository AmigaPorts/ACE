/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_VIEWPORT_FETCHMODE_H_
#define _ACE_MANAGERS_VIEWPORT_FETCHMODE_H_

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
	UBYTE ubBitplaneFmode = fetchModeGetBitplaneFmode(pVPort);

	if(ubBitplaneFmode == 1 || ubBitplaneFmode == 2) {
		return ((uwWidth / 32) - 1) * 16;
	}

	if(ubBitplaneFmode == 3) {
		return ((uwWidth / 16) - 1) * 6;
	}

	return ((uwWidth / 16) - 1) * 8;
}

static inline UWORD fetchModeGetScrollPrefetchBytes(const tVPort *pVPort) {
	UBYTE ubBitplaneFmode = fetchModeGetBitplaneFmode(pVPort);

	if(ubBitplaneFmode == 1 || ubBitplaneFmode == 2) {
		return 4;
	}

	if(ubBitplaneFmode == 3) {
		return 8;
	}

	return 2;
}

static inline UWORD fetchModeGetScrollDDfStartAdjust(const tVPort *pVPort) {
	UBYTE ubBitplaneFmode = fetchModeGetBitplaneFmode(pVPort);

	if(ubBitplaneFmode == 1 || ubBitplaneFmode == 2) {
		return 16;
	}

	if(ubBitplaneFmode == 3) {
		return 24;
	}

	return 8;
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

static inline void fetchModeApplyScrollBufferXScrollCopper(
	const tVPort *pVPort, UWORD *pDDfStrt, UWORD *pModulo
) {
	UBYTE ubBitplaneFmode = fetchModeGetBitplaneFmode(pVPort);

	if(pVPort->eFlags & VP_FLAG_HIRES) {
		*pDDfStrt -= 8;
		*pModulo -= 4;
		return;
	}

	// Scrollbuffer uses a circular/corkscrew bitmap layout. FMODE 3 still
	// needs the wider fetch modulo, but starting one extra fetch block earlier
	// causes a 64 px ghost after the vertical wrap.
	if(ubBitplaneFmode == 3) {
		*pDDfStrt -= 16;
	}
	else {
		*pDDfStrt -= fetchModeGetScrollDDfStartAdjust(pVPort);
	}
	*pModulo -= fetchModeGetScrollPrefetchBytes(pVPort);
}

static inline LONG fetchModeGetInitialBplOffset(const tVPort *pVPort) {
	if(pVPort->eFlags & VP_FLAG_HIRES) {
		return -4;
	}

	return -(LONG)fetchModeGetScrollPrefetchBytes(pVPort);
}

static inline UWORD fetchModeCalcBplShift(const tVPort *pVPort, UWORD uwScrollX) {
	UWORD uwShift = (16 - (uwScrollX & 0xF)) & 0xF;

	if(pVPort->eFlags & VP_FLAG_HIRES) {
		uwShift >>= 1; // Usable scroll values are 0..7, shifts 2 pixels per value
	}

	return (uwShift << 4) | uwShift;
}

static inline LONG fetchModeCalcBplOffsetX(const tVPort *pVPort, UWORD uwScrollX) {
	LONG lBplAddX = (((LONG)uwScrollX - 1) >> 4) << 1;
	UBYTE ubBitplaneFmode = fetchModeGetBitplaneFmode(pVPort);

	if(pVPort->eFlags & VP_FLAG_HIRES) {
		return lBplAddX - 2;
	}

	if(ubBitplaneFmode == 1 || ubBitplaneFmode == 2) {
		return lBplAddX - 2;
	}

	if(ubBitplaneFmode == 3) {
		lBplAddX += 2;
	}

	return lBplAddX;
}

#endif // _ACE_MANAGERS_VIEWPORT_FETCHMODE_H_
