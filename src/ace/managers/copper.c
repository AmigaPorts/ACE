/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/copper.h>
#ifdef AMIGA
#include <stdarg.h>
#include <ace/managers/system.h>
#include <ace/utils/tag.h>
#include <limits.h>

tCopManager g_sCopManager;

void copCreate(void) {
	logBlockBegin("copCreate()");

	// TODO: save previous copperlist

	// Create blank copperlist
	g_sCopManager.pBlankList = copListCreate(0, TAG_DONE);

	// Set both buffers to blank copperlist
	g_sCopManager.pCopList = g_sCopManager.pBlankList;
	copProcessBlocks();
	copProcessBlocks();
	// Update copper-related regs
	g_pCustom->copjmp1 = 1;
	systemSetDmaBit(DMAB_COPPER, 1);

	logBlockEnd("copCreate()");
}

void copDestroy(void) {
	logBlockBegin("copDestroy()");

	// Load system copperlist
	systemSetDmaBit(DMAB_COPPER, 0);
	g_pCustom->cop1lc = (ULONG)GfxBase->copinit;
	g_pCustom->copjmp1 = 1;

	// Free blank copperlist
	// All others should be freed by user
	copListDestroy(g_sCopManager.pBlankList);

	logBlockEnd("copDestroy()");
}

void copSwapBuffers(void) {
	tCopBfr *pTmp;
	tCopList *pCopList;

	pCopList = g_sCopManager.pCopList;
	g_pCustom->cop1lc = (ULONG)((void *)pCopList->pBackBfr->pList);
	pTmp = pCopList->pFrontBfr;
	pCopList->pFrontBfr = pCopList->pBackBfr;
	pCopList->pBackBfr = pTmp;
}

void copDumpCmd(tCopCmd *pCmd) {
		if(pCmd->sWait.bfIsWait) {
			logWrite(
				"@%p: %08X - WAIT: %hu,%hu%s\n",
				pCmd, pCmd->ulCode, pCmd->sWait.bfWaitX << 1, pCmd->sWait.bfWaitY,
				!pCmd->sWait.bfBlitterIgnore ? " & blit done" : ""
			);
		}
		else {
			logWrite(
				"@%p: %08X - MOVE: %03X := %04X\n",
				pCmd, pCmd->ulCode,  pCmd->sMove.bfDestAddr, pCmd->sMove.bfValue
			);
		}
}

void copDumpBlocks(void) {
	systemUse();
	logBlockBegin("copDumpBlocks()");

	tCopList *pCopList = g_sCopManager.pCopList;
	logWrite(
		"Copperlist %p cmd count: %u/%u\n",
		pCopList, pCopList->pFrontBfr->uwCmdCount,
		pCopList->pFrontBfr->uwAllocSize / 4
	);
	tCopCmd *pCmds = pCopList->pFrontBfr->pList;
	for(UWORD i = 0; i < pCopList->pFrontBfr->uwCmdCount; ++i) {
		copDumpCmd(&pCmds[i]);
	}

	logWrite("Copper block count: %u\n", pCopList->uwBlockCount);
	tCopBlock *pBlock = pCopList->pFirstBlock;
	while(pBlock) {
		if(pBlock->ubDisabled) {
			logWrite("DISABLED ");
		}
		logWrite(
			"Block %p starts at %u,%u and has %u/%u cmds:\n",
			pBlock, pBlock->uWaitPos.uwX, pBlock->uWaitPos.uwY,
			pBlock->uwCurrCount, pBlock->uwMaxCmds
		);
		logPushIndent();
		pCmds = pBlock->pCmds;
		for(UWORD i = 0; i < pBlock->uwCurrCount; ++i) {
			copDumpCmd(&pCmds[i]);
		}

		logPopIndent();
		pBlock = pBlock->pNext;
	}

	logBlockEnd("copDumpBlocks()");
	systemUnuse();
}

void copDumpBfr(tCopBfr *pBfr) {
	UWORD i;

	systemUse();
	logBlockBegin("copDumpBfr(pBfr: %p)", pBfr);
	logWrite("Alloc size: %u, cmd count: %u\n", pBfr->uwAllocSize, pBfr->uwCmdCount);
	for(i = 0; i != pBfr->uwCmdCount; ++i) {
		copDumpCmd(&pBfr->pList[i]);
	}

	logBlockEnd("copDumpBfr");
	systemUnuse();
}

