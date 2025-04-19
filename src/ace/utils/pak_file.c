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

#define UNPACKER_CTL_BITS 8
#define UNPACKER_RLE_CTL_BYTES 2
#define UNPACKER_RLE_MAX_LENGTH (15 + 3)
#define UNPACKER_PACKED_BUFFER_SIZE (UNPACKER_CTL_BITS * UNPACKER_RLE_CTL_BYTES + (UNPACKER_CTL_BITS / 8))
#define UNPACKER_UNPACKED_BUFFER_SIZE (UNPACKER_CTL_BITS * UNPACKER_RLE_MAX_LENGTH)

typedef struct tCompressUnpacker {
	void *pSubfileData;
	ULONG ulCompressedSize;
	ULONG ulUncompressedSize;
	ULONG ulUnpackedCount;
	UWORD uwLookupPos;

	UBYTE ubUnpackedChunkPos;
	UBYTE ubUnpackedChunkSize;
	UBYTE *pPackedCurrent;
	UBYTE pLookup[0x1000];
	UBYTE pUnpacked[UNPACKER_UNPACKED_BUFFER_SIZE];
	UBYTE pPacked[UNPACKER_PACKED_BUFFER_SIZE];
} tCompressUnpacker;

typedef struct tPakFileSubfileData {
	tPakFile *pPak;
	tPakFileEntry *pEntry;
	ULONG ulPos;
} tPakFileSubfileData;

typedef struct tPakFileCompressedData {
	tCompressUnpacker sUnpacker;
	tFile *pSubfile;
} tPakFileCompressedData;

static void pakSubfileClose(void *pData);
static ULONG pakSubfileRead(void *pData, void *pDest, ULONG ulSize);
static ULONG pakSubfileWrite(UNUSED_ARG void *pData, UNUSED_ARG const void *pSrc, UNUSED_ARG ULONG ulSize);
static ULONG pakSubfileSeek(void *pData, LONG lPos, WORD wMode);
static ULONG pakSubfileGetPos(void *pData);
static ULONG pakSubfileGetSize(void *pData);
static UBYTE pakSubfileIsEof(void *pData);
static void pakSubfileFlush(UNUSED_ARG void *pData);

static void pakCompressedClose(void *pData);
static ULONG pakCompressedRead(void *pData, void *pDest, ULONG ulSize);
static ULONG pakCompressedWrite(UNUSED_ARG void *pData, UNUSED_ARG const void *pSrc, UNUSED_ARG ULONG ulSize);
static ULONG pakCompressedSeek(void *pData, LONG lPos, WORD wMode);
static ULONG pakCompressedGetPos(void *pData);
static ULONG pakCompressedGetSize(void *pData);
static UBYTE pakCompressedIsEof(void *pData);
static void pakCompressedFlush(UNUSED_ARG void *pData);

static const tFileCallbacks s_sPakSubfileCallbacks = {
	.cbFileClose = pakSubfileClose,
	.cbFileRead = pakSubfileRead,
	.cbFileWrite = pakSubfileWrite,
	.cbFileSeek = pakSubfileSeek,
	.cbFileGetPos = pakSubfileGetPos,
	.cbFileGetSize = pakSubfileGetSize,
	.cbFileIsEof = pakSubfileIsEof,
	.cbFileFlush = pakSubfileFlush,
};

static const tFileCallbacks s_sPakCompressedCallbacks = {
	.cbFileClose = pakCompressedClose,
	.cbFileRead = pakCompressedRead,
	.cbFileWrite = pakCompressedWrite,
	.cbFileSeek = pakCompressedSeek,
	.cbFileGetPos = pakCompressedGetPos,
	.cbFileGetSize = pakCompressedGetSize,
	.cbFileIsEof = pakCompressedIsEof,
	.cbFileFlush = pakCompressedFlush,
};

//------------------------------------------------------------------ PRIVATE FNS

