#include <ace/managers/copper.h>

tCopManager g_sCopManager;

void copCreate(void) {
	logBlockBegin("copCreate()");
	
	// TODO: save previous copperlist
	
	// Create blank copperlist
	g_sCopManager.pBlankList = copListCreate();	
	
	// Set both buffers to blank copperlist
	g_sCopManager.pCopList = g_sCopManager.pBlankList;
	copProcess();
	copProcess();
	// Update copper-related regs
	custom.copjmp1 = 1;	
	custom.dmacon = DMAF_SETCLR | DMAF_COPPER;

	logBlockEnd("copCreate()");
}

void copDestroy(void) {
	logBlockBegin("copDestroy()");

	// Load system copperlist
	custom.cop1lc = (ULONG)GfxBase->copinit;
	custom.copjmp1 = 1;
	
	// Free blank copperlist
	// All others should be freed by user	
	copListDestroy(g_sCopManager.pBlankList);
	
	logBlockEnd("copDestroy()");
}

void copProcess(void) {
	// logBlockBegin("copProcess()");
	UBYTE ubNewStatus;
	tCopBfr *pBackBfr;
	tCopBlock *pBlock;
	tCopList *pCopList;
	
	pCopList = g_sCopManager.pCopList;
	pBackBfr = pCopList->pBackBfr;
	ubNewStatus = 0;
	
	// Realloc buffer memeory
	if(pCopList->ubStatus & STATUS_REALLOC) {
		// logBlockBegin("Realloc");
		// Free memory
		// logWrite("free mem: %u\n", pBackBfr->uwAllocSize);
		if(pBackBfr->uwAllocSize)
			memFree(pBackBfr->pList, pBackBfr->uwAllocSize);
		
		// Calculate new list size
		// logWrite("Alloc calc\n");
		if(pCopList->ubStatus & STATUS_REALLOC_CURR) {
			
			pBackBfr->uwAllocSize = 0;
			for(pBlock = pCopList->pFirstBlock; pBlock; pBlock = pBlock->pNext)
				pBackBfr->uwAllocSize += pBlock->uwMaxCmds;
			pBackBfr->uwAllocSize += pCopList->uwBlockCount + 1; // all WAITs + double WAIT
			pBackBfr->uwAllocSize *= sizeof(tCopCmd);
			// Pass realloc to next buffer
			ubNewStatus |= STATUS_REALLOC_PREV;
		}
		else
			pBackBfr->uwAllocSize = pCopList->pFrontBfr->uwAllocSize;
		
		// Alloc memory
		// logWrite("Alloc: %u\n", pBackBfr->uwAllocSize);
		pBackBfr->pList = memAllocChip(pBackBfr->uwAllocSize);
		// logBlockEnd("Realloc");
	}
	
	// Sort blocks if needed
	if(pCopList->ubStatus & STATUS_REORDER) {
		// logBlockBegin("reorder");
		UBYTE ubDone;
		tCopBlock *pPrev;
		
		do {
			ubDone = 1;
			pBlock = pCopList->pFirstBlock;
			pPrev = 0;
			while(pBlock->pNext) {
				if(pBlock->uWaitPos.ulYX > pBlock->pNext->uWaitPos.ulYX) {
					if(!pPrev) {
						pCopList->pFirstBlock = pBlock->pNext;
						pBlock->pNext = pCopList->pFirstBlock->pNext;
						pCopList->pFirstBlock->pNext = pBlock;
					}
					else {
						pPrev->pNext = pBlock->pNext;
						pBlock->pNext = pPrev->pNext->pNext;
						pPrev->pNext->pNext = pBlock;
					}
					ubDone = 0;
					break;
				}
				pPrev = pBlock;
				pBlock = pBlock->pNext;
			}
			
		} while(!ubDone);
		// logBlockEnd("reorder");
	}
	
	// Update buffer data
	if(pCopList->ubStatus & STATUS_UPDATE) {
		UWORD uwListPos;
		UBYTE ubWasLimitY;
		
		// Update buffers if their sizes haven't changed
		pBlock = pCopList->pFirstBlock;
		uwListPos = 0;
		ubWasLimitY = 0;
		// /////////////////////////////////////////////////////////////////////////
		// Disabled 'cuz it's broken
		// This part should update content of modified blocks, which sizes were
		// not changed. To test fix candidates, run copper test in ACE showcase.
		// /////////////////////////////////////////////////////////////////////////
		while(0 && pBlock && !pBlock->ubResized) { 
			if(!pBlock->ubDisabled) {
				if(pBlock->ubUpdated) {
					// Update WAIT
					if(pBlock->uWaitPos.sUwCoord.uwY > 0xFF) {
						if(!ubWasLimitY) {
							// FIXME: If copper block was moved down, so that WAIT suddenly
							// consists of 2 instructions instead of 1, such block should be
							// treated as resized one. Same refers to block, which changed
							// from 2 WAITs to 1
							copSetWait((tCopWaitCmd*)&pBackBfr->pList[uwListPos], 0xDF, 0xFF);
							++uwListPos;
							ubWasLimitY = 1;
						}
						copSetWait((tCopWaitCmd*)&pBackBfr->pList[uwListPos], pBlock->uWaitPos.sUwCoord.uwX, pBlock->uWaitPos.sUwCoord.uwY & 0xFF);
						++uwListPos;
					}
					else {
						copSetWait((tCopWaitCmd*)&pBackBfr->pList[uwListPos], pBlock->uWaitPos.sUwCoord.uwX, pBlock->uWaitPos.sUwCoord.uwY);
						++uwListPos;
					}
					
					// Copy MOVEs
					CopyMem(pBlock->pCmds, &pBackBfr->pList[uwListPos], pBlock->uwCurrCount*sizeof(tCopCmd));
					// logWrite("Copied %u instructions from block\n", pBlock->uwCurrCount);
					--pBlock->ubUpdated;
				}
				uwListPos += pBlock->uwCurrCount;
			}
			pBlock = pBlock->pNext;
		}
		// /////////////////////////////////////////////////////////////////////////
		// End of broken part
		// /////////////////////////////////////////////////////////////////////////
		
		// Do full merge on remaining blocks
		while(pBlock) {
			if(!pBlock->ubDisabled) {
				if(pBlock->ubResized)
					--pBlock->ubResized;
				if(pBlock->ubUpdated)
					--pBlock->ubUpdated;
				
				// Update WAIT
				if(pBlock->uWaitPos.sUwCoord.uwY > 0xFF) {
					if(!ubWasLimitY) {
						copSetWait((tCopWaitCmd*)&pBackBfr->pList[uwListPos], 0xDF, 0xFF);
						++uwListPos;
						ubWasLimitY = 1;
					}
					copSetWait((tCopWaitCmd*)&pBackBfr->pList[uwListPos], pBlock->uWaitPos.sUwCoord.uwX, pBlock->uWaitPos.sUwCoord.uwY & 0xFF);
					++uwListPos;
				}
				else {
					copSetWait((tCopWaitCmd*)&pBackBfr->pList[uwListPos], pBlock->uWaitPos.sUwCoord.uwX, pBlock->uWaitPos.sUwCoord.uwY);
					++uwListPos;
				}
				
				// Copy MOVEs
				CopyMem(pBlock->pCmds, &pBackBfr->pList[uwListPos], pBlock->uwCurrCount*sizeof(tCopCmd));
				uwListPos += pBlock->uwCurrCount;
			}			
			pBlock = pBlock->pNext;
		}
		
		// Add 0xFFFF terminator
		copSetWait((tCopWaitCmd*)&pBackBfr->pList[uwListPos], 0xFF, 0xFF);
		++uwListPos;
		
		pCopList->pBackBfr->uwCmdCount = uwListPos;
		
		if(pCopList->ubStatus & STATUS_UPDATE_CURR)
			ubNewStatus |= STATUS_UPDATE_PREV;
		// logBlockEnd("update");
	}

	// Swap copper buffers
	tCopBfr *pTmp;
	pTmp = pCopList->pFrontBfr;
	pCopList->pFrontBfr = pCopList->pBackBfr;
	pCopList->pBackBfr = pTmp;
	custom.cop1lc = (ULONG)((void *)pCopList->pFrontBfr->pList);
	
	// Update status code
	pCopList->ubStatus = ubNewStatus;
	// logBlockEnd("copProcess()");
}

