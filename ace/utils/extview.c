#include <ace/utils/extview.h>
#include <limits.h>
#include <ace/utils/tag.h>

tView *viewCreate(void *pTags, ...) {
	logBlockBegin("viewCreate(pTags: %p)", pTags);

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
	else
		pView->pCopList = copListCreate(0, TAG_DONE);

	// Additional CLUT tags
	if(tagGet(pTags, vaTags, TAG_VIEW_GLOBAL_CLUT, 0)) {
		pView->uwFlags |= VIEW_FLAG_GLOBAL_CLUT;
		logWrite("Global CLUT mode enabled\n");
	}

	va_end(vaTags);
	logBlockEnd("viewCreate()");
	return pView;
}

void viewDestroy(tView *pView) {
	logBlockBegin("viewDestroy(pView: %p)", pView);

	if(g_sCopManager.pCopList == pView->pCopList)
		viewLoad(0);

	// Free all attached viewports
	while(pView->pFirstVPort)
		vPortDestroy(pView->pFirstVPort);

	// Free view
	logWrite("Freeing copperlists...\n");
	copListDestroy(pView->pCopList);
	memFree(pView, sizeof(tView));
	logBlockEnd("viewDestroy()");
}

void viewProcessManagers(tView *pView) {
	tVPort *pVPort;
	tVpManager *pManager;
	pVPort = pView->pFirstVPort;
	while(pVPort) {
		pManager = pVPort->pFirstManager;
		while(pManager) {
			pManager->process(pManager);
			pManager = pManager->pNext;
		}
		pVPort = pVPort->pNext;
	}
}

void viewUpdateCLUT(tView *pView) {
	if(pView->uwFlags & VIEW_FLAG_GLOBAL_CLUT)
		CopyMem(pView->pFirstVPort->pPalette, custom.color, 32);
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
	WaitTOF();
	ULONG uwDMA;
	if(!pView) {
		g_sCopManager.pCopList = g_sCopManager.pBlankList;
		uwDMA = DMAF_RASTER;
		custom.bplcon0 = 0; // No output
		custom.fmode = 0;   // AGA fix
		UBYTE i;
		for(i = 0; i != 6; ++i)
			custom.bplpt[i] = 0;
		custom.bpl1mod = 0;
		custom.bpl2mod = 0;
	}
	else {
		g_sCopManager.pCopList = pView->pCopList;
		custom.bplcon0 = (pView->pFirstVPort->ubBPP << 12) | (1 << 9); // BPP + composite output
		custom.fmode = 0;        // AGA fix
		custom.diwstrt = 0x2C81; // VSTART: 0x2C, HSTART: 0x81
		custom.diwstop = 0x2CC1; // VSTOP: 0x2C, HSTOP: 0xC1
		viewUpdateCLUT(pView);
		uwDMA = DMAF_SETCLR | DMAF_RASTER;
	}
	copProcessBlocks();
	custom.copjmp1 = 1;
	custom.dmacon = uwDMA;
	WaitTOF();
	logBlockEnd("viewLoad()");
}

tVPort *vPortCreate(void *pTagList, ...) {
	logBlockBegin("vPortCreate(pTagList: %p)", pTagList);

	tVPort *pVPort = memAllocFastClear(sizeof(tVPort));
	logWrite("Addr: %p\n", pVPort);

	const UWORD uwDefaultWidth = 320;
	const UWORD uwDefaultHeight = -1;
	const UWORD uwDefaultBpp = 4; // 'Cuz copper is slower @ 5bpp & more in OCS

	va_list vaTags;
	va_start(vaTags, pTagList);

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
	if(pVPort->uwOffsY && !(pView->uwFlags & VIEW_FLAG_GLOBAL_CLUT))
		pVPort->uwOffsY += 2; // TODO: not always required?
	ULONG ulAddOffsY = tagGet(pTagList, vaTags, TAG_VPORT_OFFSET_TOP, 0);
	pVPort->uwOffsY += ulAddOffsY;
	logWrite("Offsets: %ux%u\n", pVPort->uwOffsX, pVPort->uwOffsY);

	// Get dimensions
	pVPort->uwWidth = tagGet(pTagList, vaTags, TAG_VPORT_WIDTH, uwDefaultWidth);
	pVPort->uwHeight = tagGet(pTagList, vaTags, TAG_VPORT_HEIGHT, uwDefaultHeight);
	if(pVPort->uwHeight == uwDefaultHeight)
		pVPort->uwHeight = 256-pVPort->uwOffsY;
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
		while(pPrevVPort->pNext)
			pPrevVPort = pPrevVPort->pNext;
		pPrevVPort->pNext = pVPort;
		logWrite("VPort added after %p\n", pPrevVPort);
	}

	va_end(vaTags);
	logBlockEnd("vPortCreate()");
	return pVPort;

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
			if(pPrevVPort)
				pPrevVPort->pNext = pCurrVPort->pNext;
			else
				pView->pFirstVPort = pCurrVPort->pNext;
			--pView->ubVpCount;

			// Destroy managers
			logBlockBegin("Destroying managers");
			while(pCurrVPort->pFirstManager)
				vPortRmManager(pCurrVPort, pCurrVPort->pFirstManager);
			logBlockEnd("Destroying managers");

			// Free stuff
			memFree(pVPort, sizeof(tVPort));
			break;
		}
		else
			logWrite("\n");
		pPrevVPort = pCurrVPort;
		pCurrVPort = pCurrVPort->pNext;
	}
	logBlockEnd("vPortDestroy()");
}

