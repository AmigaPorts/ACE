/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/pak_file.h>
#include <string.h>
#include <ace/utils/disk_file.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>

#if !defined(ACE_FILE_USE_ONLY_DISK)
#define ADLER32_MODULO 65521

typedef struct tPakFileSubfileData {
	tPakFile *pPak;
	ULONG ulPos;
	UWORD uwFileIndex;
} tPakFileSubfileData;

static void pakSubfileClose(void *pData);
static ULONG pakSubfileRead(void *pData, void *pDest, ULONG ulSize);
static ULONG pakSubfileWrite(void *pData, const void *pSrc, ULONG ulSize);
static ULONG pakSubfileSeek(void *pData, LONG lPos, WORD wMode);
static ULONG pakSubfileGetPos(void *pData);
static UBYTE pakSubfileIsEof(void *pData);
static void pakSubfileFlush(void *pData);

static const tFileCallbacks s_sPakSubfileCallbacks = {
	.cbFileClose = pakSubfileClose,
	.cbFileRead = pakSubfileRead,
	.cbFileWrite = pakSubfileWrite,
	.cbFileSeek = pakSubfileSeek,
	.cbFileGetPos = pakSubfileGetPos,
	.cbFileIsEof = pakSubfileIsEof,
	.cbFileFlush = pakSubfileFlush,
};

//------------------------------------------------------------------ PRIVATE FNS

static ULONG adler32Buffer(const UBYTE *pData, ULONG ulDataSize) {
	ULONG a = 1, b = 0;
	for(ULONG i = 0; i < ulDataSize; ++i) {
		a += pData[i];
		if(a >= ADLER32_MODULO) {
			a -= ADLER32_MODULO;
		}
		b += a;
		if(b >= ADLER32_MODULO) {
			b -= ADLER32_MODULO;
		}
	}
	return (b << 16) | a;
}

static void pakSubfileClose(UNUSED_ARG void *pData) {
	tPakFileSubfileData *pSubfileData = (tPakFileSubfileData*)pData;

	memFree(pSubfileData, sizeof(*pSubfileData));
}

static ULONG pakSubfileRead(void *pData, void *pDest, ULONG ulSize) {
	tPakFileSubfileData *pSubfileData = (tPakFileSubfileData*)pData;
	tPakFile *pPak = pSubfileData->pPak;

	if(pPak->pPrevReadSubfile != pSubfileData) {
		// Other file was accessed or there was a seek recently - seek back
		fileSeek(
			pPak->pFile,
			pPak->pEntries[pSubfileData->uwFileIndex].ulOffs + pSubfileData->ulPos,
			FILE_SEEK_SET
		);
		pPak->pPrevReadSubfile = pSubfileData;
	}
	ULONG ulRead = fileRead(pPak->pFile, pDest, ulSize);
	pSubfileData->ulPos += ulRead;
	return ulRead;
}

static ULONG pakSubfileWrite(
	UNUSED_ARG void *pData, UNUSED_ARG const void *pSrc, UNUSED_ARG ULONG ulSize
) {
	logWrite("ERR: Unsupported: pakSubfileWrite()\n");
	return 0;
}

static ULONG pakSubfileSeek(void *pData, LONG lPos, WORD wMode) {
	tPakFileSubfileData *pSubfileData = (tPakFileSubfileData*)pData;
	tPakFile *pPak = pSubfileData->pPak;
	const tPakFileEntry *pPakEntry = &pPak->pEntries[pSubfileData->uwFileIndex];

	if(wMode == FILE_SEEK_SET) {
		pSubfileData->ulPos = lPos;
	}
	else if(wMode == FILE_SEEK_CURRENT) {
		pSubfileData->ulPos += lPos;
	}
	else if(wMode == FILE_SEEK_END) {
		pSubfileData->ulPos = pPakEntry->ulSize + lPos;
	}
	pPak->pPrevReadSubfile = 0; // Invalidate cache

	if(pSubfileData->ulPos > pPakEntry->ulSize) {
		logWrite("ERR: Seek position %lu out of range %lu for pakFile %hu\n", pSubfileData->ulPos, pPakEntry->ulSize, pSubfileData->uwFileIndex);
		pSubfileData->ulPos = pPakEntry->ulSize;
		return 0;
	}
	return 1;
}

