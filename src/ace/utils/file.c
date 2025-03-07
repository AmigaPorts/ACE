/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/file.h>
#include <stdarg.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>

void fileWriteStr(tFile *pFile, const char *szLine) {
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
	}
	fileWrite(pFile, szLine, strlen(szLine));
}

#if defined(ACE_FILE_USE_ONLY_DISK)
#include <ace/utils/disk_file_private.h>

void fileClose(tFile *pFile) {
	diskFileClose(pFile);
}

ULONG fileRead(tFile *pFile, void *pDest, ULONG ulSize) {
	return diskFileRead(pFile, pDest, ulSize);
}

ULONG fileWrite(tFile *pFile, const void *pSrc, ULONG ulSize) {
	return diskFileWrite(pFile, pSrc, ulSize);
}

ULONG fileSeek(tFile *pFile, LONG lPos, WORD wMode) {
	return diskFileSeek(pFile, lPos, wMode);
}

ULONG fileGetPos(tFile *pFile) {
	return diskFileGetPos(pFile);
}

ULONG fileGetSize(tFile *pFile) {
	return diskFileGetSize(pFile);
}

UBYTE fileIsEof(tFile *pFile) {
	return diskFileIsEof(pFile);
}

void fileFlush(tFile *pFile) {
	diskFileFlush(pFile);
}

#else
void fileClose(tFile *pFile) {
	logWrite("Closing file %p\n", pFile);
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
		return;
	}
	pFile->pCallbacks->cbFileClose(pFile->pData);
	memFree(pFile, sizeof(*pFile));
}

ULONG fileRead(tFile *pFile, void *pDest, ULONG ulSize) {
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
	}
	if(!ulSize) {
		logWrite("ERR: File read size = 0\n");
	}
	return pFile->pCallbacks->cbFileRead(pFile->pData, pDest, ulSize);
}

ULONG fileWrite(tFile *pFile, const void *pSrc, ULONG ulSize) {
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
	}
	return pFile->pCallbacks->cbFileWrite(pFile->pData, pSrc, ulSize);
}

ULONG fileSeek(tFile *pFile, LONG lPos, WORD wMode) {
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
	}
	return pFile->pCallbacks->cbFileSeek(pFile->pData, lPos, wMode);
}

ULONG fileGetPos(tFile *pFile) {
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
	}
	return pFile->pCallbacks->cbFileGetPos(pFile->pData);
}

LONG fileGetSize(tFile *pFile) {
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
	}
	return pFile->pCallbacks->cbFileGetSize(pFile->pData);
}

UBYTE fileIsEof(tFile *pFile) {
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
	}
	return pFile->pCallbacks->cbFileIsEof(pFile->pData);
}

void fileFlush(tFile *pFile) {
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
	}
	pFile->pCallbacks->cbFileFlush(pFile->pData);
}
#endif
