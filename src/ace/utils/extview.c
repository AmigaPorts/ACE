/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/extview.h>
#include <limits.h>
#include <ace/managers/system.h>
#include <ace/utils/tag.h>
#include <ace/generic/screen.h>

tView *viewCreate(void *pTags, ...) {
	logBlockBegin("viewCreate(pTags: %p)", pTags);
#ifdef AMIGA

	// Create view stub
	tView *pView = memAllocFastClear(sizeof(tView));
	logWrite("addr: %p\n", pView);

	va_list vaTags;
	va_start(vaTags, pTags);

	// Process copperlist raw/block tags
	if(
		tagGet(pTags, vaTags, TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_BLOCK)
		== VIEW_COPLIST_MODE_RAW
	) {
		ULONG ulCopListSize = tagGet(pTags, vaTags, TAG_VIEW_COPLIST_RAW_COUNT, -1);
		pView->pCopList = copListCreate(0,
			TAG_COPPER_LIST_MODE, COPPER_MODE_RAW,
			TAG_COPPER_RAW_COUNT, ulCopListSize,
			TAG_DONE
		);
		pView->uwFlags |= VIEW_FLAG_COPLIST_RAW;
	}
	else {
		pView->pCopList = copListCreate(0, TAG_DONE);
	}

	// Additional CLUT tags
	if(tagGet(pTags, vaTags, TAG_VIEW_GLOBAL_CLUT, 0)) {
		pView->uwFlags |= VIEW_FLAG_GLOBAL_CLUT;
		logWrite("Global CLUT mode enabled\n");
	}

	va_end(vaTags);
	logBlockEnd("viewCreate()");
	return pView;
#else
	logBlockEnd("viewCreate()");
	return 0;
#endif // AMIGA
}

void viewDestroy(tView *pView) {
	logBlockBegin("viewDestroy(pView: %p)", pView);
#ifdef AMIGA
	if(g_sCopManager.pCopList == pView->pCopList) {
		viewLoad(0);
	}

	// Free all attached viewports
	while(pView->pFirstVPort) {
		vPortDestroy(pView->pFirstVPort);
	}

	// Free view
	logWrite("Freeing copperlists...\n");
	copListDestroy(pView->pCopList);
	memFree(pView, sizeof(tView));
#endif // AMIGA
	logBlockEnd("viewDestroy()");
}

void vPortProcessManagers(tVPort *pVPort) {
	tVpManager *pManager = pVPort->pFirstManager;
	while(pManager) {
		pManager->process(pManager);
		pManager = pManager->pNext;
	}
}

void viewProcessManagers(tView *pView) {
	tVPort *pVPort = pView->pFirstVPort;
	while(pVPort) {
		vPortProcessManagers(pVPort);
		pVPort = pVPort->pNext;
	}
}

void viewUpdateCLUT(tView *pView) {
#ifdef AMIGA
	if(pView->uwFlags & VIEW_FLAG_GLOBAL_CLUT) {
		for(UBYTE i = 0; i < 32; ++i) {
			g_pCustom->color[i] = pView->pFirstVPort->pPalette[i];
		}
	}
	else {
		// na petli: vPortUpdateCLUT();
	}
#endif // AMIGA
}

/**
 *  @todo bplcon0 BPP is set up globally - make it only when all vports
 *        are truly of same BPP.
 */
void viewLoad(tView *pView) {
	logBlockBegin("viewLoad(pView: %p)", pView);
#if defined(AMIGA)
	while(g_pRayPos->bfPosY < 300) {}
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
		g_sCopManager.pCopList = pView->pCopList;
		g_pCustom->bplcon0 = (pView->pFirstVPort->ubBPP << 12) | BV(9); // BPP + composite output
		g_pCustom->fmode = 0;        // AGA fix
		g_pCustom->bplcon3 = 0;      // AGA fix
		g_pCustom->diwstrt = 0x2C81; // VSTART: 0x2C, HSTART: 0x81
		g_pCustom->diwstop = 0x2CC1; // VSTOP: 0x2C, HSTOP: 0xC1
		viewUpdateCLUT(pView);
	}
	copProcessBlocks();
	g_pCustom->copjmp1 = 1;
	systemSetDmaBit(DMAB_RASTER, pView != 0);
	while(g_pRayPos->bfPosY < 300) {}
#endif // AMIGA
	logBlockEnd("viewLoad()");
}