tCopList *copListCreate(void) {
	tCopList *pCopList;
	logBlockBegin("copListCreate()");
	
	// Create copperlist stub
	pCopList = memAllocFastClear(sizeof(tCopList));
	logWrite("Addr: %p\n", pCopList);
	
	pCopList->pFrontBfr = memAllocFastClear(sizeof(tCopBfr));
	pCopList->pBackBfr = memAllocFastClear(sizeof(tCopBfr));
		
	logBlockEnd("copListCreate()");
	return pCopList;
}

void copListDestroy(tCopList *pCopList) {
	logBlockBegin("copListDestroy(pCopList: %p)", pCopList);
	
	// Free copperlist buffers
	while(pCopList->pFirstBlock)
		copBlockDestroy(pCopList, pCopList->pFirstBlock);
	
	// Free front buffer
	if(pCopList->pFrontBfr->uwAllocSize)
		memFree(pCopList->pFrontBfr->pList, pCopList->pFrontBfr->uwAllocSize);
	memFree(pCopList->pFrontBfr, sizeof(tCopBfr));
	
	// Free back buffer
	if(pCopList->pBackBfr->uwAllocSize)
		memFree(pCopList->pBackBfr->pList, pCopList->pBackBfr->uwAllocSize);
	memFree(pCopList->pBackBfr, sizeof(tCopBfr));
	
	// Free main struct
	memFree(pCopList, sizeof(tCopList));
	
	logBlockEnd("copListDestroy()");
}

