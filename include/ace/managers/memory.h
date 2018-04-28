/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GUARD_ACE_MANAGER_MEMORY_H
#define GUARD_ACE_MANAGER_MEMORY_H

#ifdef AMIGA
#include <clib/exec_protos.h> // Amiga typedefs
#include <exec/memory.h>      // MEMF_CLEAR etc
#else
#define MEMF_CHIP    0
#define MEMF_FAST    1
#define MEMF_CLEAR   2
#define MEMF_PUBLIC  4
#define MEMF_LARGEST 8
#endif // AMIGA

#include <ace/types.h>

/* Types */

typedef struct _tMemEntry {
	void *pAddr;
	ULONG ulSize;
	UWORD uwId;
	struct _tMemEntry *pNext;
} tMemEntry;

/* Globals */

/* Functions */

void _memCreate(void);
void _memEntryAdd(void *pAddr, ULONG ulSize, UWORD uwLine, char *szFile);
void _memEntryDelete(void *pAddr, ULONG ulSize, UWORD uwLine, char *szFile);
void _memDestroy(void);

void *_memAllocDbg(ULONG ulSize, ULONG ulFlags, UWORD uwLine, char *szFile);
void _memFreeDbg(void *pMem, ULONG ulSize, UWORD uwLine, char *szFile);
void *_memAllocRls(ULONG ulSize, ULONG ulFlags);
void _memFreeRls(void *pMem, ULONG ulSize);

/**
 * Macros for enabling or disabling logging
 */

#ifdef GAME_DEBUG
# define memAlloc(ulSize, ulFlags) _memAllocDbg(ulSize, ulFlags, __LINE__, __FILE__)
# define memFree(pMem, ulSize) _memFreeDbg(pMem, ulSize, __LINE__, __FILE__)
# define memCreate() _memCreate()
# define memDestroy() _memDestroy()
# define memEntryAdd(pAddr, ulSize) _memEntryAdd(pAddr, ulSize, __LINE__, __FILE__)
# define memEntryDelete(pAddr, ulSize) _memEntryDelete(pAddr, ulSize, __LINE__, __FILE__)
#else
# define memAlloc(ulSize, ulFlags) _memAllocRls(ulSize, ulFlags)
# define memFree(pMem, ulSize) _memFreeRls(pMem, ulSize)
# define memCreate()
# define memDestroy()
# define memEntryAdd(pAddr, ulSize)
# define memEntryDelete(pAddr, ulSize)
#endif // GAME_DEBUG

// Shorthands
#define memAllocFast(ulSize) memAlloc(ulSize, 0)
#define memAllocChip(ulSize) memAlloc(ulSize, MEMF_CHIP)
#define memAllocFastClear(ulSize) memAlloc(ulSize, MEMF_CLEAR)
#define memAllocChipClear(ulSize) memAlloc(ulSize, MEMF_CHIP | MEMF_CLEAR)
#define memAllocChipFlags(ulSize, ulFlags) memAlloc(ulSize, MEMF_CHIP | ulFlags)
#define memAllocFastFlags(ulSize, ulFlags) memAlloc(ulSize, ulFlags)

#endif
