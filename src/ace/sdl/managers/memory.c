/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>
#include <ace/types.h>

void *_memAllocRls(ULONG ulSize, UNUSED_ARG ULONG ulFlags) {
	void *pResult = malloc(ulSize);
	return pResult;
}

void _memFreeRls(void *pMem, UNUSED_ARG ULONG ulSize) {
	free(pMem);
}

UBYTE memIsChip(UNUSED_ARG const void *pMem) {
	return 0;
}

ULONG memGetChipSize(void) {
	return 2 * 1024 * 1024;
}