tVPort *vPortCreate(void *pTagList, ...) {
	logBlockBegin("vPortCreate(pTagList: %p)", pTagList);
	va_list vaTags;
	va_start(vaTags, pTagList);
#ifdef AMIGA

	tVPort *pVPort = memAllocFastClear(sizeof(tVPort));
	logWrite("Addr: %p\n", pVPort);

	const UWORD uwDefaultWidth = SCREEN_PAL_WIDTH;
	const UWORD uwDefaultHeight = -1;
	const UWORD uwDefaultBpp = 4; // 'Cuz copper is slower @ 5bpp & more in OCS

	// Determine parent view
	tView *pView = (tView*)tagGet(pTagList, vaTags, TAG_VPORT_VIEW, 0);
	if(!pView) {
		logWrite("ERR: no view ptr in TAG_VPORT_VIEW specified!\n");
		goto fail;
	}
	pVPort->pView = pView;
	logWrite("Parent view: %p\n", pView);

	// Calculate Y offset - beneath previous ViewPort
	pVPort->uwOffsY = 0;
	tVPort *pPrevVPort = pView->pFirstVPort;
	while(pPrevVPort) {
		pVPort->uwOffsY += pPrevVPort->uwHeight;
		pPrevVPort = pPrevVPort->pNext;
	}
	if(pVPort->uwOffsY && !(pView->uwFlags & VIEW_FLAG_GLOBAL_CLUT)) {
		pVPort->uwOffsY += 2; // TODO: not always required?
	}
	ULONG ulAddOffsY = tagGet(pTagList, vaTags, TAG_VPORT_OFFSET_TOP, 0);
	pVPort->uwOffsY += ulAddOffsY;
	logWrite("Offsets: %ux%u\n", pVPort->uwOffsX, pVPort->uwOffsY);

	// Get dimensions
	pVPort->uwWidth = tagGet(pTagList, vaTags, TAG_VPORT_WIDTH, uwDefaultWidth);
	pVPort->uwHeight = tagGet(pTagList, vaTags, TAG_VPORT_HEIGHT, uwDefaultHeight);
	if(pVPort->uwHeight == uwDefaultHeight) {
		pVPort->uwHeight = SCREEN_PAL_HEIGHT-pVPort->uwOffsY;
	}
	pVPort->ubBPP = tagGet(pTagList, vaTags, TAG_VPORT_BPP, uwDefaultBpp);
	logWrite(
		"Dimensions: %ux%u@%hu\n", pVPort->uwWidth, pVPort->uwHeight, pVPort->ubBPP
	);

	// Update view - add to vPort list
	++pView->ubVpCount;
	if(!pView->pFirstVPort) {
		pView->pFirstVPort = pVPort;
		logWrite("No prev VPorts - added to head\n");
	}
	else {
		pPrevVPort = pView->pFirstVPort;
		while(pPrevVPort->pNext) {
			pPrevVPort = pPrevVPort->pNext;
		}
		pPrevVPort->pNext = pVPort;
		logWrite("VPort added after %p\n", pPrevVPort);
	}

	// Palette tag
	UWORD *pSrcPalette = (UWORD*)tagGet(pTagList, vaTags, TAG_VPORT_PALETTE_PTR, 0);
	if(pSrcPalette) {
		UWORD uwPaletteSize = tagGet(pTagList, vaTags, TAG_VPORT_PALETTE_SIZE, 0xFFFF);
		if(uwPaletteSize == 0xFFFF) {
			logWrite("WARN: you must specify palette size in TAG_VPORT_PALETTE_SIZE\n");
		}
		else if(!uwPaletteSize || uwPaletteSize > 32) {
			logWrite("ERR: Wrong palette size: %hu\n", uwPaletteSize);
		}
		else {
			memcpy(pVPort->pPalette, pSrcPalette, uwPaletteSize * sizeof(UWORD));
		}
	}

	va_end(vaTags);
	logBlockEnd("vPortCreate()");
	return pVPort;
#endif // AMIGA
fail:
	va_end(vaTags);
	logBlockEnd("vPortCreate()");
	return 0;
}

void vPortDestroy(tVPort *pVPort) {
	logBlockBegin("vPortDestroy(pVPort: %p)", pVPort);
	tView *pView;
	tVPort *pPrevVPort, *pCurrVPort;

	pView = pVPort->pView;
	logWrite("Parent extView: %p\n", pView);
	pPrevVPort = 0;
	pCurrVPort = pView->pFirstVPort;
	while(pCurrVPort) {
		logWrite("found VP: %p...", pCurrVPort);
		if(pCurrVPort == pVPort) {
			logWrite(" gotcha!\n");

			// Remove from list
			if(pPrevVPort) {
				pPrevVPort->pNext = pCurrVPort->pNext;
			}
			else {
				pView->pFirstVPort = pCurrVPort->pNext;
			}
			--pView->ubVpCount;

			// Destroy managers
			logBlockBegin("Destroying managers");
			while(pCurrVPort->pFirstManager) {
				vPortRmManager(pCurrVPort, pCurrVPort->pFirstManager);
			}
			logBlockEnd("Destroying managers");

			// Free stuff
			memFree(pVPort, sizeof(tVPort));
			break;
		}
		else {
			logWrite("\n");
		}
		pPrevVPort = pCurrVPort;
		pCurrVPort = pCurrVPort->pNext;
	}
	logBlockEnd("vPortDestroy()");
}


