/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/disk_file.h>
#include <ace/managers/system.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/utils/disk_file_private.h>

#if !defined(ACE_FILE_USE_ONLY_DISK)
static const tFileCallbacks s_sDiskFileCallbacks = {
	.cbFileClose = diskFileClose,
	.cbFileRead = diskFileRead,
	.cbFileWrite = diskFileWrite,
	.cbFileSeek = diskFileSeek,
	.cbFileGetPos = diskFileGetPos,
	.cbFileIsEof = diskFileIsEof,
	.cbFileFlush = diskFileFlush,
};
#endif

//------------------------------------------------------------------ PRIVATE FNS

DISKFILE_PRIVATE void diskFileClose(void *pData) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	fclose(pFile);
	systemGetBlitterFromOs();
	systemUnuse();
}

DISKFILE_PRIVATE ULONG diskFileRead(void *pData, void *pDest, ULONG ulSize) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulReadCount = fread(pDest, ulSize, 1, pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulReadCount;
}

DISKFILE_PRIVATE ULONG diskFileWrite(void *pData, const void *pSrc, ULONG ulSize) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulResult = fwrite(pSrc, ulSize, 1, pFile);
	fflush(pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulResult;
}

DISKFILE_PRIVATE ULONG diskFileSeek(void *pData, LONG lPos, WORD wMode) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulResult = fseek(pFile, lPos, wMode);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulResult;
}

DISKFILE_PRIVATE ULONG diskFileGetPos(void *pData) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	ULONG ulResult = ftell(pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ulResult;
}

DISKFILE_PRIVATE UBYTE diskFileIsEof(void *pData) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	UBYTE ubResult = feof(pFile);
	systemGetBlitterFromOs();
	systemUnuse();

	return ubResult;
}

DISKFILE_PRIVATE void diskFileFlush(void *pData) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	systemReleaseBlitterToOs();
	fflush(pFile);
	systemGetBlitterFromOs();
	systemUnuse();
}

//------------------------------------------------------------------- PUBLIC FNS

tFile *diskFileOpen(const char *szPath, const char *szMode) {
	logBlockBegin("diskFileOpen(szPath: '%s', szMode: '%s')", szPath, szMode);
	// TODO check if disk is read protected when szMode has 'a'/'r'/'x'
	systemUse();
	systemReleaseBlitterToOs();
	tFile *pFile = 0;
	FILE *pFileHandle = fopen(szPath, szMode);
	if(pFileHandle == 0) {
		logWrite("ERR: Can't open file\n");
	}
	else {
#if defined(ACE_FILE_USE_ONLY_DISK)
	pFile = (tFile*)pFileHandle;
#else
		pFile = memAllocFast(sizeof(*pFile));
		pFile->pCallbacks = &s_sDiskFileCallbacks;
		pFile->pData = pFileHandle;
		logWrite("File handle: %p, data: %p\n", pFile, pFile->pData);
#endif
	}
	systemGetBlitterFromOs();
	systemUnuse();

	logBlockEnd("diskFileOpen()");
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
