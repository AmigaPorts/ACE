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

typedef UBYTE (*tCbCompressReadByte)(UBYTE *pOut, void *pData);

typedef enum tCompressUnpackStateKind {
	COMPRESS_UNPACK_STATE_KIND_READ_CTL,
	COMPRESS_UNPACK_STATE_KIND_PROCESS_CTL_BIT,
	COMPRESS_UNPACK_STATE_KIND_END_CTL,
	COMPRESS_UNPACK_STATE_KIND_WRITE_RLE,
	COMPRESS_UNPACK_STATE_KIND_DONE,
} tCompressUnpackStateKind;

typedef enum tCompressUnpackerResult {
	COMPRESS_UNPACK_RESULT_BUSY,
	COMPRESS_UNPACK_RESULT_BUSY_WROTE_BYTE,
	COMPRESS_UNPACK_RESULT_DONE,
} tCompressUnpackerResult;

typedef struct tCompressUnpacker {
	tCompressUnpackStateKind eCurrentState;
	UBYTE pLookup[0x1000];
	void *pSubfileData;
	ULONG ulCompressedSize;
	ULONG ulUncompressedSize;
	UBYTE ubCtlByte;
	UBYTE ubCtlBitIndex;
	UBYTE ubRleLength;
	UBYTE ubOut;
	UWORD uwRleStart;
	UWORD uwLookupPos;
	ULONG ulUnpackedCount;
} tCompressUnpacker;

typedef struct tPakFileSubfileData {
	tPakFile *pPak;
	ULONG ulPos;
	UWORD uwFileIndex;
} tPakFileSubfileData;

typedef struct tPakFileCompressedData {
	tCompressUnpacker sUnpacker;
	tFile *pSubfile;
} tPakFileCompressedData;

static void pakSubfileClose(void *pData);
static ULONG pakSubfileRead(void *pData, void *pDest, ULONG ulSize);
static ULONG pakSubfileWrite(void *pData, const void *pSrc, ULONG ulSize);
static ULONG pakSubfileSeek(void *pData, LONG lPos, WORD wMode);
static ULONG pakSubfileGetPos(void *pData);
static UBYTE pakSubfileIsEof(void *pData);
static void pakSubfileFlush(void *pData);

static void pakCompressedClose(void *pData);
static ULONG pakCompressedRead(void *pData, void *pDest, ULONG ulSize);
static ULONG pakCompressedWrite(void *pData, const void *pSrc, ULONG ulSize);
static ULONG pakCompressedSeek(void *pData, LONG lPos, WORD wMode);
static ULONG pakCompressedGetPos(void *pData);
static UBYTE pakCompressedIsEof(void *pData);
static void pakCompressedFlush(void *pData);

static const tFileCallbacks s_sPakSubfileCallbacks = {
	.cbFileClose = pakSubfileClose,
	.cbFileRead = pakSubfileRead,
	.cbFileWrite = pakSubfileWrite,
	.cbFileSeek = pakSubfileSeek,
	.cbFileGetPos = pakSubfileGetPos,
	.cbFileIsEof = pakSubfileIsEof,
	.cbFileFlush = pakSubfileFlush,
};

static const tFileCallbacks s_sPakCompressedCallbacks = {
	.cbFileClose = pakCompressedClose,
	.cbFileRead = pakCompressedRead,
	.cbFileWrite = pakCompressedWrite,
	.cbFileSeek = pakCompressedSeek,
	.cbFileGetPos = pakCompressedGetPos,
	.cbFileIsEof = pakCompressedIsEof,
	.cbFileFlush = pakCompressedFlush,
};

//------------------------------------------------------------------ PRIVATE FNS

void compressUnpackerInit(
	tCompressUnpacker *pUnpacker, ULONG ulCompressedSize, size_t ulUncompressedSize,
	void *pSubfileData
) {
	memset(pUnpacker, 0, sizeof(*pUnpacker));
	pUnpacker->ulCompressedSize = ulCompressedSize;
	pUnpacker->ulUncompressedSize = ulUncompressedSize;
	pUnpacker->pSubfileData = pSubfileData;
}

static void rleTableWrite(UBYTE *pTable, UWORD *pPos, UBYTE ubData) {
	pTable[*pPos] = ubData;
	(*pPos)++;
	(*pPos) &= 0xfff;
}

static UBYTE rleTableRead(const UBYTE *pTable, UWORD *pPos) {
	UBYTE ubData;

	ubData = pTable[*pPos];
	(*pPos)++;
	(*pPos) &= 0xfff;

	return ubData;
}

