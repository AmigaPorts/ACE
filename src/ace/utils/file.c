/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/file.h>
#include <stdarg.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>
#include <ace/utils/endian.h>

void fileWriteStr(tFile *pFile, const char *szLine) {
	fileWriteBytes(pFile, (const UBYTE*)szLine, strlen(szLine));
}

#if defined(ACE_FILE_USE_ONLY_DISK)
#include <ace/utils/disk_file_private.h>

ALWAYS_INLINE
inline static ULONG fileReadData(tFile *pFile, void *pDest, ULONG ulSize) {
	return diskFileRead(pFile->pData, pDest, ulSize);
}

ALWAYS_INLINE
inline static ULONG fileWriteData(
	tFile *pFile, const void *pSrc, ULONG ulSize
) {
	return diskFileWrite(pFile->pData, pSrc, ulSize);
}

void fileClose(tFile *pFile) {
	diskFileClose(pFile);
}

ULONG fileSeek(tFile *pFile, LONG lPos, WORD wMode) {
	return diskFileSeek(pFile, lPos, wMode);
}

ULONG fileGetPos(tFile *pFile) {
	return diskFileGetPos(pFile);
}

LONG fileGetSize(tFile *pFile) {
	return diskFileGetSize(pFile);
}

UBYTE fileIsEof(tFile *pFile) {
	return diskFileIsEof(pFile);
}

void fileFlush(tFile *pFile) {
	diskFileFlush(pFile);
}

#else
ALWAYS_INLINE
inline static ULONG fileReadData(tFile *pFile, void *pDest, ULONG ulSize) {
	return pFile->pCallbacks->cbFileRead(pFile->pData, pDest, ulSize);
}

ALWAYS_INLINE
inline static ULONG fileWriteData(tFile *pFile, const void *pSrc, ULONG ulSize) {
	return pFile->pCallbacks->cbFileWrite(pFile->pData, pSrc, ulSize);
}

void fileClose(tFile *pFile) {
	logWrite("Closing file %p\n", pFile);
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
		return;
	}
	pFile->pCallbacks->cbFileClose(pFile->pData);
	memFree(pFile, sizeof(*pFile));
}

ULONG fileReadBytes(tFile *pFile, UBYTE *pDest, ULONG ulCount) {
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
	}
	if(!ulCount) {
		logWrite("ERR: File read size = 0\n");
	}

	ULONG ulReadCount = fileReadData(pFile, pDest, ulCount);
	return ulReadCount;
}

ULONG fileReadWords(tFile *pFile, UWORD *pDest, ULONG ulCount) {
	ULONG ulReadCount = fileReadBytes(pFile, (UBYTE*)pDest, ulCount * sizeof(UWORD)) / sizeof(UWORD);

#if defined(ENDIAN_NATIVE_LITTLE)
	for(ULONG i = ulReadCount; i--;) {
		pDest[i] = endianBigToNative16(pDest[i]);
	}
#endif

	return ulReadCount;
}

ULONG fileReadLongs(tFile *pFile, ULONG *pDest, ULONG ulCount) {
	ULONG ulReadCount = fileReadBytes(pFile, (UBYTE*)pDest, ulCount * sizeof(ULONG)) / sizeof(ULONG);

#if defined(ENDIAN_NATIVE_LITTLE)
	for(ULONG i = ulReadCount; i--;) {
		pDest[i] = endianBigToNative32(pDest[i]);
	}
#endif

	return ulReadCount;
}


ULONG fileWriteBytes(tFile *pFile, const UBYTE *pSrc, ULONG ulCount) {
	if(!pFile) {
		logWrite("ERR: Null file handle\n");
	}

	ULONG ulWriteCount = fileWriteData(pFile, pSrc, ulCount);
	return ulWriteCount;
}

ULONG fileWriteWords(tFile *pFile, const UWORD *pSrc, ULONG ulCount) {
#if defined(ENDIAN_NATIVE_LITTLE)
	// If 100-ish buffered write ops is perf penalty for modern platforms,
	// it could temporary allocate and swap all beforehand.
	ULONG ulWriteCount = 0;
	for(ULONG i = ulReadCount; i--;) {
		UWORD uwSwapped = endianNativeToBig16(pDest[i]);
		if(fileWriteBytes(pFile, (const UBYTE*)&uwSwapped, sizeof(UWORD))) {
			++ulWriteCount;
		}
	}
#else
	ULONG ulWriteCount = fileWriteBytes(
		pFile, (const UBYTE*)pSrc, ulCount * sizeof(UWORD)
	) / sizeof(UWORD);
#endif

	return ulWriteCount;
}

ULONG fileWriteLongs(tFile *pFile, const ULONG *pSrc, ULONG ulCount) {
#if defined(ENDIAN_NATIVE_LITTLE)
	// If 100-ish buffered write ops is perf penalty for modern platforms,
	// it could temporary allocate and swap all beforehand.
	ULONG ulWriteCount = 0;
	for(ULONG i = ulReadCount; i--;) {
		UWORD ulSwapped = endianNativeToBig32(pDest[i]);
		if(fileWriteBytes(pFile, (const UBYTE*)&ulSwapped, sizeof(ULONG))) {
			++ulWriteCount;
		}
	}
#else
	ULONG ulWriteCount = fileWriteBytes(
		pFile, (const UBYTE*)pSrc, ulCount * sizeof(ULONG)
	) / sizeof(ULONG);
#endif
	return ulWriteCount;
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
