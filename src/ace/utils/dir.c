/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/dir.h>
#include <string.h>
#include <ace/macros.h>
#include <ace/managers/system.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>

tDir *dirOpen(const char *szPath) {
	systemUse();
	tDir *pDir = memAllocFast(sizeof(tDir));
	pDir->pLock = Lock((unsigned char*)szPath, ACCESS_READ);
	LONG lResult = Examine(pDir->pLock, &pDir->sFileBlock);
	if(lResult == DOSFALSE) {
		UnLock(pDir->pLock);
		memFree(pDir, sizeof(tDir));
		systemUnuse();
		return 0;
	}
	logWrite("dirOpenOk\n");
	systemUnuse();
	return pDir;
}

UBYTE dirRead(tDir *pDir, char *szFileName, UWORD uwFileNameMax) {
	systemUse();
	LONG lResult = ExNext(pDir->pLock, &pDir->sFileBlock);
	if(lResult == DOSFALSE) {
		systemUnuse();
		return 0;
	}

	strncpy(szFileName, pDir->sFileBlock.fib_FileName, uwFileNameMax-1);
	szFileName[uwFileNameMax-1] = '\0';
	logWrite("dirReadOk %s\n", szFileName);
	systemUnuse();
	return 1;
}

void dirClose(tDir *pDir) {
	systemUse();
	UnLock(pDir->pLock);
	memFree(pDir, sizeof(tDir));
	logWrite("dirCloseOk\n");
	systemUnuse();
}



