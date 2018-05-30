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
	if(!pDir->pLock || Examine(pDir->pLock, &pDir->sFileBlock) == DOSFALSE) {
		UnLock(pDir->pLock);
		memFree(pDir, sizeof(tDir));
		systemUnuse();
		return 0;
	}
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
	systemUnuse();
	return 1;
}

void dirClose(tDir *pDir) {
	systemUse();
	UnLock(pDir->pLock);
	memFree(pDir, sizeof(tDir));
	systemUnuse();
}

UBYTE dirExists(const char *szPath) {
	systemUse();
	tDir *pDir = dirOpen(szPath);
	if(pDir) {
		dirClose(pDir);
	}
	systemUnuse();
	return pDir != 0;
}

UBYTE dirCreate(const char *szName) {
	systemUse();
	LONG lResult = CreateDir((STRPTR)szName);
	if(!lResult) {
		systemUnuse();
		return 0;
	}
	UnLock(lResult);
	systemUnuse();
	return 1;
}

UBYTE dirCreatePath(const char *szPath) {
	systemUse();
	BYTE bPrevSlash = -1;
	char szSubPath[108];
	char *pSlash;
	UBYTE isCreated = 1;
	while((pSlash = strchr(&szPath[bPrevSlash+1], '/'))) {
		memcpy(&szSubPath[bPrevSlash], &szPath[bPrevSlash], pSlash-szPath - bPrevSlash);
		szSubPath[pSlash-szPath] = '\0';
		if(dirExists(szSubPath)) {
			// This level exists - skip
			isCreated = 1;
		}
		else if(!dirCreate(szSubPath))  {
			systemUnuse();
			return 0;
		}
		bPrevSlash = pSlash-szPath;
	}
	if(szPath[strlen(szPath)-1] != '/' && !dirExists(szPath)) {
		// Path doesn't end with directory - create once again
		isCreated = dirCreate(szPath);
	}
	systemUnuse();
	return isCreated;
}
