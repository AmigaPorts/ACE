/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <proto/exec.h> // Bartman's compiler needs this
#include <proto/dos.h> // Bartman's compiler needs this
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>

#ifdef AMIGA
#include <clib/exec_protos.h> // AvailMem, AllocMem, FreeMem, etc.
#endif

//------------------------------------------------------------------------ TYPES

typedef struct _tMemEntry {
	void *pAddr;
	ULONG ulSize;
	UWORD uwId;
	struct _tMemEntry *pNext;
} tMemEntry;

//----------------------------------------------------------------- PRIVATE VARS

static UWORD s_uwLastId = 0;
static tMemEntry *s_pMemTail;
static ULONG s_ulChipUsage, s_ulChipPeakUsage, s_ulFastUsage, s_ulFastPeakUsage;

//---------------------------------------------------------------- MEM ENTRY FNS

static void _memEntryAdd(
	void *pAddr, ULONG ulSize, UWORD uwLine, const char *szFile
) {
	// Ozzyboshi discovered that memory allocation works without re-enabling
	// the OS. After analyzing what's under the hood, it looks like there are only
	// Forbid/Permit calls inside OS mem fns and the rest is just plain code.
	// Still, OS mem fns can be patched by Snoopdos and/or newer OS versions,
	// so I guess the safest approach is not to assume anything and wake OS up
	// in case it needs other components on some exotic configs.

	systemUse();
	tMemEntry *pNext;
	// Add mem usage entry
	pNext = s_pMemTail;
	s_pMemTail = _memAllocRls(sizeof(tMemEntry), MEMF_CLEAR);
	s_pMemTail->pAddr = pAddr;
	s_pMemTail->ulSize = ulSize;
	s_pMemTail->uwId = s_uwLastId++;
	s_pMemTail->pNext = pNext;

	logWrite(
		"[MEM] Allocated %s memory %hu@%p, size %lu (%s:%u)\n",
		(memType(pAddr) & MEMF_CHIP) ? "CHIP" : "FAST",
		s_pMemTail->uwId, pAddr, ulSize, szFile, uwLine
	);

	// Update mem usage counter
	if(memType(pAddr) & MEMF_CHIP) {
		s_ulChipUsage += ulSize;
		if(s_ulChipUsage > s_ulChipPeakUsage) {
			s_ulChipPeakUsage = s_ulChipUsage;
		}
	}
	else {
		s_ulFastUsage += ulSize;
		if(s_ulFastUsage > s_ulFastPeakUsage) {
			s_ulFastPeakUsage = s_ulFastUsage;
		}
	}
	systemUnuse();
}

static ULONG _memEntryDelete(
	void *pAddr, ULONG ulSize, UWORD uwLine, const char *szFile
) {
	tMemEntry *pPrev = 0;
	tMemEntry *pCurr = s_pMemTail;

	// find memory entry
	while(pCurr && pCurr->pAddr != pAddr) {
		pPrev = pCurr;
		pCurr = pCurr->pNext;
	}
	if(!pCurr) {
		logWrite(
			"[MEM] ERR: can't find memory allocated at %p (%s:%u)\n", pAddr, szFile, uwLine
		);
		return 0;
	}

	// unlink entry from list
	if(pPrev) {
		pPrev->pNext = pCurr->pNext;
	}
	else {
		s_pMemTail = pCurr->pNext;
	}

	// remove entry
	systemUse();
	if(ulSize != pCurr->ulSize) {
		logWrite(
			"[MEM] ERR: memFree size mismatch at memory %hu@%p: %lu, should be %lu (%s:%u)\n",
			pCurr->uwId, pAddr, ulSize, pCurr->ulSize, szFile, uwLine
		);
	}
	logWrite(
		"[MEM] Freed memory %hu@%p, size %lu (%s:%u)\n",
		pCurr->uwId, pAddr, ulSize, szFile, uwLine
	);

	// Update mem usage counter
	if(memType(pAddr) & MEMF_CHIP) {
		s_ulChipUsage -= ulSize;
	}
	else {
		s_ulFastUsage -= ulSize;
	}

	systemUnuse();
	return pCurr->ulSize;
}

static void memEntryCheckTrash(
	const tMemEntry *pEntry, UWORD uwLine, const char *szFile
) {
	UBYTE *pCafe = (UBYTE*)(pEntry->pAddr - 4*sizeof(UBYTE));
	UBYTE *pDead = (UBYTE*)(pEntry->pAddr + pEntry->ulSize);
	if(pCafe[0] != 0xCA || pCafe[1] != 0xFE || pCafe[2] != 0xBA || pCafe[3] != 0xBE) {
		logWrite(
			"[MEM] ERR: Left mem trashed: %hu@%p (%s:%u)\n",
			pEntry->uwId, pEntry->pAddr, szFile, uwLine
		);
	}
	if(pDead[0] != 0xDE || pDead[1] != 0xAD || pDead[2] != 0xBE || pDead[3] != 0xEF) {
		logWrite(
			"[MEM] ERR: Right mem trashed: %hu@%p (%s:%u)\n",
			pEntry->uwId, pEntry->pAddr, szFile, uwLine
		);
	}
}

//---------------------------------------------------------------------- MEM FNS

