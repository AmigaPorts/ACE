#include <ace/managers/memory.h>
#include <ace/utils/file.h>

/* Globals */

#ifndef AMIGA
#include <stdlib.h>
int TypeOfMem(void *pMem) {return MEMF_FAST;};
#endif // AMIGA

static UWORD s_uwLastId = 0;

/* Vars */
tMemEntry *s_pMemTail;
tFile *s_pMemLog;
ULONG s_ulChipUsage, s_ulChipPeakUsage, s_ulFastUsage, s_ulFastPeakUsage;

/* Functions */

/**
 * Memory manager functions
 * mainly used for debug, should be replaced by NOP on release builds
 */
void _memCreate(void) {
	s_pMemTail = 0;
	s_ulChipUsage = 0;
	s_ulChipPeakUsage = 0;
	s_ulFastUsage = 0;
	s_ulFastPeakUsage = 0;
	s_uwLastId = 0;
	s_pMemLog = fileOpen("memory.log", "w");
}

void _memEntryAdd(void *pAddr, ULONG ulSize, UWORD uwLine, char *szFile) {
	tMemEntry *pNext;
	// Add mem usage entry
	pNext = s_pMemTail;
	s_pMemTail = _memAllocRls(sizeof(tMemEntry), MEMF_CLEAR);
	s_pMemTail->pAddr = pAddr;
	s_pMemTail->ulSize = ulSize;
	s_pMemTail->uwId = s_uwLastId++;
	s_pMemTail->pNext = pNext;

	if(TypeOfMem(pAddr) & MEMF_CHIP)
		filePrintf(
			s_pMemLog, "Allocated CHIP memory %hu@%p, size %lu (%s:%u)\n",
			s_pMemTail->uwId, pAddr, ulSize, szFile, uwLine
		);
	else
		filePrintf(
			s_pMemLog, "Allocated FAST memory %hu@%p, size %lu (%s:%u)\n",
			s_pMemTail->uwId, pAddr, ulSize, szFile, uwLine
		);

	// Update mem usage counter
	if(TypeOfMem(pAddr) & MEMF_CHIP) {
		s_ulChipUsage += ulSize;
		if(s_ulChipUsage > s_ulChipPeakUsage)
			s_ulChipPeakUsage = s_ulChipUsage;
	}
	else {
		s_ulFastUsage += ulSize;
		if(s_ulFastUsage > s_ulFastPeakUsage)
			s_ulFastPeakUsage = s_ulFastUsage;
	}
	// filePrintf(s_pMemLog, "usage: CHIP: %lu, FAST: %lu\n", s_ulChipUsage, s_ulFastUsage);

	fflush(s_pMemLog);
}

void _memEntryDelete(void *pAddr, ULONG ulSize, UWORD uwLine, char *szFile) {
	tMemEntry *pPrev = 0;
	tMemEntry *pCurr = s_pMemTail;

	// find memory entry
	while(pCurr && pCurr->pAddr != pAddr) {
		pPrev = pCurr;
		pCurr = pCurr->pNext;
	}
	if(pCurr->pAddr != pAddr) {
		filePrintf(
			s_pMemLog, "ERR: can't find memory allocated at %p (%s:%u)\n",
			pAddr, szFile, uwLine
		);
		fflush(s_pMemLog);
		return;
	}

	// unlink entry from list
	if(pPrev)
		pPrev->pNext = pCurr->pNext;
	else
		s_pMemTail = pCurr->pNext;

	// remove entry
	if(ulSize != pCurr->ulSize) {
		filePrintf(
			s_pMemLog, "WARNING: memFree size mismatch at memory %hu@%p: %lu, should be %lu (%s:%u)\n",
			pCurr->uwId, pAddr, ulSize, pCurr->ulSize, szFile, uwLine
		);
	}
	filePrintf(
		s_pMemLog, "freed memory %hu@%p, size %lu (%s:%u)\n",
		pCurr->uwId, pAddr, ulSize, szFile, uwLine
	);

	// Update mem usage counter
	if(TypeOfMem(pAddr) & MEMF_CHIP)
		s_ulChipUsage -= ulSize;
	else
		s_ulFastUsage -= ulSize;
	// filePrintf(s_pMemLog, "Usage: CHIP: %lu, FAST: %lu\n", s_ulChipUsage, s_ulFastUsage);

	fflush(s_pMemLog);
}

void _memDestroy(void) {
	filePrintf(
		s_pMemLog, "\n=============== MEMORY MANAGER DESTROY ==============\n"
	);
	filePrintf(
		s_pMemLog, "If something is deallocated past here, you're a wuss!\n"
	);
	while(s_pMemTail) {
		_memEntryDelete(s_pMemTail->pAddr, s_pMemTail->ulSize, 0, "memoryDestroy");
	}
	filePrintf(
		s_pMemLog, "Peak usage: CHIP: %lu, FAST: %lu\n",
		s_ulChipPeakUsage, s_ulFastPeakUsage
	);
	fileClose(s_pMemLog);
	s_pMemLog = 0;
}

void *_memAllocDbg(ULONG ulSize, ULONG ulFlags, UWORD uwLine, char *szFile) {
	void *pAddr;
	pAddr = _memAllocRls(ulSize, ulFlags);
	if(!pAddr) {
		filePrintf(
			s_pMemLog, "ERR: couldn't allocate %lu bytes! (%s:%u)\n",
			ulSize, szFile, uwLine
		);
		filePrintf(
			s_pMemLog, "Peak usage: CHIP: %lu, FAST: %lu\n",
			s_ulChipPeakUsage, s_ulFastPeakUsage
		);
#ifdef AMIGA
		filePrintf(
			s_pMemLog, "Largest available chunk of given type: %lu\n",
			AvailMem(ulFlags | MEMF_LARGEST)
		);
#endif // AMIGA
		return 0;
	}
	_memEntryAdd(pAddr, ulSize, uwLine, szFile);
	return pAddr;
}

void _memFreeDbg(void *pMem, ULONG ulSize, UWORD uwLine, char *szFile) {
	_memEntryDelete(pMem, ulSize, uwLine, szFile);
	_memFreeRls(pMem, ulSize);
}

void *_memAllocRls(ULONG ulSize, ULONG ulFlags) {
	#ifdef AMIGA
	return AllocMem(ulSize, ulFlags);
	#else
	return malloc(ulSize);
	#endif // AMIGA
}

void _memFreeRls(void *pMem, ULONG ulSize) {
	#ifdef AMIGA
	FreeMem(pMem, ulSize);
	#else
	free(pMem);
	#endif // AMIGA
}
