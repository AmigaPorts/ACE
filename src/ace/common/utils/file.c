/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/file.h>
#include <ace/utils/endian.h>
#include <stdarg.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>

//------------------------------------------------------------------ PRIVATE FNS

static ULONG fileReadData(tFile *pFile, void *pDest, UBYTE ubDataSize, ULONG ulCount) {
#ifdef ACE_DEBUG
	if(!ulSize) {
		logWrite("ERR: File read size = 0!\n");
	}
#endif
	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulReadCount = fread(pDest, ubDataSize, ulCount, pFile);
#if defined(ENDIAN_NATIVE_LITTLE)
	if(ubDataSize == sizeof(UBYTE)) {
		// no endian swap for bytes
	}
	if(ubDataSize == sizeof(UWORD)) {
		UWORD *pWords = pDest;
		for(ULONG i = 0; i < ulCount; ++i) {
			pWords[i] = endianBigToNative16(pWords[i]);
		}
	}
	else if(ubDataSize == sizeof(ULONG)) {
		UWORD *pLongs = pDest;
		for(ULONG i = 0; i < ulCount; ++i) {
			pLongs[i] = endianBigToNative32(pLongs[i]);
		}
	}
	else {
		logWrite("ERR: Unsupported data size: %hhu\n", ubDataSize);
	}
#endif
	systemGetBlitterFromOs();
	systemUnuse();

	return ulReadCount;
}

static ULONG fileWriteData(tFile *pFile, const void *pSource, UBYTE ubDataSize, ULONG ulCount) {
#ifdef ACE_DEBUG
	if(!ulSize) {
		logWrite("ERR: File read size = 0!\n");
	}
#endif
	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulWriteCount = 0;
#if defined(ENDIAN_NATIVE_LITTLE)
	if(ubDataSize == sizeof(UBYTE)) {
		// no endian swap for bytes
		ulWriteCount = fwrite(pSource, ubDataSize, ulCount, pFile);
	}
	else if(ubDataSize == sizeof(UWORD)) {
		const UWORD *pWords = pSource;
		for(ULONG i = 0; i < ulCount; ++i) {
			UWORD uwReversed = endianBigToNative16(pWords[i]);
			ulWriteCount += fwrite(&uwReversed, ubDataSize, 1, pFile);
		}
	}
	else if(ubDataSize == sizeof(ULONG)) {
		const UWORD *pLongs = pSource;
		for(ULONG i = 0; i < ulCount; ++i) {
			ULONG ulReversed = endianBigToNative32(pLongs[i]);
			ulWriteCount += fwrite(&ulReversed, ubDataSize, 1, pFile);
		}
	}
	else {
		logWrite("ERR: Unsupported data size: %hhu\n", ubDataSize);
	}
#endif
	systemGetBlitterFromOs();
	systemUnuse();

	return ulWriteCount;
}

//------------------------------------------------------------------- PUBLIC FNS

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
	FILE *pFile = fopen(szPath, "rb");
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

tFile *fileOpen(const char *szPath, const char *szMode) {
	// TODO check if disk is read protected when szMode has 'a'/'r'/'x'
	systemUse();
	systemReleaseBlitterToOs();
	FILE *pFile = fopen(szPath, szMode);
	systemGetBlitterFromOs();
	systemUnuse();

	return pFile;
}

void fileClose(tFile *pFile) {
	systemUse();
	systemReleaseBlitterToOs();
	fclose(pFile);
	systemGetBlitterFromOs();
	systemUnuse();
}

ULONG fileReadBytes(tFile *pFile, UBYTE *pDest, ULONG ulSize) {
	ULONG ulReadCount = fileReadData(pFile, pDest, sizeof(UBYTE), ulSize);
	return ulReadCount;
}

ULONG fileReadWords(tFile *pFile, UWORD *pDest, ULONG ulSize) {
	ULONG ulReadCount = fileReadData(pFile, pDest, sizeof(UWORD), ulSize);
	return ulReadCount;
}

ULONG fileReadLongs(tFile *pFile, ULONG *pDest, ULONG ulSize) {
	ULONG ulReadCount = fileReadData(pFile, pDest, sizeof(ULONG), ulSize);
	return ulReadCount;
}


ULONG fileWriteBytes(tFile *pFile, const void *pSrc, ULONG ulSize) {
	ULONG ulWriteCount = fileWriteData(pFile, pSrc, sizeof(UBYTE), ulSize);
	return ulWriteCount;
}

ULONG fileWriteWords(tFile *pFile, const void *pSrc, ULONG ulSize) {
	ULONG ulWriteCount = fileWriteData(pFile, pSrc, sizeof(UWORD), ulSize);
	return ulWriteCount;
}

ULONG fileWriteLongs(tFile *pFile, const void *pSrc, ULONG ulSize) {
	ULONG ulWriteCount = fileWriteData(pFile, pSrc, sizeof(ULONG), ulSize);
	return ulWriteCount;
}

ULONG fileSeek(tFile *pFile, ULONG ulPos, WORD wMode) {
	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulResult = fseek(pFile, ulPos, wMode);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulResult;
}

ULONG fileGetPos(tFile *pFile) {
	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulResult = ftell(pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulResult;
}

UBYTE fileIsEof(tFile *pFile) {
	systemUse();
	systemReleaseBlitterToOs();
	UBYTE ubResult = feof(pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ubResult;
}

#if !defined(BARTMAN_GCC) // Not implemented in mini_std for now, sorry!
LONG fileVaPrintf(tFile *pFile, const char *szFmt, va_list vaArgs) {
	systemUse();
	systemReleaseBlitterToOs();
	LONG lResult = vfprintf(pFile, szFmt, vaArgs);
	fflush(pFile);
	systemGetBlitterFromOs();
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
	systemReleaseBlitterToOs();
	LONG lResult = vfscanf(pFile, szFmt, vaArgs);
	systemGetBlitterFromOs();
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
	systemReleaseBlitterToOs();
	fflush(pFile);
	systemGetBlitterFromOs();
	systemUnuse();
}

void fileWriteStr(tFile *pFile, const char *szLine) {
	fileWriteBytes(pFile, szLine, strlen(szLine));
}

UBYTE fileExists(const char *szPath) {
	systemUse();
	systemReleaseBlitterToOs();
	UBYTE isExisting = 0;
	tFile *pFile = fileOpen(szPath, "rb");
	if(pFile) {
		isExisting = 1;
		fileClose(pFile);
	}
	systemGetBlitterFromOs();
	systemUnuse();

	return isExisting;
}

UBYTE fileDelete(const char *szFilePath) {
	systemUse();
	systemReleaseBlitterToOs();
	UBYTE isSuccess = remove(szFilePath);
	systemGetBlitterFromOs();
	systemUnuse();
	return isSuccess;
}

UBYTE fileMove(const char *szSource, const char *szDest) {
	systemUse();
	systemReleaseBlitterToOs();
	UBYTE isSuccess = rename(szSource, szDest);
	systemGetBlitterFromOs();
	systemUnuse();
	return isSuccess;
}
