/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/file.h>
#include <stdarg.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>

// TODO: move to system to be able to use for dir functions?

static void fileAccessEnable(void) {
	systemUse();

	// Needed only for KS1.3
	// TODO: do only when reading from floppy
	if(systemGetVersion() < 36) {
		systemReleaseBlitterToOs();
	}
}

static void fileAccessDisable(void) {
	if(systemGetVersion() < 36) {
		systemGetBlitterFromOs();
	}

	systemUnuse();
}

LONG fileGetSize(const char *szPath) {
	// One could use std library to seek to end of file and use ftell,
	// but SEEK_END is not guaranteed to work.
	// http://www.cplusplus.com/reference/cstdio/fseek/
	// On the other hand, Lock/UnLock is bugged on KS1.3 and doesn't allow
	// for doing Open() on same file after using it.
	// So I ultimately do it using fseek.

	fileAccessEnable();
	logBlockBegin("fileGetSize(szPath: '%s')", szPath);
	FILE *pFile = fopen(szPath, "r");
	if(!pFile) {
		logWrite("ERR: File doesn't exist");
		logBlockEnd("fileGetSize()");
		fileAccessDisable();
		return -1;
	}
	fseek(pFile, 0, SEEK_END);
	LONG lSize = ftell(pFile);
	fclose(pFile);

	logBlockEnd("fileGetSize()");
	fileAccessDisable();

	return lSize;
}

tFile *fileOpen(const char *szPath, const char *szMode) {
	// TODO check if disk is read protected when szMode has 'a'/'r'/'x'
	fileAccessEnable();
	FILE *pFile = fopen(szPath, szMode);
	fileAccessDisable();

	return pFile;
}

void fileClose(tFile *pFile) {
	fileAccessEnable();
	fclose(pFile);
	fileAccessDisable();
}

ULONG fileRead(tFile *pFile, void *pDest, ULONG ulSize) {
#ifdef ACE_DEBUG
	if(!ulSize) {
		logWrite("ERR: File read size = 0!\n");
	}
#endif
	fileAccessEnable();
	ULONG ulReadCount = fread(pDest, ulSize, 1, pFile);
	fileAccessDisable();

	return ulReadCount;
}

ULONG fileWrite(tFile *pFile, const void *pSrc, ULONG ulSize) {
	fileAccessEnable();
	ULONG ulResult = fwrite(pSrc, ulSize, 1, pFile);
	fflush(pFile);
	fileAccessDisable();

	return ulResult;
}

ULONG fileSeek(tFile *pFile, ULONG ulPos, WORD wMode) {
	fileAccessEnable();
	ULONG ulResult = fseek(pFile, ulPos, wMode);
	fileAccessDisable();

	return ulResult;
}

ULONG fileGetPos(tFile *pFile) {
	fileAccessEnable();
	ULONG ulResult = ftell(pFile);
	fileAccessDisable();

	return ulResult;
}

UBYTE fileIsEof(tFile *pFile) {
	fileAccessEnable();
	UBYTE ubResult = feof(pFile);
	fileAccessDisable();

	return ubResult;
}

#if !defined(BARTMAN_GCC) // Not implemented in mini_std for now, sorry!
LONG fileVaPrintf(tFile *pFile, const char *szFmt, va_list vaArgs) {
	fileAccessEnable();
	LONG lResult = vfprintf(pFile, szFmt, vaArgs);
	fflush(pFile);
	fileAccessDisable();
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
	fileAccessEnable();
	LONG lResult = vfscanf(pFile, szFmt, vaArgs);
	fileAccessDisable();
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
	fileAccessEnable();
	fflush(pFile);
	fileAccessDisable();
}

void fileWriteStr(tFile *pFile, const char *szLine) {
	fileWrite(pFile, szLine, strlen(szLine));
}

UBYTE fileExists(const char *szPath) {
	fileAccessEnable();
	UBYTE isExisting = 0;
	tFile *pFile = fileOpen(szPath, "r");
	if(pFile) {
		isExisting = 1;
		fileClose(pFile);
	}
	fileAccessDisable();

	return isExisting;
}

UBYTE fileDelete(const char *szFilePath) {
	fileAccessEnable();
	UBYTE isSuccess = remove(szFilePath);
	fileAccessDisable();
	return isSuccess;
}

UBYTE fileMove(const char *szSource, const char *szDest) {
	fileAccessEnable();
	UBYTE isSuccess = rename(szSource, szDest);
	fileAccessDisable();
	return isSuccess;
}