void vPortUpdateCLUT(tVPort *pVPort) {
	// TODO: If not same CLUTs on all vports, there are 2 strategies to do them:
	// 1) Using the copperlist (copblock/raw copper instructions)
	// 2) By using CPU
	// 1st approach is better since it will always work, doesn't require any waits
	// So the only problem is implementing it using copblocks or raw copperlist.
	// Also some viewports which are one after another may use shared pallette, so
	// adding CLUT copper instructions between them is unnecessary.
	// I propose following implementation:
	// - for quick check, view contains flag VIEW_GLOBAL_CLUT if all viewports use
	//   same palette
	// - if VIEW_GLOBAL_CLUT is not found, every vport is checked for
	// VPORT_HAS_OWN_CLUT and if it's present then cop instructions are created
	// during vport creation and updated using this fn.
	// There is a problem that VPorts may change vertical size and position
	// (like expanding HUD to fullscreen like we did in Goblin Villages).
	// On copblocks implementing it is somewhat easy, but on raw copperlist
	// something clever must be done.
	if(pVPort->uwFlags & VIEWPORT_HAS_OWN_CLUT) {
		logWrite("TODO: implement vPortUpdateCLUT!\n");
	}
}

void vPortWaitForEnd(tVPort *pVPort) {
#ifdef AMIGA
	// Determine VPort end position
	UWORD uwEndPos = pVPort->uwOffsY + pVPort->uwHeight;
	uwEndPos += 0x2C; // Addition from DiWStrt
#if defined(ACE_DEBUG)
	if(uwEndPos >= 312) {
		logWrite("ERR: vPortWaitForEnd - too big wait pos: %04hx (%hu)\n", uwEndPos, uwEndPos);
		logWrite("\tVPort offs: %hu, height: %hu\n", pVPort->uwOffsY, pVPort->uwHeight);
	}
#endif

	// If current beam pos is past end pos, wait for start of next frame
	while(g_pRayPos->bfPosY > uwEndPos) {}
	// If current beam pos is before end pos, wait for it
	while(g_pRayPos->bfPosY < uwEndPos) {}
#endif // AMIGA
}

void vPortAddManager(tVPort *pVPort, tVpManager *pVpManager) {
	// podpiecie
	if(!pVPort->pFirstManager) {
		pVPort->pFirstManager = pVpManager;
		logWrite("Manager %p attached to head of VP %p\n", pVpManager, pVPort);
		return;
	}
	tVpManager *pVpCurr = pVPort->pFirstManager;
	// przewin przed menedzer o wyzszym numerze niz wstawiany
	while(pVpCurr->pNext && pVpCurr->pNext->ubId <= pVpManager->ubId) {
		if(pVpCurr->ubId <= pVpManager->ubId) {
			pVpCurr = pVpCurr->pNext;
		}
	}
	pVpManager->pNext = pVpCurr->pNext;
	pVpCurr->pNext = pVpManager;
	logWrite("Manager %p attached after manager %p of VP %p\n", pVpManager, pVpCurr, pVPort);
}

void vPortRmManager(tVPort *pVPort, tVpManager *pVpManager) {
	if(!pVPort->pFirstManager) {
		logWrite("ERR: vPort %p has no managers!\n", pVPort);
		return;
	}
	if(pVPort->pFirstManager == pVpManager) {
		logWrite("Destroying manager %u @addr: %p\n", pVpManager->ubId, pVpManager);
		pVPort->pFirstManager = pVpManager->pNext;
		pVpManager->destroy(pVpManager);
		return;
	}
	tVpManager *pParent = pVPort->pFirstManager;
	while(pParent->pNext) {
		if(pParent->pNext == pVpManager) {
			logWrite("Destroying manager %u @addr: %p\n", pVpManager->ubId, pVpManager);
			pParent->pNext = pVpManager->pNext;
			pVpManager->destroy(pVpManager);
			return;
		}
	}
	logWrite("ERR: vPort %p manager %p not found!\n", pVPort, pVpManager);
}

tVpManager *vPortGetManager(tVPort *pVPort, UBYTE ubId) {
	tVpManager *pManager;

	pManager = pVPort->pFirstManager;
	while(pManager) {
		if(pManager->ubId == ubId) {
			return pManager;
		}
		pManager = pManager->pNext;
	}
	return 0;
}