static ULONG pakSubfileGetPos(void *pData) {
	tPakFileSubfileData *pSubfileData = (tPakFileSubfileData*)pData;

	return pSubfileData->ulPos;
}

static UBYTE pakSubfileIsEof(void *pData) {
	tPakFileSubfileData *pSubfileData = (tPakFileSubfileData*)pData;

	return pSubfileData->ulPos <= pSubfileData->pPak->pEntries[pSubfileData->uwFileIndex].ulSize;
}

static void pakSubfileFlush(UNUSED_ARG void *pData) {
	// no-op
}

static UWORD pakFileGetFileIndex(const tPakFile *pPakFile, const char *szPath) {
	ULONG ulPathHash = adler32Buffer((UBYTE*)szPath, strlen(szPath)); // TODO: Calculate path hash
	for(UWORD i = 0; i < pPakFile->uwFileCount; ++i) {
		if(pPakFile->pEntries[i].ulPathChecksum == ulPathHash) {
			return i;
		}
	}
	return UWORD_MAX;
}

//------------------------------------------------------------------- PUBLIC FNS

tPakFile *pakFileOpen(const char *szPath) {
	logBlockBegin("pakFileOpen(szPath: '%s')", szPath);
	tFile *pMainFile = diskFileOpen(szPath, "rb");
	if(!pMainFile) {
		logBlockEnd("pakFileOpen()");
		return 0;
	}

	tPakFile *pPakFile = memAllocFast(sizeof(*pPakFile));
	pPakFile->pFile = pMainFile;
	pPakFile->pPrevReadSubfile = 0;
	fileRead(pMainFile, &pPakFile->uwFileCount, sizeof(pPakFile->uwFileCount));
	pPakFile->pEntries = memAllocFast(sizeof(pPakFile->pEntries[0]) * pPakFile->uwFileCount);
	for(UWORD i = 0; i < pPakFile->uwFileCount; ++i) {
		fileRead(pMainFile, &pPakFile->pEntries[i].ulPathChecksum, sizeof(pPakFile->pEntries[i].ulPathChecksum));
		fileRead(pMainFile, &pPakFile->pEntries[i].ulOffs, sizeof(pPakFile->pEntries[i].ulOffs));
		fileRead(pMainFile, &pPakFile->pEntries[i].ulSize, sizeof(pPakFile->pEntries[i].ulSize));
	}
	logWrite("Pak file: %p, file count: %hu\n", pPakFile, pPakFile->uwFileCount);

	logBlockEnd("pakFileOpen()");
	return pPakFile;
}

void pakFileClose(tPakFile *pPakFile) {
	logBlockBegin("pakFileClose(pPakFile: %p)", pPakFile);
	fileClose(pPakFile->pFile);
	memFree(pPakFile->pEntries, sizeof(pPakFile->pEntries[0]) * pPakFile->uwFileCount);
	memFree(pPakFile, sizeof(*pPakFile));
	logBlockEnd("pakFileClose()");
}

tFile *pakFileGetFile(tPakFile *pPakFile, const char *szInternalPath) {
	logBlockBegin("pakFileGetFile(pPakFile: %p, szInternalPath: '%s')", pPakFile, szInternalPath);
	UWORD uwFileIndex = pakFileGetFileIndex(pPakFile, szInternalPath);
	if(uwFileIndex == UWORD_MAX) {
		logWrite("ERR: Can't find subfile in pakfile\n");
		logBlockEnd("pakFileGetFile()");
		return 0;
	}
	logWrite(
		"Subfile index: %hu, offset: %lu, size: %lu\n",
		uwFileIndex,
		pPakFile->pEntries[uwFileIndex].ulOffs,
		pPakFile->pEntries[uwFileIndex].ulSize
	);

	// Create tFile, fill subfileData
	tPakFileSubfileData *pSubfileData = memAllocFast(sizeof(*pSubfileData));
	pSubfileData->pPak = pPakFile;
	pSubfileData->uwFileIndex = uwFileIndex;
	pSubfileData->ulPos = 0;
	// Prevent reading from same place if pSubfileData gets mem from recently closed file
	pPakFile->pPrevReadSubfile = 0;

	tFile *pFile = memAllocFast(sizeof(*pFile));
	pFile->pCallbacks = &s_sPakSubfileCallbacks;
	pFile->pData = pSubfileData;

	logBlockEnd("pakFileGetFile()");
	return pFile;
}

#endif
