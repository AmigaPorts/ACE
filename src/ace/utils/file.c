/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/file.h>
#include <stdarg.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
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
	logBlockBegin("fileGetSize(szPath: '%s')", szPath);
	FILE *pFile = fopen(szPath, "r");
	if(!pFile) {
		return -1;
	}
	fseek(pFile, 0, SEEK_END);
	LONG lSize = ftell(pFile);
	fclose(pFile);

	logBlockEnd("fileGetSize()");
	systemUnuse();
	return lSize;
}

tFile *fileOpen(const char *szPath, const char *szMode) {
	// TODO check if disk is read protected when szMode has 'a'/'r'/'x'
	systemUse();
	FILE *pFile = fopen(szPath, szMode);
	systemUnuse();
	return pFile;
}

void fileClose(tFile *pFile) {
	systemUse();
	fclose(pFile);
	systemUnuse();
}

ULONG fileRead(tFile *pFile, void *pDest, ULONG ulSize) {
#ifdef ACE_DEBUG
	if(!ulSize) {
		logWrite("ERR: File read size = 0!\n");
	}
#endif
	systemUse();
	ULONG ulResult = fread(pDest, ulSize, 1, pFile);
	systemUnuse();
	return ulResult;
}

ULONG fileWrite(tFile *pFile, const void *pSrc, ULONG ulSize) {
	systemUse();
	ULONG ulResult = fwrite(pSrc, ulSize, 1, pFile);
	fflush(pFile);
	systemUnuse();
	return ulResult;
}

ULONG fileSeek(tFile *pFile, ULONG ulPos, WORD wMode) {
	systemUse();
	ULONG ulResult = fseek(pFile, ulPos, wMode);
	systemUnuse();
	return ulResult;
}

ULONG fileGetPos(tFile *pFile) {
	systemUse();
	ULONG ulResult = ftell(pFile);
	systemUnuse();
	return ulResult;
}

UBYTE fileIsEof(tFile *pFile) {
	systemUse();
	UBYTE ubResult = feof(pFile);
	systemUnuse();
	return ubResult;
}

#if !defined(BARTMAN_GCC) // Not implemented in mini_std for now, sorry!
LONG fileVaPrintf(tFile *pFile, const char *szFmt, va_list vaArgs) {
	systemUse();
	LONG lResult = vfprintf(pFile, szFmt, vaArgs);
	fflush(pFile);
	systemUnuse();
	return lResult;
}

LONG filePrintf(tFile *pFile, const char *szFmt, ...) {
	va_list vaArgs;
	va_start(vaArgs, szFmt);
	LONG lResult = fileVaPrintf(pFile, szFmt, vaArgs);
	va_end(vaArgs);
	return lResult;
}

LONG fileVaScanf(tFile *pFile, const char *szFmt, va_list vaArgs) {
	systemUse();
	LONG lResult = vfscanf(pFile, szFmt, vaArgs);
	systemUnuse();
	return lResult;
}

LONG fileScanf(tFile *pFile, const char *szFmt, ...) {
	va_list vaArgs;
	va_start(vaArgs, szFmt);
	LONG lResult = fileVaScanf(pFile, szFmt, vaArgs);
	va_end(vaArgs);
	return lResult;
}
#endif

void fileFlush(tFile *pFile) {
	systemUse();
	fflush(pFile);
	systemUnuse();
}

void fileWriteStr(tFile *pFile, const char *szLine) {
	fileWrite(pFile, szLine, strlen(szLine));
}
