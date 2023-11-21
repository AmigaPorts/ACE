/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <proto/exec.h> // Bartman's compiler needs this
#include <proto/dos.h> // Bartman's compiler needs this
#include <clib/exec_protos.h> // AvailMem, AllocMem, FreeMem, etc.

ULONG memAvail(ULONG ulFlags) {
	return AvailMem(ulFlags);
}

void *_memAllocRls(ULONG ulSize, ULONG ulFlags) {
	systemUse();
	void *pResult;
	pResult = AllocMem(ulSize, ulFlags);
	if(!(ulFlags & MEMF_CHIP) && !pResult) {
		// No FAST available - allocate CHIP instead
		logWrite("[MEM] WARN: Couldn't allocate FAST mem\r\n");
		pResult = AllocMem(ulSize, (ulFlags & ~MEMF_FAST) | MEMF_ANY);
	}
	systemUnuse();
	return pResult;
}

void _memFreeRls(void *pMem, ULONG ulSize) {
	systemUse();
	FreeMem(pMem, ulSize);
	systemUnuse();
}

UBYTE memIsChip(const void *pMem) {
	ULONG ulOsType = TypeOfMem((void *)pMem);
	if(ulOsType & MEMF_FAST) {
		return 0;
	}
	return 1;
}

ULONG memGetChipSize(void) {
	return AvailMem(MEMF_CHIP);
}
