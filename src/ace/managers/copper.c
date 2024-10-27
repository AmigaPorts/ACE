/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/copper.h>
#ifdef AMIGA
#include <stdarg.h>
#include <ace/managers/system.h>
#include <limits.h>
#include <proto/exec.h>

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
#ifdef ACE_DEBUG
		const char* szCmd = pCmd->sWait.bfIsSkip ? "SKIP" : "WAIT";
		logWrite(
			"@%p: %08lX - %s: %hu,%hu%s\n",
			pCmd, pCmd->ulCode, szCmd, pCmd->sWait.bfWaitX << 1, pCmd->sWait.bfWaitY,
			!pCmd->sWait.bfBlitterIgnore ? " & blit done" : ""
		);
#endif
	}
	else {
#ifdef ACE_DEBUG
		static const char* pCustomRegNames[256] = {
			"BLTDDAT", "DMACONR", "VPOSR", "VHPOSR", "DSKDATR", "JOY0DAT",
			"JOY1DAT", "CLXDAT", "ADKCONR", "POT0DAT", "POT1DAT", "POTGOR",
			"SERDATR", "DSKBYTR", "INTENAR", "INTREQR", "DSKPTH", "DSKPTL",
			"DSKLEN", "DSKDAT", "REFPTR", "VPOSW", "VHPOSW", "COPCON",
			"SERDAT", "SERPER", "POTGO", "JOYTEST", "STREQU", "STRVBL",
			"STRHOR", "STRLONG", "BLTCON0", "BLTCON1", "BLTAFWM", "BLTALWM",
			"BLTCPTH", "BLTCPTL", "BLTBPTH", "BLTBPTL", "BLTAPTH", "BLTAPTL",
			"BLTDPTH", "BLTDPTL", "BLTSIZE", "BLTCON0L", "BLTSIZV", "BLTSIZH",
			"BLTCMOD", "BLTBMOD", "BLTAMOD", "BLTDMOD", NULL, NULL,
			NULL, NULL, "BLTCDAT", "BLTBDAT", "BLTADAT", NULL,
			"SPRHDAT", NULL, "DENISEID", "DSKSYNC", "COP1LCH", "COP1LCL",
			"COP2LCH", "COP2LCL", "COPJMP1", "COPJMP2", "COPINS", "DIWSTRT",
			"DIWSTOP", "DDFSTRT", "DDFSTOP", "DMACON", "CLXCON", "INTENA",
			"INTREQ", "ADKCON", "AUD0LCH", "AUD0LCL", "AUD0LEN", "AUD0PER",
			"AUD0VOL", "AUD0DAT", NULL, NULL, "AUD1LCH", "AUD1LCL",
			"AUD1LEN", "AUD1PER", "AUD1VOL", "AUD1DAT", NULL, NULL,
			"AUD2LCH", "AUD2LCL", "AUD2LEN", "AUD2PER", "AUD2VOL", "AUD2DAT",
			NULL, NULL, "AUD3LCH", "AUD3LCL", "AUD3LEN", "AUD3PER",
			"AUD3VOL", "AUD3DAT", NULL, NULL, "BPL1PTH", "BPL1PTL",
			"BPL2PTH", "BPL2PTL", "BPL3PTH", "BPL3PTL", "BPL4PTH", "BPL4PTL",
			"BPL5PTH", "BPL5PTL", "BPL6PTH", "BPL6PTL",
			NULL, NULL, NULL, NULL, "BPLCON0", "BPLCON1",
			"BPLCON2", "BPLCON3", "BPL1MOD", "BPL2MOD", NULL, NULL,
			"BPL1DAT", "BPL2DAT", "BPL3DAT", "BPL4DAT", "BPL5DAT", "BPL6DAT",
			NULL, NULL, "SPR0PTH", "SPR0PTL", "SPR1PTH", "SPR1PTL", "SPR2PTH",
			"SPR2PTL", "SPR3PTH", "SPR3PTL", "SPR4PTH", "SPR4PTL", "SPR5PTH",
			"SPR5PTL", "SPR6PTH", "SPR6PTL", "SPR7PTH", "SPR7PTL", "SPR0POS",
			"SPR0CTL", "SPR0DATA", "SPR0DATB", "SPR1POS", "SPR1CTL", "SPR1DATA",
			"SPR1DATB", "SPR2POS", "SPR2CTL", "SPR2DATA", "SPR2DATB", "SPR3POS",
			"SPR3CTL", "SPR3DATA", "SPR3DATB", "SPR4POS", "SPR4CTL", "SPR4DATA",
			"SPR4DATB", "SPR5POS", "SPR5CTL", "SPR5DATA", "SPR5DATB", "SPR6POS",
			"SPR6CTL", "SPR6DATA", "SPR6DATB", "SPR7POS", "SPR7CTL", "SPR7DATA",
			"SPR7DATB", "COLOR00", "COLOR01", "COLOR02", "COLOR03", "COLOR04",
			"COLOR05", "COLOR06", "COLOR07", "COLOR08", "COLOR09", "COLOR10",
			"COLOR11", "COLOR12", "COLOR13", "COLOR14", "COLOR15", "COLOR16",
			"COLOR17", "COLOR18", "COLOR19", "COLOR20", "COLOR21", "COLOR22",
			"COLOR23", "COLOR24", "COLOR25", "COLOR26", "COLOR27", "COLOR28",
			"COLOR29", "COLOR30", "COLOR31", "HTOTAL", "HSSTOP", "HBSTRT",
			"HBSTOP", "VTOTAL", "VSSTOP", "VBSTRT", "VBSTOP", NULL,
			NULL, NULL, NULL, NULL, NULL, "BEAMCON0",
			"HSSTRT", "VSSTRT", "HCENTER", "DIWHIGH", NULL
		};

		const char* szRegName = pCustomRegNames[pCmd->sMove.bfDestAddr >> 1];
		if (szRegName) {
			logWrite(
				"@%p: %08lX - MOVE: %8s := %04X\n",
				pCmd, pCmd->ulCode, szRegName, pCmd->sMove.bfValue
			);
		}
		else {
			logWrite(
				"@%p: %08lX - MOVE:      %03X := %04X\n",
				pCmd, pCmd->ulCode, pCmd->sMove.bfDestAddr, pCmd->sMove.bfValue
			);
		}
#endif
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
				memcpy(
					&pBackBfr->pList[uwListPos], pBlock->pCmds,
					pBlock->uwCurrCount * sizeof(tCopCmd)
				);
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
		if(pBlock->uwCurrCount == 0) {
			continue;
		}

		// Update WAIT
		if(pBlock->uWaitPos.uwY >= 0xFF) {
			// FIXME: double WAIT only when previous line ended before some pos
			if(!ubWasLimitY) {
				copSetWait((tCopWaitCmd*)&pBackBfr->pList[uwListPos], 0xDF, 0xFF);
				++uwListPos;
				ubWasLimitY = 1;
			}
			if(pBlock->uWaitPos.uwY > 0xFF) {
				copSetWait(
					(tCopWaitCmd*)&pBackBfr->pList[uwListPos],
					pBlock->uWaitPos.uwX, pBlock->uWaitPos.uwY & 0xFF
				);
				++uwListPos;
			}
		}
		else {
			copSetWait(
				(tCopWaitCmd*)&pBackBfr->pList[uwListPos],
				pBlock->uWaitPos.uwX, pBlock->uWaitPos.uwY
			);
			++uwListPos;
		}

		// Copy MOVEs
		for(UWORD i = pBlock->uwCurrCount; i--;) {
			pBackBfr->pList[uwListPos + i].ulCode = pBlock->pCmds[i].ulCode;
		}
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
	copSetMoveVal(pMoveCmd, uwValue);
}

#endif // AMIGA