static tCompressUnpackerResult compressUnpackerProcess(
	tCompressUnpacker *pUnpacker
) {
	// TODO: handle cbReadByte not returning any bytes
	switch(pUnpacker->eCurrentState) {
		case COMPRESS_UNPACK_STATE_KIND_PROCESS_CTL_BIT:
			if(pUnpacker->ubCtlBitIndex--) {
				UBYTE ubBitValue = pUnpacker->ubCtlByte & 1;
				pUnpacker->ubCtlByte >>= 1;
				if (ubBitValue) {
					// Output the next byte and store it in the table
					UBYTE ubRawByte;
					pakSubfileRead(pUnpacker->pSubfileData, &ubRawByte, 1);
					rleTableWrite(pUnpacker->pLookup, &pUnpacker->uwLookupPos, ubRawByte);
					pUnpacker->ubOut = ubRawByte;
					pUnpacker->ulUnpackedCount++;
					return COMPRESS_UNPACK_RESULT_BUSY_WROTE_BYTE;
				}
				else {
					UBYTE ubHi, ubLo;
					pakSubfileRead(pUnpacker->pSubfileData, &ubLo, 1);
					pakSubfileRead(pUnpacker->pSubfileData, &ubHi, 1);
					UWORD uwRleCtl = (ubLo << 0) | (ubHi << 8);

					pUnpacker->ubRleLength = ((uwRleCtl & 0xf000) >> 12) + 3;
					pUnpacker->uwRleStart = uwRleCtl & 0xfff;
					pUnpacker->eCurrentState = COMPRESS_UNPACK_STATE_KIND_WRITE_RLE;
				}
				break;
			}
			// fallthrough
		case COMPRESS_UNPACK_STATE_KIND_END_CTL:
			if (pUnpacker->ulUnpackedCount >= pUnpacker->ulUncompressedSize) {
				pUnpacker->eCurrentState = COMPRESS_UNPACK_STATE_KIND_DONE;
				break;
			}
			// fallthrough
		case COMPRESS_UNPACK_STATE_KIND_READ_CTL:
			pakSubfileRead(pUnpacker->pSubfileData, &pUnpacker->ubCtlByte, 1);
			ULONG ulRemaining = pUnpacker->ulUncompressedSize - pUnpacker->ulUnpackedCount;
			pUnpacker->ubCtlBitIndex = MIN(8, ulRemaining);
			pUnpacker->eCurrentState = COMPRESS_UNPACK_STATE_KIND_PROCESS_CTL_BIT;
			break;
		case COMPRESS_UNPACK_STATE_KIND_WRITE_RLE:
			if(pUnpacker->ubRleLength--) {
				UBYTE ubRawByte = rleTableRead(pUnpacker->pLookup, &pUnpacker->uwRleStart);
				rleTableWrite(pUnpacker->pLookup, &pUnpacker->uwLookupPos, ubRawByte);
				pUnpacker->ubOut = ubRawByte;
				pUnpacker->ulUnpackedCount++;
				return COMPRESS_UNPACK_RESULT_BUSY_WROTE_BYTE;
			}
			else {
				pUnpacker->eCurrentState = COMPRESS_UNPACK_STATE_KIND_PROCESS_CTL_BIT;
			}
			break;

		case COMPRESS_UNPACK_STATE_KIND_DONE:
			return COMPRESS_UNPACK_RESULT_DONE;
			break;
	}

	return COMPRESS_UNPACK_RESULT_BUSY;
}

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
		pSubfileData->ulPos = pPakEntry->ulSizeData + lPos;
	}
	pPak->pPrevReadSubfile = 0; // Invalidate cache

	if(pSubfileData->ulPos > pPakEntry->ulSizeData) {
		logWrite("ERR: Seek position %lu out of range %lu for pakFile %hu\n",
			pSubfileData->ulPos, pPakEntry->ulSizeData, pSubfileData->uwFileIndex
		);
		pSubfileData->ulPos = pPakEntry->ulSizeData;
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

	return pSubfileData->ulPos >= pSubfileData->pPak->pEntries[pSubfileData->uwFileIndex].ulSizeData;
}

static void pakSubfileFlush(UNUSED_ARG void *pData) {
	// no-op
}

static void pakCompressedClose(void *pData) {
	tPakFileCompressedData *pCompressedData = (tPakFileCompressedData*)pData;
	fileClose(pCompressedData->pSubfile);
	memFree(pCompressedData, sizeof(*pCompressedData));
}

static ULONG pakCompressedRead(void *pData, void *pDest, ULONG ulSize) {
	tPakFileCompressedData *pCompressedData = (tPakFileCompressedData*)pData;
	tCompressUnpacker *pUnpacker = &pCompressedData->sUnpacker;
	ULONG ulRemaining = ulSize;
	UBYTE *pDestBytes = pDest;

	while(ulRemaining) {
		tCompressUnpackerResult eUnpackResult = compressUnpackerProcess(pUnpacker);
		if(eUnpackResult == COMPRESS_UNPACK_RESULT_BUSY_WROTE_BYTE) {
			*(pDestBytes++) = pUnpacker->ubOut;
			--ulRemaining;
		}
		else if(eUnpackResult == COMPRESS_UNPACK_RESULT_DONE) {
			break;
		}
	}
	return ulSize - ulRemaining;
}

