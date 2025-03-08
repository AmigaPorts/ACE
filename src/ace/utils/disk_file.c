/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/disk_file.h>
#include <ace/managers/system.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/utils/disk_file_private.h>

#define DISK_FILE_BUFFER_SIZE 512

typedef struct tDiskFileData {
	FILE *pFileHandle;
	UBYTE pBuffer[DISK_FILE_BUFFER_SIZE];
	UWORD uwBufferFill;
	UWORD uwBufferReadPos;
} tDiskFileData;

#if !defined(ACE_FILE_USE_ONLY_DISK)
static const tFileCallbacks s_sDiskFileCallbacks = {
	.cbFileClose = diskFileClose,
	.cbFileRead = diskFileRead,
	.cbFileWrite = diskFileWrite,
	.cbFileSeek = diskFileSeek,
	.cbFileGetPos = diskFileGetPos,
	.cbFileGetSize = diskFileGetSize,
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
	tDiskFileData *pDiskFileData = (tDiskFileData*)pData;

	systemUse();
	fileAccessEnable();
	fclose(pDiskFileData->pFileHandle);
	fileAccessDisable();
	memFree(pDiskFileData, sizeof(*pDiskFileData));
	systemUnuse();
}

DISKFILE_PRIVATE ULONG diskFileRead(void *pData, void *pDest, ULONG ulSize) {
	tDiskFileData *pDiskFileData = (tDiskFileData*)pData;
	UBYTE *pDestBytes = (UBYTE*)pDest;
	ULONG ulReadCount = 0;

	// copy some data from buffer
	UWORD uwReadyBytes = pDiskFileData->uwBufferFill - pDiskFileData->uwBufferReadPos;
	UWORD uwBytesToCopy = MIN(uwReadyBytes, ulSize);
	if(uwBytesToCopy) {
		for(UWORD i = 0; i < uwBytesToCopy; ++i) {
			*(pDestBytes++) = pDiskFileData->pBuffer[pDiskFileData->uwBufferReadPos + i];
		}
		ulReadCount += uwBytesToCopy;
		pDiskFileData->uwBufferReadPos += uwBytesToCopy;
		ulSize -= uwBytesToCopy;
	}

	if(ulSize && ulSize > DISK_FILE_BUFFER_SIZE) {
		// if remaining data is bigger than buffer, read rest directly
		systemUse();
		fileAccessEnable();
		ULONG ulReadPartSize = fread(pDestBytes, ulSize, 1, pDiskFileData->pFileHandle);
		pDestBytes += ulReadPartSize;
		ulReadCount += ulReadPartSize;
		ulSize -= ulReadPartSize;
		fileAccessDisable();
		systemUnuse();
	}

	if(ulSize) {
		// if not, fill the buffer and read remaining data from buffer
		if(pDiskFileData->uwBufferFill == pDiskFileData->uwBufferReadPos) {
			systemUse();
			fileAccessEnable();
			pDiskFileData->uwBufferFill = fread(
				pDiskFileData->pBuffer, DISK_FILE_BUFFER_SIZE, 1,
				pDiskFileData->pFileHandle
			);
			fileAccessDisable();
			systemUnuse();
			pDiskFileData->uwBufferReadPos = 0;
		}

		uwReadyBytes = pDiskFileData->uwBufferFill - pDiskFileData->uwBufferReadPos;
		uwBytesToCopy = MIN(uwReadyBytes, ulSize);
		if(uwBytesToCopy) {
			for(UWORD i = 0; i < uwBytesToCopy; ++i) {
				*(pDestBytes++) = pDiskFileData->pBuffer[i];
			}
			ulReadCount += uwBytesToCopy;
			ulSize -= uwBytesToCopy;
			pDiskFileData->uwBufferReadPos += uwBytesToCopy;
		}
	}

	return ulReadCount;
}

DISKFILE_PRIVATE ULONG diskFileWrite(void *pData, const void *pSrc, ULONG ulSize) {
	tDiskFileData *pDiskFileData = (tDiskFileData*)pData;

	systemUse();
	fileAccessEnable();
	ULONG ulResult = fwrite(pSrc, ulSize, 1, pDiskFileData->pFileHandle);
	fflush(pDiskFileData->pFileHandle);
	fileAccessDisable();
	systemUnuse();

	return ulResult;
}