void _memCheckIntegrity(UWORD uwLine, const char *szFile) {
	tMemEntry *pEntry = s_pMemTail;
	while(pEntry) {
		memEntryCheckTrash(pEntry, uwLine, szFile);
		pEntry = pEntry->pNext;
	}

	systemCheckStack();
}

void _memCreate(void) {
	s_pMemTail = 0;
	s_ulChipUsage = 0;
	s_ulChipPeakUsage = 0;
	s_ulFastUsage = 0;
	s_ulFastPeakUsage = 0;
	s_uwLastId = 0;
}

void _memDestroy(void) {
	systemUse();
	logWrite("\n=============== MEMORY MANAGER DESTROY ==============\n");
	logWrite("If something is deallocated past here, you're a wuss!\n");
	while(s_pMemTail) {
		_memFreeDbg(s_pMemTail->pAddr, s_pMemTail->ulSize, 0, "memoryDestroy");
	}
	logWrite(
		"[MEM] Peak usage: CHIP: %lu, FAST: %lu\n",
		s_ulChipPeakUsage, s_ulFastPeakUsage
	);
	systemUnuse();
}

void *_memAllocDbg(
	ULONG ulSize, ULONG ulFlags, UWORD uwLine, const char *szFile
) {
	if(!ulSize) {
		logWrite("[MEM] ERR: zero alloc size! (%s:%u)\n", szFile, uwLine);
		return 0;
	}
	void *pAddr;
	pAddr = _memAllocRls(ulSize + 2 * sizeof(ULONG), ulFlags);
	if(!pAddr) {
		logWrite(
			"[MEM] ERR: couldn't allocate %lu bytes! (%s:%u)\n",
			ulSize, szFile, uwLine
		);
		memLogPeak();
#ifdef AMIGA
		logWrite(
			"[MEM] Largest available chunk of given type: %lu\n",
			AvailMem(ulFlags | MEMF_LARGEST)
		);
#endif // AMIGA
		return 0;
	}
	pAddr += sizeof(ULONG);
	UBYTE *pCafe = (UBYTE*)(pAddr - 4*sizeof(UBYTE));
	UBYTE *pDead = (UBYTE*)(pAddr + ulSize);
	pCafe[0] = 0xCA; pCafe[1] = 0xFE; pCafe[2] = 0xBA; pCafe[3] = 0xBE;
	pDead[0] = 0xDE; pDead[1] = 0xAD; pDead[2] = 0xBE; pDead[3] = 0xEF;
	_memEntryAdd(pAddr, ulSize, uwLine, szFile);
	return pAddr;
}

void _memFreeDbg(
	void *pMem, ULONG ulSize, UWORD uwLine, const char *szFile
) {
	systemUse();
	_memCheckIntegrity(uwLine, szFile);
	ulSize = _memEntryDelete(pMem, ulSize, uwLine, szFile);
	if(ulSize) {
		_memFreeRls(pMem - sizeof(ULONG), ulSize + 2 * sizeof(ULONG));
	}
	systemUnuse();
}

void *_memAllocRls(ULONG ulSize, ULONG ulFlags) {
	systemUse();
	void *pResult;
	#ifdef AMIGA
	pResult = AllocMem(ulSize, ulFlags);
	if(!(ulFlags & MEMF_CHIP) && !pResult) {
		// No FAST available - allocate CHIP instead
		logWrite("[MEM] WARN: Couldn't allocate FAST mem\r\n");
		pResult = AllocMem(ulSize, (ulFlags & ~MEMF_FAST) | MEMF_ANY);
	}
	#else
	pResult =  malloc(ulSize);
	#endif // AMIGA
	systemUnuse();
	return pResult;
}

void _memFreeRls(void *pMem, ULONG ulSize) {
	systemUse();
	#ifdef AMIGA
	FreeMem(pMem, ulSize);
	#else
	free(pMem);
	#endif // AMIGA
	systemUnuse();
}

void _memCheckTrashAtAddr(void *pMem, UWORD uwLine, const char *szFile) {
	// find memory entry
	tMemEntry *pEntry = s_pMemTail;
	while(pEntry && pEntry->pAddr != pMem) {
		pEntry = pEntry->pNext;
	}
	if(pEntry->pAddr != pMem) {
		logWrite(
			"[MEM] ERR: can't find memory allocated at %p (%s:%u)\n",
			pMem, szFile, uwLine
		);
		return;
	}
	memEntryCheckTrash(pEntry, uwLine, szFile);
}

void _memLogPeak(void) {
	logWrite(
		"[MEM] Peak usage: CHIP: %lu, FAST: %lu\n",
		s_ulChipPeakUsage, s_ulFastPeakUsage
	);
}

UBYTE memType(const void *pMem) {
#ifdef AMIGA
	ULONG ulOsType = TypeOfMem((void *)pMem);
	if(ulOsType & MEMF_FAST) {
		return MEMF_FAST;
	}
	return MEMF_CHIP;
#else
	return MEMF_FAST;
#endif // AMIGA
}

ULONG memGetFreeChipSize(void) {
	return AvailMem(MEMF_CHIP);
}

ULONG memGetFreeSize(void) {
	return AvailMem(MEMF_ANY);
}
