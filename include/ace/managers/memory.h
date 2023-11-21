/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_MEMORY_H_
#define _ACE_MANAGERS_MEMORY_H_

/**
 * Memory manager functions.
 * mainly used for debug, should be replaced by NOP on release builds.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>
#include <ace/macros.h>

#if defined(AMIGA)
#include <exec/memory.h> // MEMF_CLEAR etc
#else
#define MEMF_ANY 0
#define MEMF_PUBLIC  BV(0)
#define MEMF_CHIP    BV(1)
#define MEMF_FAST    BV(2)
#define MEMF_CLEAR   BV(16)
#define MEMF_LARGEST BV(17)
#endif // AMIGA

/* Types */

/* Globals */

/* Functions */

/**
 * @brief Checks whether memory pointer is in CHIP or FAST mem.
 *
 * @param pMem Pointer to memory to be checked.
 * @return 1 if memory is of CHIP type, otherwise 0.
 */
UBYTE memIsChip(const void *pMem);

ULONG memGetChipSize(void);

ULONG memAvail(ULONG ulFlags);

void _memCreate(void);
void _memDestroy(void);

void *_memAllocDbg(ULONG ulSize, ULONG ulFlags, UWORD uwLine, const char *szFile);
void _memFreeDbg(void *pMem, ULONG ulSize, UWORD uwLine, const char *szFile);
void *_memAllocRls(ULONG ulSize, ULONG ulFlags) __attribute__((malloc));
void _memFreeRls(void *pMem, ULONG ulSize);

void _memCheckTrashAtAddr(void *pMem, UWORD uwLine, const char *szFile);

void _memCheckIntegrity(UWORD uwLine, const char *szFile);

/**
 * Macros for enabling or disabling logging
 */

#ifdef ACE_DEBUG
# define memAlloc(ulSize, ulFlags) _memAllocDbg(ulSize, ulFlags, __LINE__, __FILE__)
# define memFree(pMem, ulSize) _memFreeDbg(pMem, ulSize, __LINE__, __FILE__)
# define memCreate() _memCreate()
# define memDestroy() _memDestroy()
# define memCheckTrashAtAddr(pAddr) _memCheckTrashAtAddr(pAddr, __LINE__, __FILE__)
# define memCheckIntegrity() _memCheckIntegrity(__LINE__, __FILE__)
#else
# define memAlloc(ulSize, ulFlags) _memAllocRls(ulSize, ulFlags)
# define memFree(pMem, ulSize) _memFreeRls(pMem, ulSize)
# define memCreate()
# define memDestroy()
# define memCheckTrashAtAddr(pAddr, ulSize)
# define memCheckIntegrity()
#endif // ACE_DEBUG

// Shorthands
#define memAllocFast(ulSize) memAlloc(ulSize, MEMF_ANY)
#define memAllocChip(ulSize) memAlloc(ulSize, MEMF_CHIP)
#define memAllocFastClear(ulSize) memAlloc(ulSize, MEMF_ANY | MEMF_CLEAR)
#define memAllocChipClear(ulSize) memAlloc(ulSize, MEMF_CHIP | MEMF_CLEAR)
#define memAllocChipFlags(ulSize, ulFlags) memAlloc(ulSize, MEMF_CHIP | ulFlags)
#define memAllocFastFlags(ulSize, ulFlags) memAlloc(ulSize, MEMF_ANY |ulFlags)

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_MEMORY_H_