tCopList *copListCreate(void *pTagList, ...) {
	va_list vaTags;
	va_start(vaTags, pTagList);
	tCopList *pCopList;
	logBlockBegin("copListCreate()");

	// Create copperlist stub
	pCopList = memAllocFastClear(sizeof(tCopList));
	logWrite("Addr: %p\n", pCopList);
	pCopList->pFrontBfr = memAllocFastClear(sizeof(tCopBfr));
	pCopList->pBackBfr = memAllocFastClear(sizeof(tCopBfr));

	// Handle raw copperlist creation
	 pCopList->ubMode = tagGet(pTagList, vaTags, TAG_COPPER_LIST_MODE, COPPER_MODE_BLOCK);
	if(pCopList->ubMode	== COPPER_MODE_RAW) {
		const ULONG ulInvalidSize = ULONG_MAX;
		ULONG ulListSize = tagGet(
			pTagList, vaTags, TAG_COPPER_RAW_COUNT, ulInvalidSize
		);
		if(ulListSize == ulInvalidSize) {
			logWrite("ERR: no size specified for raw list\n");
			goto fail;
		}
		if(ulListSize > USHRT_MAX) {
			logWrite(
				"ERR: raw copperlist size too big: %lu, max is %u\n",
				ulListSize, USHRT_MAX
			);
			goto fail;
		}
		logWrite("RAW mode, size: %lu + WAIT(0xFFFF)\n", ulListSize);
		// Front bfr
		pCopList->pFrontBfr->uwCmdCount = ulListSize+1;
		pCopList->pFrontBfr->uwAllocSize = (ulListSize+1)*sizeof(tCopCmd);
		pCopList->pFrontBfr->pList = memAllocChipClear(pCopList->pFrontBfr->uwAllocSize);
		copSetWait(&pCopList->pFrontBfr->pList[ulListSize].sWait, 0xFF, 0xFF);
		// Back bfr
		pCopList->pBackBfr->uwCmdCount = ulListSize+1;
		pCopList->pBackBfr->uwAllocSize = (ulListSize+1)*sizeof(tCopCmd);
		pCopList->pBackBfr->pList = memAllocChipClear(pCopList->pBackBfr->uwAllocSize);
		copSetWait(&pCopList->pBackBfr->pList[ulListSize].sWait, 0xFF, 0xFF);
	}
	else {
		logWrite("BLOCK mode\n");
	}

	logBlockEnd("copListCreate()");
	va_end(vaTags);
	return pCopList;

fail:
	va_end(vaTags);
	copListDestroy(pCopList);
	logBlockEnd("copListCreate()");
	return 0;
}

