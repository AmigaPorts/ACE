/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/file.h>
#include <stdarg.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>

LONG fileGetSize(const char *szPath) {
	// One could use std library to seek to end of file and use ftell,
	// but SEEK_END is not guaranteed to work.
	// http://www.cplusplus.com/reference/cstdio/fseek/
	// On the other hand, Lock/UnLock is bugged on KS1.3 and doesn't allow
	// for doing Open() on same file after using it.
	// So I ultimately do it using fseek.

	systemUse();
	systemReleaseBlitterToOs();
	logBlockBegin("fileGetSize(szPath: '%s')", szPath);
	FILE *pFile = fopen(szPath, "r");
	if(!pFile) {
		logWrite("ERR: File doesn't exist");
		logBlockEnd("fileGetSize()");
		systemGetBlitterFromOs();
		systemUnuse();
		return -1;
	}
	fseek(pFile, 0, SEEK_END);
	LONG lSize = ftell(pFile);
	fclose(pFile);

	logBlockEnd("fileGetSize()");
	systemGetBlitterFromOs();
	systemUnuse();

	return lSize;
}

void fileClose(tFile *pFile) {
	pFile->pCallbacks->cbFileClose(pFile->pData);
	memFree(pFile, sizeof(*pFile));
}

ULONG fileRead(tFile *pFile, void *pDest, ULONG ulSize) {
#ifdef ACE_DEBUG
	if(!ulSize) {
		logWrite("ERR: File read size = 0!\n");
	}
#endif
	return pFile->pCallbacks->cbFileRead(pFile->pData, pDest, ulSize);
}

ULONG fileWrite(tFile *pFile, const void *pSrc, ULONG ulSize) {
	return pFile->pCallbacks->cbFileWrite(pFile->pData, pSrc, ulSize);
}

ULONG fileSeek(tFile *pFile, LONG lPos, WORD wMode) {
	return pFile->pCallbacks->cbFileSeek(pFile->pData, lPos, wMode);
}

ULONG fileGetPos(tFile *pFile) {
	return pFile->pCallbacks->cbFileGetPos(pFile->pData);
}

UBYTE fileIsEof(tFile *pFile) {
	return pFile->pCallbacks->cbFileIsEof(pFile->pData);
}

void fileFlush(tFile *pFile) {
	pFile->pCallbacks->cbFileFlush(pFile->pData);
}

void fileWriteStr(tFile *pFile, const char *szLine) {
	fileWrite(pFile, szLine, strlen(szLine));
}