static ULONG pakCompressedWrite(
	UNUSED_ARG void *pData, UNUSED_ARG const void *pSrc, UNUSED_ARG ULONG ulSize
) {
	logWrite("ERR: Unsupported: pakCompressedWrite()\n");
	return 0;
}

static ULONG pakCompressedSeek(void *pData, LONG lPos, WORD wMode) {
	tPakFileCompressedData *pCompressedData = (tPakFileCompressedData*)pData;
	// seek forward: unpack some bytes to void
	// seek backward: restart unpacker and unpack some bytes to void

	ULONG ulPosCurrent = pCompressedData->sUnpacker.ulUnpackedCount;
	ULONG ulPosTarget;
	ULONG ulFileSize = pCompressedData->sUnpacker.ulUncompressedSize;
	if(wMode == SEEK_CUR) {
		ulPosTarget = ulPosCurrent + lPos;
	}
	else if(wMode == SEEK_END) {
		ulPosTarget = ulFileSize + lPos;
	}
	else { // (wMode == SEEK_SET)
		ulPosTarget = lPos;
	}

	if(ulPosTarget > pCompressedData->sUnpacker.ulUncompressedSize) {
		logWrite(
			"ERR: Seek position %lu out of range %lu for compressed pakFile\n",
			ulPosTarget, pCompressedData->sUnpacker.ulUncompressedSize
		);
		return 0;
	}

	if(ulPosTarget == ulFileSize) {
		pCompressedData->sUnpacker.eCurrentState = COMPRESS_UNPACK_STATE_KIND_DONE;
		pCompressedData->sUnpacker.ulUnpackedCount = ulFileSize;
		return 1;
	}

	if(ulPosTarget < ulPosCurrent) {
		logWrite("WARN: Huge performance penalty due to going back in compressed file. Do you *really* need to do it?\n");
		compressUnpackerInit(
			&pCompressedData->sUnpacker,
			pCompressedData->sUnpacker.ulCompressedSize,
			pCompressedData->sUnpacker.ulUncompressedSize,
			pCompressedData->pSubfile->pData
		);
	}

	while(pCompressedData->sUnpacker.ulUnpackedCount < ulPosTarget) {
		UBYTE ubDummy;
		pakCompressedRead(pData, &ubDummy, 1);
	}

	return 1;
}

static ULONG pakCompressedGetPos(void *pData) {
	tPakFileCompressedData *pCompressedData = (tPakFileCompressedData*)pData;
	return pCompressedData->sUnpacker.ulUnpackedCount;
}

static UBYTE pakCompressedIsEof(void *pData) {
	tPakFileCompressedData *pCompressedData = (tPakFileCompressedData*)pData;
	return pCompressedData->sUnpacker.ulUnpackedCount >= pCompressedData->sUnpacker.ulUncompressedSize;
}

static void pakCompressedFlush(UNUSED_ARG void *pData) {
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
		fileRead(pMainFile, &pPakFile->pEntries[i].ulSizeUncompressed, sizeof(pPakFile->pEntries[i].ulSizeUncompressed));
		fileRead(pMainFile, &pPakFile->pEntries[i].ulSizeData, sizeof(pPakFile->pEntries[i].ulSizeData));
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
	UBYTE isCompressed = pPakFile->pEntries[uwFileIndex].ulSizeUncompressed != pPakFile->pEntries[uwFileIndex].ulSizeData;
	logWrite(
		"Subfile index: %hu, offset: %lu, size: %lu, compressed: %hhu\n",
		uwFileIndex,
		pPakFile->pEntries[uwFileIndex].ulOffs,
		pPakFile->pEntries[uwFileIndex].ulSizeUncompressed,
		isCompressed
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

	if(isCompressed) {
		tPakFileCompressedData *pCompressedData = memAllocFast(sizeof(*pCompressedData));
		pCompressedData->pSubfile = pFile;
		compressUnpackerInit(
			&pCompressedData->sUnpacker,
			pPakFile->pEntries[uwFileIndex].ulSizeData,
			pPakFile->pEntries[uwFileIndex].ulSizeUncompressed,
			pCompressedData->pSubfile->pData
		);

		tFile *pCompressedFile = memAllocFast(sizeof(*pCompressedFile));
		pCompressedFile->pCallbacks = &s_sPakCompressedCallbacks;
		pCompressedFile->pData = pCompressedData;
		pFile = pCompressedFile;
	}

	logBlockEnd("pakFileGetFile()");
	return pFile;
}

#endif