DISKFILE_PRIVATE ULONG diskFileSeek(void *pData, LONG lPos, WORD wMode) {
	tDiskFileData *pDiskFileData = (tDiskFileData*)pData;

	if(wMode == SEEK_SET) {
		LONG lDelta = lPos - diskFileGetPos(pData);
		if(
			(lDelta <= 0 && -lDelta < pDiskFileData->uwBufferReadPos) ||
			(lDelta > 0 && lDelta < pDiskFileData->uwBufferFill - pDiskFileData->uwBufferReadPos)
		) {
			pDiskFileData->uwBufferReadPos += lDelta;
			return 0;
		}
	}

	if(wMode == SEEK_CUR && (
		(lPos > 0 && lPos < pDiskFileData->uwBufferFill - pDiskFileData->uwBufferReadPos) ||
		(lPos < pDiskFileData->uwBufferReadPos)
	)) {
		pDiskFileData->uwBufferReadPos += lPos;
		return 0;
	}

	systemUse();
	fileAccessEnable();

	// Failsafe for buffering
	if(wMode == SEEK_CUR) {
		wMode = SEEK_SET;
		lPos += diskFileGetPos(pData);
	}

	ULONG ulResult = fseek(pDiskFileData->pFileHandle, lPos, wMode);
	if(pDiskFileData->uwBufferFill) {
		logWrite("WARN: slow - read buffer discard\n");
		pDiskFileData->uwBufferReadPos = 0;
		pDiskFileData->uwBufferFill = 0;
	}
	fileAccessDisable();
	systemUnuse();

	return ulResult;
}

DISKFILE_PRIVATE ULONG diskFileGetPos(void *pData) {
	tDiskFileData *pDiskFileData = (tDiskFileData*)pData;

	systemUse();
	fileAccessEnable();
	ULONG ulResult = ftell(pDiskFileData->pFileHandle);
	ulResult -= pDiskFileData->uwBufferFill - pDiskFileData->uwBufferReadPos;
	fileAccessDisable();
	systemUnuse();

	return ulResult;
}

DISKFILE_PRIVATE ULONG diskFileGetSize(void *pData) {
	// One could use std library to seek to end of file and use ftell,
	// but SEEK_END is not guaranteed to work.
	// http://www.cplusplus.com/reference/cstdio/fseek/
	// On the other hand, Lock/UnLock is bugged on KS1.3 and doesn't allow
	// for doing Open() on same file after using it.
	// So I ultimately do it using fseek.

	systemUse();
	fileAccessEnable();
	LONG lOldPos = diskFileGetPos(pData);
	diskFileSeek(pData, 0, SEEK_END);
	LONG lSize = diskFileGetPos(pData);
	diskFileSeek(pData, lOldPos, SEEK_SET);
	fileAccessDisable();
	systemUnuse();

	return lSize;
}

DISKFILE_PRIVATE UBYTE diskFileIsEof(void *pData) {
	tDiskFileData *pDiskFileData = (tDiskFileData*)pData;

	systemUse();
	fileAccessEnable();
	UBYTE isEof = (
		(pDiskFileData->uwBufferReadPos == pDiskFileData->uwBufferFill) &&
		feof(pDiskFileData->pFileHandle)
	);
	fileAccessDisable();
	systemUnuse();

	return isEof;
}

DISKFILE_PRIVATE void diskFileFlush(void *pData) {
	tDiskFileData *pDiskFileData = (tDiskFileData*)pData;

	systemUse();
	fileAccessEnable();
	fflush(pDiskFileData->pFileHandle);
	fileAccessDisable();
	systemUnuse();
}

//------------------------------------------------------------------- PUBLIC FNS

tFile *diskFileOpen(const char *szPath, const char *szMode) {
	logBlockBegin("diskFileOpen(szPath: '%s', szMode: '%s')", szPath, szMode);
	// TODO check if disk is read protected when szMode has 'a'/'r'/'x'
	// TODO: disable buffering in a/w/x modes
	systemUse();
	fileAccessEnable();
	tFile *pFile = 0;
	FILE *pFileHandle = fopen(szPath, szMode);
	if(pFileHandle == 0) {
		logWrite("ERR: Can't open file\n");
	}
	else {
#if defined(ACE_FILE_USE_ONLY_DISK) // TODO: verify if still viable
	pFile = (tFile*)pFileHandle;
#else
		tDiskFileData *pData = memAllocFast(sizeof(*pData));
		pData->pFileHandle = pFileHandle;
		pData->uwBufferFill = 0;
		pData->uwBufferReadPos = 0;
		pFile = memAllocFast(sizeof(*pFile));
		pFile->pCallbacks = &s_sDiskFileCallbacks;
		pFile->pData = pData;
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
