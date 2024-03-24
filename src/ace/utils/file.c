/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/file.h>
#include <stdarg.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>
#include <ace/utils/assume.h>

LONG fileGetSize(const char *szFilePath) {
	assumeNotNull(szFilePath);
	// One could use std library to seek to end of file and use ftell,
	// but SEEK_END is not guaranteed to work.
	// http://www.cplusplus.com/reference/cstdio/fseek/
	// On the other hand, Lock/UnLock is bugged on KS1.3 and doesn't allow
	// for doing Open() on same file after using it.
	// So I ultimately do it using fseek.

	systemUse();
	systemReleaseBlitterToOs();
	logBlockBegin("fileGetSize(szFilePath: '%s')", szFilePath);
	FILE *pFile = fopen(szFilePath, "r");
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

tFile *fileOpen(const char *szFilePath, const char *szMode) {
	assumeNotNull(szFilePath);
	assumeNotNull(szMode);

	// TODO check if disk is write protected when szMode has 'a'/'w'/'x'
	systemUse();
	systemReleaseBlitterToOs();
	FILE *pFile = fopen(szFilePath, szMode);
	systemGetBlitterFromOs();
	systemUnuse();

	return pFile;
}

void fileClose(tFile *pFile) {
	assumeNotNull(pFile);

	systemUse();
	systemReleaseBlitterToOs();
	fclose(pFile);
	systemGetBlitterFromOs();
	systemUnuse();
}

ULONG fileRead(tFile *pFile, void *pDest, ULONG ulSize) {
	assumeNotNull(pFile);
	assumeNotNull(pDest);
	assumeMsg(ulSize != 0, "File read size is zero");

	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulReadCount = fread(pDest, ulSize, 1, pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulReadCount;
}

ULONG fileWrite(tFile *pFile, const void *pSrc, ULONG ulSize) {
	assumeNotNull(pFile);
	assumeNotNull(pSrc);
	assumeMsg(ulSize != 0, "File write size is zero");

	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulResult = fwrite(pSrc, ulSize, 1, pFile);
	fflush(pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulResult;
}

ULONG fileSeek(tFile *pFile, ULONG ulPos, WORD wMode) {
	assumeNotNull(pFile);

	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulResult = fseek(pFile, ulPos, wMode);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulResult;
}

ULONG fileGetPos(tFile *pFile) {
	assumeNotNull(pFile);

	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulResult = ftell(pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulResult;
}

UBYTE fileIsEof(tFile *pFile) {
	assumeNotNull(pFile);

	systemUse();
	systemReleaseBlitterToOs();
	UBYTE ubResult = feof(pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ubResult;
}

#if !defined(BARTMAN_GCC) // Not implemented in mini_std for now, sorry!
LONG fileVaPrintf(tFile *pFile, const char *szFmt, va_list vaArgs) {
	assumeNotNull(pFile);

	systemUse();
	systemReleaseBlitterToOs();
	LONG lResult = vfprintf(pFile, szFmt, vaArgs);
	fflush(pFile);
	systemGetBlitterFromOs();
	systemUnuse();
	return lResult;
}

LONG filePrintf(tFile *pFile, const char *szFmt, ...) {
	assumeNotNull(pFile);

	va_list vaArgs;
	va_start(vaArgs, szFmt);
	LONG lResult = fileVaPrintf(pFile, szFmt, vaArgs);
	va_end(vaArgs);
	return lResult;
}

LONG fileVaScanf(tFile *pFile, const char *szFmt, va_list vaArgs) {
	assumeNotNull(pFile);

	systemUse();
	systemReleaseBlitterToOs();
	LONG lResult = vfscanf(pFile, szFmt, vaArgs);
	systemGetBlitterFromOs();
	systemUnuse();
	return lResult;
}

LONG fileScanf(tFile *pFile, const char *szFmt, ...) {
	assumeNotNull(pFile);

	va_list vaArgs;
	va_start(vaArgs, szFmt);
	LONG lResult = fileVaScanf(pFile, szFmt, vaArgs);
	va_end(vaArgs);
	return lResult;
}
#endif

void fileFlush(tFile *pFile) {
	assumeNotNull(pFile);

	systemUse();
	systemReleaseBlitterToOs();
	fflush(pFile);
	systemGetBlitterFromOs();
	systemUnuse();
}

void fileWriteStr(tFile *pFile, const char *szLine) {
	assumeNotNull(pFile);

	fileWrite(pFile, szLine, strlen(szLine));
}

UBYTE fileExists(const char *szFilePath) {
	assumeNotNull(szFilePath);

	systemUse();
	systemReleaseBlitterToOs();
	UBYTE isExisting = 0;
	tFile *pFile = fileOpen(szFilePath, "r");
	if(pFile) {
		isExisting = 1;
		fileClose(pFile);
	}
	systemGetBlitterFromOs();
	systemUnuse();

	return isExisting;
}

UBYTE fileDelete(const char *szFilePath) {
	assumeNotNull(szFilePath);

	systemUse();
	systemReleaseBlitterToOs();
	UBYTE isSuccess = remove(szFilePath);
	systemGetBlitterFromOs();
	systemUnuse();
	return isSuccess;
}

UBYTE fileMove(const char *szSourcePath, const char *szDestPath) {
	assumeNotNull(szSourcePath);
	assumeNotNull(szDestPath);

	systemUse();
	systemReleaseBlitterToOs();
	UBYTE isSuccess = rename(szSourcePath, szDestPath);
	systemGetBlitterFromOs();
	systemUnuse();
	return isSuccess;
}