tCopBlock *copBlockCreate(tCopList *pCopList, UWORD uwMaxCmds, UWORD uwWaitX, UWORD uwWaitY) {
	tCopBlock *pBlock;
	
	logBlockBegin("copBlockCreate(pCopList: %p, uwMaxCmds: %u, uwWaitX: %u, uwWaitY: %u)", pCopList, uwMaxCmds, uwWaitX, uwWaitY);
	pBlock = memAllocFastClear(sizeof(tCopBlock));
	logWrite("pAddr: %p\n", pBlock);
	pBlock->uwMaxCmds = uwMaxCmds; // MOVEs only
	pBlock->pCmds     = memAllocFast(sizeof(tCopCmd) * pBlock->uwMaxCmds);
	
	copWait(pCopList, pBlock, uwWaitX, uwWaitY);
	
	// Add to list
	logWrite("Head: %p\n", pCopList->pFirstBlock);
	if(!pCopList->pFirstBlock || pBlock->uWaitPos.ulYX < pCopList->pFirstBlock->uWaitPos.ulYX) {
		pBlock->pNext = pCopList->pFirstBlock;
		pCopList->pFirstBlock = pBlock;
		logWrite("Added as head, next: %p\n", pBlock->pNext);
	}
	else {
		tCopBlock *pPrev;
		
		pPrev = pCopList->pFirstBlock;
		while(pPrev->pNext && pPrev->pNext->uWaitPos.ulYX < pBlock->uWaitPos.ulYX)
			pPrev = pPrev->pNext;
		pBlock->pNext = pPrev->pNext;
		pPrev->pNext = pBlock;
	}
	pCopList->ubStatus |= STATUS_REALLOC_CURR | STATUS_REORDER | STATUS_UPDATE_CURR;
	++pCopList->uwBlockCount;
	
	logBlockEnd("copBlockCreate()");
	return pBlock;
}

void copBlockDestroy(tCopList *pCopList, tCopBlock *pBlock) {
	logBlockBegin("copBlockDestroy(pCopList: %p, pBlock: %p)", pCopList, pBlock);
	
	// Remove from list
	if(pBlock == pCopList->pFirstBlock)
		pCopList->pFirstBlock = pBlock->pNext;
	else {
		tCopBlock *pCurr;
		
		pCurr = pCopList->pFirstBlock;
		while(pCurr->pNext && pCurr->pNext != pBlock)
			pCurr = pCurr->pNext;
		
		if(pCurr->pNext)
			pCurr->pNext = pBlock->pNext;
		else
			logWrite("ERR: Can't find block %p\n", pBlock);
	}
		
	// Free mem
	memFree(pBlock->pCmds, sizeof(tCopCmd)*pBlock->uwMaxCmds);
	memFree(pBlock, sizeof(tCopBlock));
	
	pCopList->ubStatus |= STATUS_REALLOC_CURR;
	--pCopList->uwBlockCount;
	
	logBlockEnd("copBlockDestroy()");
}

