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
	tDiskFileMode eMode;
	UBYTE pBuffer[DISK_FILE_BUFFER_SIZE];
	UWORD uwBufferFill;
	UWORD uwBufferReadPos;
	UBYTE isUninterrupted;
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

	// Blitter is needed for floppy drive access NOT only on KS1.3!
	// Apparently, Aminer crashes on KS3.1 on trying to save the settings
	// when blitter isn't released and also during in-gameplay loadings.
	// TODO: Investigate if it's only needed for writes in >= v36,
	/// and add isWrite param to decide on blitter release
	// TODO: do only when reading from floppy
	// if(systemGetVersion() < 36) {
	systemReleaseBlitterToOs();
	// }
}

static void fileAccessDisable(void) {
	// if(systemGetVersion() < 36) {
	systemGetBlitterFromOs();
	// }

	systemUnuse();
}

DISKFILE_PRIVATE void diskFileClose(void *pData) {
	tDiskFileData *pDiskFileData = (tDiskFileData*)pData;

	if(!pDiskFileData->isUninterrupted) {
		fileAccessEnable();
	}

	if(pDiskFileData->eMode == DISK_FILE_MODE_WRITE) {
		// write remaining data
		fwrite(
			pDiskFileData->pBuffer, pDiskFileData->uwBufferFill, 1,
			pDiskFileData->pFileHandle
		);
		fflush(pDiskFileData->pFileHandle);
	}

	fclose(pDiskFileData->pFileHandle);
	memFree(pDiskFileData, sizeof(*pDiskFileData));
	fileAccessDisable();
}