void compressUnpackerInit(
	tCompressUnpacker *pUnpacker, ULONG ulCompressedSize, size_t ulUncompressedSize,
	void *pSubfileData
) {
	pUnpacker->ulUnpackedCount = 0;
	pUnpacker->uwLookupPos = 0;
	pUnpacker->ubUnpackedChunkPos = 0;
	pUnpacker->ubUnpackedChunkSize = 0;

	pUnpacker->ulCompressedSize = ulCompressedSize;
	pUnpacker->ulUncompressedSize = ulUncompressedSize;
	pUnpacker->pSubfileData = pSubfileData;
	pUnpacker->pPackedCurrent = &pUnpacker->pPacked[UNPACKER_PACKED_BUFFER_SIZE];
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

static WORD compressUnpackerReadNext(tCompressUnpacker *pUnpacker, UBYTE *pOut, ULONG ulReadSize) {
	UBYTE ubReadSize = MIN((UBYTE)(pUnpacker->ubUnpackedChunkSize - pUnpacker->ubUnpackedChunkPos), ulReadSize);
	UBYTE ubReadPos = pUnpacker->ubUnpackedChunkPos;
	for(UWORD i = 0; i < ubReadSize; ++i) {
		*(pOut++) = pUnpacker->pUnpacked[ubReadPos++];
	}
	if(ubReadSize) {
		pUnpacker->ulUnpackedCount += ubReadSize;
		pUnpacker->ubUnpackedChunkPos = ubReadPos;
		return ubReadSize;
	}

	if(pUnpacker->ulUnpackedCount >= pUnpacker->ulUncompressedSize) {
		return -1;
	}

	// Move unparsed bytes to beginning, fill up the packed buffer
	UBYTE *pPackedCurrent = pUnpacker->pPackedCurrent;
	UBYTE *pDst = &pUnpacker->pPacked[0];
	UBYTE *pEnd = &pUnpacker->pPacked[UNPACKER_PACKED_BUFFER_SIZE];
	while(pPackedCurrent < pEnd) {
		*(pDst++) = *(pPackedCurrent++);
	}

	// Read might return 0 if only unparsed bytes are remaining to process
	pEnd = pDst + pakSubfileRead(pUnpacker->pSubfileData, pDst, pEnd - pDst);

	// Decode next portion of data
	pPackedCurrent = &pUnpacker->pPacked[0];
	UBYTE ubCtl = *(pPackedCurrent++);
	UBYTE ubBits = UNPACKER_CTL_BITS;
	UBYTE *pUnpacked = pUnpacker->pUnpacked;

	while(ubBits--) {
		if(ubCtl & 1) {
			UBYTE ubRawByte = *(pPackedCurrent++);
			rleTableWrite(pUnpacker->pLookup, &pUnpacker->uwLookupPos, ubRawByte);
			*(pUnpacked++) = ubRawByte;
		}
		else {
			UBYTE ubHi = *(pPackedCurrent++);
			UBYTE ubLo = *(pPackedCurrent++);
			ULONG ulRleCtl = (ubHi << 8) | ubLo;

			UWORD uwRleLength = (ulRleCtl & 0xF) + 3;
			UWORD uwRlePos = ulRleCtl >> 4;
			while(uwRleLength--) {
				UBYTE ubRawByte = rleTableRead(pUnpacker->pLookup, &uwRlePos);
				rleTableWrite(pUnpacker->pLookup, &pUnpacker->uwLookupPos, ubRawByte);
				*(pUnpacked++) = ubRawByte;
			}
		}
		if(pPackedCurrent == pEnd) {
			// TODO: optimize packing so that there are always 8 bits to process
			// at the end - would allow removing this cmp.
			// Might be impossible for some files? E.g. one with no RLE sequences -
			// perhaps don't ever use compression for them.
			break;
		}
		ubCtl >>= 1;
	}
	pUnpacker->ubUnpackedChunkSize = pUnpacked - pUnpacker->pUnpacked;
	pUnpacker->pPackedCurrent = pPackedCurrent;
	pUnpacker->ubUnpackedChunkPos = 0;
	return 0;
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

static void pakSubfileClose(void *pData) {
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
			pSubfileData->pEntry->ulOffs + pSubfileData->ulPos,
			FILE_SEEK_SET
		);
		pPak->pPrevReadSubfile = pSubfileData;
	}

	// Enforce upper bound of the file size
	ULONG ulRemaining = pSubfileData->pEntry->ulSizeData - pSubfileData->ulPos;
	if(ulRemaining < ulSize) {
		ulSize = ulRemaining;
		if(!ulSize) {
			return 0;
		}
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
	ULONG ulSizeData = pSubfileData->pEntry->ulSizeData;

	if(wMode == FILE_SEEK_SET) {
		pSubfileData->ulPos = lPos;
	}
	else if(wMode == FILE_SEEK_CURRENT) {
		pSubfileData->ulPos += lPos;
	}
	else if(wMode == FILE_SEEK_END) {
		pSubfileData->ulPos = ulSizeData + lPos;
	}
	pSubfileData->pPak->pPrevReadSubfile = 0; // Invalidate cache

	if(pSubfileData->ulPos > ulSizeData) {
		logWrite("ERR: Seek position %lu out of range %lu for pakFile data %p\n",
			pSubfileData->ulPos, ulSizeData, pSubfileData
		);
		pSubfileData->ulPos = ulSizeData;
		return 0;
	}
	return 1;
}

static ULONG pakSubfileGetPos(void *pData) {
	tPakFileSubfileData *pSubfileData = (tPakFileSubfileData*)pData;

	return pSubfileData->ulPos;
}

static ULONG pakSubfileGetSize(void *pData) {
	tPakFileSubfileData *pSubfileData = (tPakFileSubfileData*)pData;

	return pSubfileData->pEntry->ulSizeData;
}

static UBYTE pakSubfileIsEof(void *pData) {
	tPakFileSubfileData *pSubfileData = (tPakFileSubfileData*)pData;

	return pSubfileData->ulPos >= pakSubfileGetSize(pData);
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
	UBYTE *pDestByte = pDest;

	while(ulRemaining) {
		WORD wRead = compressUnpackerReadNext(pUnpacker, pDestByte, ulRemaining);
		if(wRead < 0) {
			break;
		}
		pDestByte += wRead;
		ulRemaining -= wRead;
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

static ULONG pakCompressedGetSize(void *pData) {
	tPakFileCompressedData *pCompressedData = (tPakFileCompressedData*)pData;
	return pCompressedData->sUnpacker.ulUncompressedSize;
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

tPakFile *pakFileOpen(const char *szPath, UBYTE isUninterrupted) {
	logBlockBegin("pakFileOpen(szPath: '%s')", szPath);
	tFile *pMainFile = diskFileOpen(szPath, DISK_FILE_MODE_READ, isUninterrupted);
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
	pSubfileData->pEntry = &pPakFile->pEntries[uwFileIndex];
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