void copListDestroy(tCopList *pCopList) {
	logBlockBegin("copListDestroy(pCopList: %p)", pCopList);

	// Free copperlist buffers
	while(pCopList->pFirstBlock) {
		copBlockDestroy(pCopList, pCopList->pFirstBlock);
	}

	// Free front buffer
	if(pCopList->pFrontBfr->uwAllocSize) {
		memFree(pCopList->pFrontBfr->pList, pCopList->pFrontBfr->uwAllocSize);
	}
	memFree(pCopList->pFrontBfr, sizeof(tCopBfr));

	// Free back buffer
	if(pCopList->pBackBfr->uwAllocSize) {
		memFree(pCopList->pBackBfr->pList, pCopList->pBackBfr->uwAllocSize);
	}
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

	copBlockWait(pCopList, pBlock, uwWaitX, uwWaitY);

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
		while(pPrev->pNext && pPrev->pNext->uWaitPos.ulYX < pBlock->uWaitPos.ulYX) {
			pPrev = pPrev->pNext;
		}
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
	if(pBlock == pCopList->pFirstBlock) {
		pCopList->pFirstBlock = pBlock->pNext;
	}
	else {
		tCopBlock *pCurr;

		pCurr = pCopList->pFirstBlock;
		while(pCurr->pNext && pCurr->pNext != pBlock) {
			pCurr = pCurr->pNext;
		}

		if(pCurr->pNext) {
			pCurr->pNext = pBlock->pNext;
		}
		else {
			logWrite("ERR: Can't find block %p\n", pBlock);
		}
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
	pBlock->ubDisabled = 1;
	pBlock->pNext->ubUpdated = 2;
	pCopList->ubStatus |= STATUS_UPDATE;
}

UBYTE copBfrRealloc(void) {
	tCopList *pCopList;
	tCopBfr *pBackBfr;
	tCopBlock *pBlock;
	UBYTE ubNewStatus;

	pCopList = g_sCopManager.pCopList;
	pBackBfr = pCopList->pBackBfr;

	// Free memory
	if(pBackBfr->uwAllocSize) {
		memFree(pBackBfr->pList, pBackBfr->uwAllocSize);
	}

	// Calculate new list size
	if(pCopList->ubStatus & STATUS_REALLOC_CURR) {

		pBackBfr->uwAllocSize = 0;
		for(pBlock = pCopList->pFirstBlock; pBlock; pBlock = pBlock->pNext) {
			pBackBfr->uwAllocSize += 1 + pBlock->uwMaxCmds; // WAIT + MOVEs
		}
		pBackBfr->uwAllocSize += 2; // final WAIT + room for double WAIT
		pBackBfr->uwAllocSize *= sizeof(tCopCmd);
		// Pass realloc to next buffer
		ubNewStatus = STATUS_REALLOC_PREV;
	}
	else {
		// If realloc just propagates to next buffer, calculations aren't necessary
		pBackBfr->uwAllocSize = pCopList->pFrontBfr->uwAllocSize;
		ubNewStatus = 0;
	}

	// Alloc memory
	pBackBfr->pList = memAllocChip(pBackBfr->uwAllocSize);
	return ubNewStatus;
}

void copReorderBlocks(void) {
	tCopList *pCopList = g_sCopManager.pCopList;
	UBYTE ubDone;
	do {
		ubDone = 1;
		tCopBlock *pBlock = pCopList->pFirstBlock;
		tCopBlock *pPrev = 0;
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
}

UBYTE copUpdateFromBlocks(void) {
	tCopList *pCopList;
	tCopBlock *pBlock;
	tCopBfr *pBackBfr;
	UWORD uwListPos;
	UBYTE ubWasLimitY;

	pCopList = g_sCopManager.pCopList;
	pBackBfr = pCopList->pBackBfr;

	pBlock = pCopList->pFirstBlock;
	uwListPos = 0;
	ubWasLimitY = 0;

	// Update buffers if their sizes haven't changed
	// ///////////////////////////////////////////////////////////////////////////
	// Disabled 'cuz it's broken
	// This part should update content of modified blocks, which sizes were
	// not changed. To test fix candidates, run copper test in ACE showcase.
	// ///////////////////////////////////////////////////////////////////////////
	/*
	while(0 && pBlock && !pBlock->ubResized) {
		if(!pBlock->ubDisabled) {
			if(pBlock->ubUpdated) {
				// Update WAIT
				if(pBlock->uWaitPos.uwY > 0xFF) {
					// FIXME: double WAIT only when previous line ended before some pos
					if(!ubWasLimitY) {
						// FIXME: If copper block was moved down, so that WAIT suddenly
						// consists of 2 instructions instead of 1 AND is first y>0xFF,
						// such block should be treated as resized one. Same refers to
						// block, which changed from 2 WAITs to 1.
						copSetWait((tCopWaitCmd*)&pBackBfr->pList[uwListPos], 0xDF, 0xFF);
						++uwListPos;
						ubWasLimitY = 1;
					}
					copSetWait((tCopWaitCmd*)&pBackBfr->pList[uwListPos], pBlock->uWaitPos.uwX, pBlock->uWaitPos.uwY & 0xFF);
					++uwListPos;
				}
				else {
					copSetWait((tCopWaitCmd*)&pBackBfr->pList[uwListPos], pBlock->uWaitPos.uwX, pBlock->uWaitPos.uwY);
					++uwListPos;
				}

				// Copy MOVEs
				CopyMem(pBlock->pCmds, &pBackBfr->pList[uwListPos], pBlock->uwCurrCount*sizeof(tCopCmd));
				--pBlock->ubUpdated;
			}
			uwListPos += pBlock->uwCurrCount;
		}
		pBlock = pBlock->pNext;
	}
	*/
	// ///////////////////////////////////////////////////////////////////////////
	// End of broken part
	// ///////////////////////////////////////////////////////////////////////////

	// Do full merge on remaining blocks
	for(; pBlock; pBlock = pBlock->pNext) {
		if(pBlock->ubDisabled) {
			continue;
		}
		if(pBlock->ubResized) {
			--pBlock->ubResized;
		}
		if(pBlock->ubUpdated) {
			--pBlock->ubUpdated;
		}

		// Update WAIT
		if(pBlock->uWaitPos.uwY > 0xFF) {
			// FIXME: double WAIT only when previous line ended before some pos
			if(!ubWasLimitY) {
				copSetWait((tCopWaitCmd*)&pBackBfr->pList[uwListPos], 0xDF, 0xFF);
				++uwListPos;
				ubWasLimitY = 1;
			}
			copSetWait(
				(tCopWaitCmd*)&pBackBfr->pList[uwListPos],
				pBlock->uWaitPos.uwX, pBlock->uWaitPos.uwY & 0xFF
			);
			++uwListPos;
		}
		else {
			copSetWait(
				(tCopWaitCmd*)&pBackBfr->pList[uwListPos],
				pBlock->uWaitPos.uwX, pBlock->uWaitPos.uwY
			);
			++uwListPos;
		}

		// Copy MOVEs
		CopyMem(pBlock->pCmds, &pBackBfr->pList[uwListPos], pBlock->uwCurrCount*sizeof(tCopCmd));
		uwListPos += pBlock->uwCurrCount;
	}

	// Add 0xFFFF terminator
	copSetWait((tCopWaitCmd*)&pBackBfr->pList[uwListPos], 0xFF, 0xFF);
	++uwListPos;

	pCopList->pBackBfr->uwCmdCount = uwListPos;

	if(pCopList->ubStatus & STATUS_UPDATE_CURR) {
		return STATUS_UPDATE_PREV;
	}
	return 0;
}

void copProcessBlocks(void) {
	tCopList *pCopList = g_sCopManager.pCopList;
	if(pCopList->ubMode == COPPER_MODE_BLOCK) {
		UBYTE ubNewStatus = 0;
		// Realloc buffer memeory
		if(pCopList->ubStatus & STATUS_REALLOC) {
			ubNewStatus = copBfrRealloc();
		}

		// Sort blocks if needed
		if(pCopList->ubStatus & STATUS_REORDER) {
			copReorderBlocks();
		}

		// Update buffer data
		if(pCopList->ubStatus & STATUS_UPDATE) {
			ubNewStatus |= copUpdateFromBlocks();
		}
		// Update status code
		pCopList->ubStatus = ubNewStatus;
	}

	// Swap copper buffers
	copSwapBuffers();
}

void copBlockWait(tCopList *pCopList, tCopBlock *pBlock, UWORD uwX, UWORD uwY) {
	pBlock->uWaitPos.uwY  = uwY;
	pBlock->uWaitPos.uwX  = uwX;

	pBlock->ubUpdated = 2;
	pCopList->ubStatus |= STATUS_UPDATE | STATUS_REORDER;
}

void copMove(tCopList *pCopList, tCopBlock *pBlock, volatile void *pAddr, UWORD uwValue) {
	copSetMove((tCopMoveCmd*)&pBlock->pCmds[pBlock->uwCurrCount], pAddr, uwValue);
	++pBlock->uwCurrCount;

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

void copSetMove(tCopMoveCmd *pMoveCmd, volatile void *pAddr, UWORD uwValue) {
	pMoveCmd->bfUnused = 0;
	pMoveCmd->bfDestAddr = (ULONG)pAddr - (ULONG)((UBYTE *)g_pCustom);
	pMoveCmd->bfValue = uwValue;
}

static UWORD CHIP s_pBlankSprite[2];

tCopBlock *copBlockDisableSprites(tCopList *pList, FUBYTE fubSpriteMask) {
	FUBYTE fubCmdCnt = 0;
	FUBYTE fubMask = fubSpriteMask;

	// Determine instruction count
	for(FUBYTE i = 0; i != 8; ++i) {
		if(fubMask & 1) {
			fubCmdCnt += 2;
		}
		fubMask >>= 1;
	}

	// Set instructions
	ULONG ulBlank = (ULONG)s_pBlankSprite;
	tCopBlock *pBlock = copBlockCreate(pList, fubCmdCnt, 0, 0);
	fubMask = fubSpriteMask;
	for(FUBYTE i = 0; i != 8; ++i) {
		if(fubMask & 1) {
			copMove(pList, pBlock, &g_pSprFetch[i].uwHi, ulBlank >> 16);
			copMove(pList, pBlock, &g_pSprFetch[i].uwLo, ulBlank & 0xFFFF);
		}
		fubMask >>= 1;
	}
	return pBlock;
}

FUBYTE copRawDisableSprites(tCopList *pList, FUBYTE fubSpriteMask, FUWORD fuwCmdOffs) {
	FUBYTE fubCmdCnt = 0;
	ULONG ulBlank = (ULONG)s_pBlankSprite;

	// No WAIT - could be done earlier by other stuff
	tCopMoveCmd *pCmd = &pList->pBackBfr->pList[fuwCmdOffs].sMove;
	for(FUBYTE i = 0; i != 8; ++i) {
		if(fubSpriteMask & 1) {
			copSetMove(pCmd++, &g_pSprFetch[i].uwHi, ulBlank >> 16);
			copSetMove(pCmd++, &g_pSprFetch[i].uwLo, ulBlank & 0xFFFF);
			fubCmdCnt += 2;
		}
		fubSpriteMask >>= 1;
	}

	// Copy to front buffer
	CopyMemQuick(
		&pList->pBackBfr->pList[fuwCmdOffs],
		&pList->pFrontBfr->pList[fuwCmdOffs],
		fubCmdCnt * sizeof(tCopCmd)
	);

	return fubCmdCnt;
}

#endif // AMIGA