DISKFILE_PRIVATE ULONG diskFileRead(void *pData, void *pDest, ULONG ulSize) {
	tDiskFileData *pDiskFileData = (tDiskFileData*)pData;
	UBYTE *pDestBytes = (UBYTE*)pDest;
	ULONG ulReadCount = 0;

	if(pDiskFileData->eMode != DISK_FILE_MODE_READ) {
		logWrite("ERR: Attempting to read file not opened for read\n");
	}

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
		if(!pDiskFileData->isUninterrupted) {
			fileAccessEnable();
		}
		ULONG ulReadPartSize = fread(pDestBytes, ulSize, 1, pDiskFileData->pFileHandle);
		pDestBytes += ulReadPartSize;
		ulReadCount += ulReadPartSize;
		ulSize -= ulReadPartSize;

		if(!pDiskFileData->isUninterrupted) {
			fileAccessDisable();
		}
	}

	if(ulSize) {
		// if not, fill the buffer and read remaining data from buffer
		if(pDiskFileData->uwBufferFill == pDiskFileData->uwBufferReadPos) {
			if(!pDiskFileData->isUninterrupted) {
				fileAccessEnable();
			}

			pDiskFileData->uwBufferFill = fread(
				pDiskFileData->pBuffer, DISK_FILE_BUFFER_SIZE, 1,
				pDiskFileData->pFileHandle
			);

			if(!pDiskFileData->isUninterrupted) {
				fileAccessDisable();
			}
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
	ULONG ulWritten;

	if(pDiskFileData->eMode != DISK_FILE_MODE_WRITE) {
		logWrite("ERR: Attempting to write file not opened for write\n");
	}

	if(pDiskFileData->uwBufferFill + ulSize < DISK_FILE_BUFFER_SIZE) {
		memcpy(&pDiskFileData->pBuffer[pDiskFileData->uwBufferFill], pSrc, ulSize);
		pDiskFileData->uwBufferFill += ulSize;
		ulWritten = ulSize;
	}
	else {
		if(!pDiskFileData->isUninterrupted) {
			fileAccessEnable();
		}

		// NOTE: Don't take previously buffered data into account in return value.
		// TODO: Make sure that all was written here?
		fwrite(
			pDiskFileData->pBuffer, pDiskFileData->uwBufferFill, 1,
			pDiskFileData->pFileHandle
		);
		pDiskFileData->uwBufferFill = 0;

		// Only allow small read here so that big write will go to the file directly
		if(ulSize < 100) {
			memcpy(&pDiskFileData->pBuffer[pDiskFileData->uwBufferFill], pSrc, ulSize);
			pDiskFileData->uwBufferFill = ulSize;
			ulWritten = ulSize;
		}
		else {
			ulWritten = fwrite(pSrc, ulSize, 1, pDiskFileData->pFileHandle);
		}

		if(!pDiskFileData->isUninterrupted) {
			fflush(pDiskFileData->pFileHandle);
			fileAccessDisable();
		}
	}
	return ulWritten;
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

	if(!pDiskFileData->isUninterrupted) {
		fileAccessEnable();
	}

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

	if(!pDiskFileData->isUninterrupted) {
		fileAccessDisable();
	}

	return ulResult;
}

DISKFILE_PRIVATE ULONG diskFileGetPos(void *pData) {
	tDiskFileData *pDiskFileData = (tDiskFileData*)pData;

	if(!pDiskFileData->isUninterrupted) {
		fileAccessEnable();
	}

	ULONG ulResult = ftell(pDiskFileData->pFileHandle);
	ulResult -= pDiskFileData->uwBufferFill - pDiskFileData->uwBufferReadPos;

	if(!pDiskFileData->isUninterrupted) {
		fileAccessDisable();
	}

	return ulResult;
}

DISKFILE_PRIVATE ULONG diskFileGetSize(void *pData) {
	// One could use std library to seek to end of file and use ftell,
	// but SEEK_END is not guaranteed to work.
	// http://www.cplusplus.com/reference/cstdio/fseek/
	// On the other hand, Lock/UnLock is bugged on KS1.3 and doesn't allow
	// for doing Open() on same file after using it.
	// So I ultimately do it using fseek.
	tDiskFileData *pDiskFileData = (tDiskFileData*)pData;
	if(!pDiskFileData->isUninterrupted) {
		fileAccessEnable();
	}

	LONG lOldPos = diskFileGetPos(pData);
	diskFileSeek(pData, 0, SEEK_END);
	LONG lSize = diskFileGetPos(pData);
	diskFileSeek(pData, lOldPos, SEEK_SET);

	if(!pDiskFileData->isUninterrupted) {
		fileAccessDisable();
	}

	return lSize;
}

DISKFILE_PRIVATE UBYTE diskFileIsEof(void *pData) {
	tDiskFileData *pDiskFileData = (tDiskFileData*)pData;

	if(!pDiskFileData->isUninterrupted) {
		fileAccessEnable();
	}

	UBYTE isEof = (
		(pDiskFileData->uwBufferReadPos == pDiskFileData->uwBufferFill) &&
		feof(pDiskFileData->pFileHandle)
	);

	if(!pDiskFileData->isUninterrupted) {
		fileAccessDisable();
	}

	return isEof;
}

DISKFILE_PRIVATE void diskFileFlush(void *pData) {
	tDiskFileData *pDiskFileData = (tDiskFileData*)pData;

	if(!pDiskFileData->isUninterrupted) {
		fileAccessEnable();
	}

	fflush(pDiskFileData->pFileHandle);

	if(!pDiskFileData->isUninterrupted) {
		fileAccessDisable();
	}
}

//------------------------------------------------------------------- PUBLIC FNS

tFile *diskFileOpen(const char *szPath, tDiskFileMode eMode, UBYTE isUninterrupted) {
	logBlockBegin(
		"diskFileOpen(szPath: '%s', eMode: %d, isUninterrupted: %hhu)",
		szPath, eMode, isUninterrupted
	);
	// TODO check if disk is read protected when szMode has 'a'/'r'/'x'
	// TODO: disable buffering in a/w/x modes
	fileAccessEnable();
	tFile *pFile = 0;
	FILE *pFileHandle = fopen(szPath, eMode == DISK_FILE_MODE_WRITE ? "wb" : "rb");
	if(pFileHandle == 0) {
		logWrite("ERR: Can't open file\n");
		fileAccessDisable();
	}
	else {
		tDiskFileData *pData = memAllocFast(sizeof(*pData));
		pData->pFileHandle = pFileHandle;
		pData->eMode = eMode;
		pData->uwBufferFill = 0;
		pData->uwBufferReadPos = 0;
		pData->isUninterrupted = isUninterrupted;
#if defined(ACE_FILE_USE_ONLY_DISK) // TODO: verify if still viable
		pFile = (tFile*)pData;
#else
		pFile = memAllocFast(sizeof(*pFile));
		pFile->pCallbacks = &s_sDiskFileCallbacks;
		pFile->pData = pData;
		logWrite("File handle: %p, data: %p\n", pFile, pFile->pData);
#endif
		if(!pData->isUninterrupted) {
			fileAccessDisable();
		}
	}
	logBlockEnd("diskFileOpen()");
	return pFile;
}

UBYTE diskFileExists(const char *szPath) {
	fileAccessEnable();
	UBYTE isExisting = 0;
	FILE *pFileHandle = fopen(szPath, "rb");
	if(pFileHandle) {
		isExisting = 1;
		fclose(pFileHandle);
	}
	fileAccessDisable();

	return isExisting;
}

UBYTE diskFileDelete(const char *szFilePath) {
	fileAccessEnable();
	UBYTE isSuccess = remove(szFilePath);
	fileAccessDisable();
	return isSuccess;
}

UBYTE diskFileMove(const char *szSource, const char *szDest) {
	fileAccessEnable();
	UBYTE isSuccess = rename(szSource, szDest);
	fileAccessDisable();
	return isSuccess;
}