void copBlockEnable(tCopList *pCopList, tCopBlock *pBlock) {
	pBlock->ubDisabled = 0;
	pBlock->ubUpdated = 2;
	pBlock->pNext->ubUpdated = 2;
	pCopList->ubStatus |= STATUS_UPDATE;
}

void copBlockDisable(tCopList *pCopList, tCopBlock *pBlock) {
	pBlock->ubDisabled = 0;
	pBlock->pNext->ubUpdated = 2;
	pCopList->ubStatus |= STATUS_UPDATE;
}

void copWait(tCopList *pCopList, tCopBlock *pBlock, UWORD uwX, UWORD uwY) {
	pBlock->uWaitPos.sUwCoord.uwY  = uwY;
	pBlock->uWaitPos.sUwCoord.uwX  = uwX;

	pBlock->ubUpdated = 2;
	pCopList->ubStatus |= STATUS_UPDATE;
}

void copMove(tCopList *pCopList, tCopBlock *pBlock, void *reg, UWORD uwValue) {
	tCopMoveCmd *pCmd;
	
	pCmd = (tCopMoveCmd*)&pBlock->pCmds[pBlock->uwCurrCount];
	++pBlock->uwCurrCount;
	
	pCmd->bfUnused = 0;
	pCmd->bfDestAddr = (ULONG)reg - (ULONG)((UBYTE *)&custom);
	pCmd->bfValue = uwValue;
	
	pBlock->ubUpdated = 2;
	pBlock->ubResized = 2;
	pCopList->ubStatus |= STATUS_UPDATE;
}

void copSetWait(tCopWaitCmd *pWaitCmd, UBYTE ubX, UBYTE ubY) {
	pWaitCmd->bfWaitY         = ubY;
	pWaitCmd->bfWaitX         = ubX >> 1;
	pWaitCmd->bfIsWait        = 1;
	pWaitCmd->bfBlitterIgnore = 1;
	pWaitCmd->bfVE            = 0x7F;
	pWaitCmd->bfHE            = 0x7F;
	pWaitCmd->bfIsSkip        = 0;
}

void copDump(void) {
	UWORD i;
	tCopList *pCopList;
	tCopBlock *pBlock;
	tCopCmd *pCmds;
	
	logBlockBegin("copDump()");
	
	pCopList = g_sCopManager.pCopList;
	logWrite("Copperlist %p cmd count: %u/%u\n", pCopList, pCopList->pFrontBfr->uwCmdCount, pCopList->pFrontBfr->uwAllocSize>>2);
	pCmds = pCopList->pFrontBfr->pList;
	for(i = 0; i != pCopList->pFrontBfr->uwCmdCount; ++i)
		if(pCmds[i].sWait.bfIsWait)
			logWrite("%08X - WAIT: %hX,%hX\n", pCmds[i].ulCode, pCmds[i].sWait.bfWaitX << 1, pCmds[i].sWait.bfWaitY);
		else
			logWrite("%08X - MOVE: %03X := %X\n", pCmds[i].ulCode,  pCmds[i].sMove.bfDestAddr, pCmds[i].sMove.bfValue);
	
	logWrite("Copper block count: %u\n", pCopList->uwBlockCount);
	pBlock = pCopList->pFirstBlock;
	while(pBlock) {
		logWrite("Block %p has %u/%u cmds:\n", pBlock, pBlock->uwCurrCount, pBlock->uwMaxCmds);
		logPushIndent();
		pCmds = pBlock->pCmds;
		for(i = 0; i != pBlock->uwCurrCount; ++i)
			if(pCmds[i].sWait.bfIsWait)
				logWrite("%08X - WAIT: %hX,%hX\n", pCmds[i].ulCode, pCmds[i].sWait.bfWaitX, pCmds[i].sWait.bfWaitY);
			else
				logWrite("%08X - MOVE: %03X := %X\n", pCmds[i].ulCode,  pCmds[i].sMove.bfDestAddr, pCmds[i].sMove.bfValue);
		
		logPopIndent();
		pBlock = pBlock->pNext;
	}
	
	logBlockEnd("copDump()");
}