void vPortUpdateCLUT(tVPort *pVPort) {
	// TODO: blok palety kolorow, priorytety na copperliscie
}

void vPortWaitForEnd(tVPort *pVPort) {
	UWORD uwEndPos;
	UWORD uwCurrFrame;

	// Determine VPort end position
	uwEndPos = pVPort->uwOffsY + pVPort->uwHeight + 0x2C; // Addition from DiWStrt
	if(vhPosRegs->uwPosY < uwEndPos) {
		// If current beam is before pos, wait for pos @ current frame
		while(vhPosRegs->uwPosY < uwEndPos);
	}
	else {
		uwCurrFrame = g_sTimerManager.uwFrameCounter;
		while(
			vhPosRegs->uwPosY < uwEndPos ||
			g_sTimerManager.uwFrameCounter == uwCurrFrame
		);
	}

	// Otherwise wait for pos @ next frame
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
		if(pVpCurr->ubId <= pVpManager->ubId)
			pVpCurr = pVpCurr->pNext;
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
		if(pManager->ubId == ubId)
			return pManager;
		pManager = pManager->pNext;
	}
	return 0;
}

/*
void extViewFadeOut(tExtView *pExtView) {
	tExtVPort *pVPort;
	BYTE bFadeStep;
	UBYTE ubColorIdx;
	UWORD pTmpPalette[32]; // TODO: view bpp aware
	UBYTE ubR, ubG, ubB;

	for (bFadeStep = 15; bFadeStep >= 0; --bFadeStep) {
		pVPort = (tExtVPort*)pExtView->sView.ViewPort;
		while(pVPort) {
			for (ubColorIdx = 0; ubColorIdx != 32; ++ubColorIdx) {
				// Wyluskanie skladowych
				ubR = (pVPort->pPalette[ubColorIdx] >> 8) & 0xF;
				ubG = (pVPort->pPalette[ubColorIdx] >> 4) & 0xF;
				ubB = (pVPort->pPalette[ubColorIdx] >> 0) & 0xF;
				// Przemnozenie i sciecie skladowych
				ubR = ((ubR * bFadeStep) >> 4) & 0xF;
				ubG = ((ubG * bFadeStep) >> 4) & 0xF;
				ubB = ((ubB * bFadeStep) >> 4) & 0xF;
				// Zlozenie w kolor
				pTmpPalette[ubColorIdx] = (ubR << 8) + (ubG << 4) + (ubB << 0);
			}
			LoadRGB4(&pVPort->sVPort, pTmpPalette, pVPort->sVPort.ColorMap->Count);
			pVPort = (tExtVPort*)pVPort->sVPort.Next;
		}
		WaitTOF();
	}

	// Zapisz trwale palete aktualnego bufora
	// MakeVPort(pExtView->pView, pExtView->pMainViewPort);
	// MrgCop(pExtView->pView);

	// Zaktualizuj palete drugiego bufora
	// swapScreenBuffers(pExtView);
	// LoadRGB4(pExtView->pMainViewPort, pTmpPalette, 1 << WINDOW_SCREEN_BPP);
	// MakeVPort(pExtView->pView, pExtView->pMainViewPort);
	// MrgCop(pExtView->pView);

	// Wroc do bufora wyjsciowego
	// swapScreenBuffers(pExtView);
}

void extViewFadeIn(tExtView *pExtView) {
	tExtVPort *pVPort;
	BYTE bFadeStep;
	UBYTE ubColorIdx;
	UWORD pTmpPalette[32]; // TODO: view bpp aware
	UBYTE ubR, ubG, ubB;

	for (bFadeStep = 0; bFadeStep <= 15; ++bFadeStep) {
		pVPort = (tExtVPort*)pExtView->sView.ViewPort;
		while(pVPort) {
			for(ubColorIdx = 0; ubColorIdx != 32; ++ubColorIdx) {
				// Wyluskanie skladowych
				ubR = (pVPort->pPalette[ubColorIdx] >> 8) & 0xF;
				ubG = (pVPort->pPalette[ubColorIdx] >> 4) & 0xF;
				ubB = (pVPort->pPalette[ubColorIdx] >> 0) & 0xF;
				// Przemnozenie i sciecie skladowych
				ubR = ((ubR * bFadeStep) >> 4) & 0xF;
				ubG = ((ubG * bFadeStep) >> 4) & 0xF;
				ubB = ((ubB * bFadeStep) >> 4) & 0xF;
				// Zlozenie w kolor
				pTmpPalette[ubColorIdx] = (ubR << 8) + (ubG << 4) + (ubB << 0);
			}
			LoadRGB4(&pVPort->sVPort, pTmpPalette, pVPort->sVPort.ColorMap->Count);
			pVPort = (tExtVPort*)pVPort->sVPort.Next;
		}
		WaitTOF();
	}

	WaitTOF();
	pVPort = (tExtVPort*)pExtView->sView.ViewPort;
	while(pVPort) {
		LoadRGB4(&pVPort->sVPort, pVPort->pPalette, pVPort->sVPort.ColorMap->Count);
		pVPort = (tExtVPort*)pVPort->sVPort.Next;
	}
}*/
