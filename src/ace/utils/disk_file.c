/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/disk_file.h>
#include <ace/managers/system.h>
#include <ace/managers/memory.h>

static void diskFileClose(void *pData);
static ULONG diskFileRead(void *pData, void *pDest, ULONG ulSize);
static ULONG diskFileWrite(void *pData, const void *pSrc, ULONG ulSize);
static ULONG diskFileSeek(void *pData, ULONG ulPos, WORD wMode);
static ULONG diskFileGetPos(void *pData);
static UBYTE diskFileIsEof(void *pData);
static void diskFileFlush(void *pData);

static const tFileCallbacks s_sDiskFileCallbacks = {
	.cbFileClose = diskFileClose,
	.cbFileRead = diskFileRead,
	.cbFileWrite = diskFileWrite,
	.cbFileSeek = diskFileSeek,
	.cbFileGetPos = diskFileGetPos,
	.cbFileIsEof = diskFileIsEof,
	.cbFileFlush = diskFileFlush,
};

//------------------------------------------------------------------ PRIVATE FNS

static void diskFileClose(void *pData) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	fclose(pFile);
	systemGetBlitterFromOs();
	systemUnuse();
}

static ULONG diskFileRead(void *pData, void *pDest, ULONG ulSize) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulReadCount = fread(pDest, ulSize, 1, pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulReadCount;
}

static ULONG diskFileWrite(void *pData, const void *pSrc, ULONG ulSize) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulResult = fwrite(pSrc, ulSize, 1, pFile);
	fflush(pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulResult;
}

static ULONG diskFileSeek(void *pData, ULONG ulPos, WORD wMode) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulResult = fseek(pFile, ulPos, wMode);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulResult;
}

static ULONG diskFileGetPos(void *pData) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulResult = ftell(pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulResult;
}

static UBYTE diskFileIsEof(void *pData) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	UBYTE ubResult = feof(pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ubResult;
}

static void diskFileFlush(void *pData) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	fflush(pFile);
	systemGetBlitterFromOs();
	systemUnuse();
}

//------------------------------------------------------------------- PUBLIC FNS

tFile *diskFileOpen(const char *szPath, const char *szMode) {
	// TODO check if disk is read protected when szMode has 'a'/'r'/'x'
	systemUse();
	systemReleaseBlitterToOs();
	tFile *pFile = 0;
	FILE *pFileHandle = fopen(szPath, szMode);
	if(pFileHandle != 0) {
		pFile = memAllocFast(sizeof(*pFile));
		pFile->pCallbacks = &s_sDiskFileCallbacks;
		pFile->pData = pFileHandle;
	}
	systemGetBlitterFromOs();
	systemUnuse();

	return pFile;
}

UBYTE diskFileExists(const char *szPath) {
	systemUse();
	systemReleaseBlitterToOs();
	UBYTE isExisting = 0;
	tFile *pFile = diskFileOpen(szPath, "r");
	if(pFile) {
		isExisting = 1;
		fileClose(pFile);
	}
	systemGetBlitterFromOs();
	systemUnuse();

	return isExisting;
}

UBYTE diskFileDelete(const char *szFilePath) {
	systemUse();
	systemReleaseBlitterToOs();
	UBYTE isSuccess = remove(szFilePath);
	systemGetBlitterFromOs();
	systemUnuse();
	return isSuccess;
}

UBYTE diskFileMove(const char *szSource, const char *szDest) {
	systemUse();
	systemReleaseBlitterToOs();
	UBYTE isSuccess = rename(szSource, szDest);
	systemGetBlitterFromOs();
	systemUnuse();
	return isSuccess;
}
