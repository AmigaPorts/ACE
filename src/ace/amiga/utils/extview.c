/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/extview.h>
#include <ace/managers/system.h>

UBYTE viewIsLoaded(const tView *pView) {
	UBYTE isLoaded = (g_sCopManager.pCopList == pView->pCopList);
	return isLoaded;
}

void viewUpdateCLUT(tView *pView) {
	if(pView->uwFlags & VIEW_FLAG_GLOBAL_CLUT) {
		for(UBYTE i = 0; i < 32; ++i) {
			g_pCustom->color[i] = pView->pFirstVPort->pPalette[i];
		}
	}
	else {
		// na petli: vPortUpdateCLUT();
	}
}

/**
 *  @todo bplcon0 BPP is set up globally - make it only when all vports
 *        are truly of same BPP.
 */
void viewLoad(tView *pView) {
	logBlockBegin("viewLoad(pView: %p)", pView);

	isPAL = systemIsPal();
	UWORD uwWaitPos = isPAL ? 300 : 260;
	// if we are setting a NULL viewport we need to know if pal/NTSC
	while(getRayPos().bfPosY < uwWaitPos) continue;
	if(!pView) {
		g_sCopManager.pCopList = g_sCopManager.pBlankList;
		g_pCustom->bplcon0 = 0; // No output
		g_pCustom->bplcon3 = 0; // AGA fix
		g_pCustom->fmode = 0;   // AGA fix
		for(UBYTE i = 0; i < 6; ++i) {
			g_pCustom->bplpt[i] = 0;
		}
		g_pCustom->bpl1mod = 0;
		g_pCustom->bpl2mod = 0;
	}
	else {
#if defined(ACE_DEBUG)
		{
			// Check if view size matches size of last vport
			tVPort *pVp = pView->pFirstVPort;
			while(pVp->pNext) {
				pVp = pVp->pNext;
			}
			if(pVp->uwOffsY + pVp->uwHeight != pView->uwHeight) {
				logWrite(
					"ERR: View height %hu doesn't match the total VPort area: %hu",
					pView->uwHeight, pVp->uwOffsY + pVp->uwHeight
				);
			}
		}
#endif
		g_sCopManager.pCopList = pView->pCopList;
		g_pCustom->bplcon0 = (pView->pFirstVPort->ubBPP << 12) | BV(9); // BPP + composite output
		g_pCustom->fmode = 0;        // AGA fix
		g_pCustom->bplcon3 = 0;      // AGA fix
		g_pCustom->diwstrt = (pView->ubPosY << 8) | 0x81; // HSTART: 0x81
		UWORD uwDiwStopY = pView->ubPosY + pView->uwHeight;
		if(BTST(uwDiwStopY, 8) == BTST(uwDiwStopY, 7)) {
			logWrite(
				"ERR: DiwStopY (%hu) bit 8 (%hhu) must be different than bit 7 (%hhu)\n",
				uwDiwStopY, BTST(uwDiwStopY, 8), BTST(uwDiwStopY, 7)
			);
		}
		g_pCustom->diwstop = ((uwDiwStopY & 0xFF) << 8) | 0xC1; // HSTOP: 0xC1
		viewUpdateCLUT(pView);
	}
	copProcessBlocks();
	g_pCustom->copjmp1 = 1;
	systemSetDmaBit(DMAB_RASTER, pView != 0);

	// if we are setting a NULL viewport we need to know if pal/NTSC
	while(getRayPos().bfPosY < uwWaitPos) continue;

	logBlockEnd("viewLoad()");
}
