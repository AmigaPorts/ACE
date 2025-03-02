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

DISKFILE_PRIVATE void diskFileClose(void *pData) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	fileAccessEnable();
	fclose(pFile);
	fileAccessDisable();
	systemUnuse();
}

DISKFILE_PRIVATE ULONG diskFileRead(void *pData, void *pDest, ULONG ulSize) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	fileAccessEnable();
	ULONG ulReadCount = fread(pDest, ulSize, 1, pFile);
	fileAccessDisable();
	systemUnuse();

	return ulReadCount;
}

DISKFILE_PRIVATE ULONG diskFileWrite(void *pData, const void *pSrc, ULONG ulSize) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	fileAccessEnable();
	ULONG ulResult = fwrite(pSrc, ulSize, 1, pFile);
	fflush(pFile);
	fileAccessDisable();
	systemUnuse();

	return ulResult;
}

DISKFILE_PRIVATE ULONG diskFileSeek(void *pData, LONG lPos, WORD wMode) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	fileAccessEnable();
	ULONG ulResult = fseek(pFile, lPos, wMode);
	fileAccessDisable();
	systemUnuse();

	return ulResult;
}

DISKFILE_PRIVATE ULONG diskFileGetPos(void *pData) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	fileAccessEnable();
	ULONG ulResult = ftell(pFile);
	fileAccessDisable();
	systemUnuse();

	return ulResult;
}

DISKFILE_PRIVATE UBYTE diskFileIsEof(void *pData) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	fileAccessEnable();
	UBYTE ubResult = feof(pFile);
	fileAccessDisable();
	systemUnuse();

	return ubResult;
}

DISKFILE_PRIVATE void diskFileFlush(void *pData) {
	FILE *pFile = (FILE*)pData;

	systemUse();
	fileAccessEnable();
	fflush(pFile);
	fileAccessDisable();
	systemUnuse();
}

//------------------------------------------------------------------- PUBLIC FNS

tFile *diskFileOpen(const char *szPath, const char *szMode) {
	logBlockBegin("diskFileOpen(szPath: '%s', szMode: '%s')", szPath, szMode);
	// TODO check if disk is read protected when szMode has 'a'/'r'/'x'
	systemUse();
	fileAccessEnable();
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
	fileAccessDisable();
	systemUnuse();

	logBlockEnd("diskFileOpen()");
	return pFile;
}

UBYTE diskFileExists(const char *szPath) {
	systemUse();
	fileAccessEnable();
	UBYTE isExisting = 0;
	FILE *pFileHandle = fopen(szPath, "r");
	if(pFileHandle) {
		isExisting = 1;
		fclose(pFileHandle);
	}
	fileAccessDisable();
	systemUnuse();

	return isExisting;
}

UBYTE diskFileDelete(const char *szFilePath) {
	systemUse();
	fileAccessEnable();
	UBYTE isSuccess = remove(szFilePath);
	fileAccessDisable();
	systemUnuse();
	return isSuccess;
}

UBYTE diskFileMove(const char *szSource, const char *szDest) {
	systemUse();
	fileAccessEnable();
	UBYTE isSuccess = rename(szSource, szDest);
	fileAccessDisable();
	systemUnuse();
	return isSuccess;
}
