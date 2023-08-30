/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/ptplayer.h>
#include <ace/managers/ptplayer_private.h>
#include <ace/managers/system.h>
#include <ace/utils/file.h>

tPtplayerMod *ptplayerModCreate(const char *szPath) {
	logBlockBegin("ptplayerModCreate(szPath: '%s')", szPath);

	tPtplayerMod *pMod = 0;
	LONG lSize = fileGetSize(szPath);
	if(lSize == -1) {
		logWrite("ERR: File doesn't exist!\n");
		return 0;
	}

	pMod = memAllocFastClear(sizeof(*pMod));
	if(!pMod) {
		return 0;
	}

	// Read header
	tFile *pFileMod = fileOpen(szPath, "rb");
	fileReadBytes(pFileMod, (UBYTE*)pMod->szSongName, sizeof(pMod->szSongName));
	for(UBYTE i = 0; i < PTPLAYER_SAMPLE_HEADER_COUNT; ++i) {
		fileReadWords(pFileMod, &pMod->pSampleHeaders[i].uwLength, 1);
		fileReadBytes(pFileMod, &pMod->pSampleHeaders[i].ubFineTune, 1);
		fileReadBytes(pFileMod, &pMod->pSampleHeaders[i].ubVolume, 1);
		fileReadWords(pFileMod, &pMod->pSampleHeaders[i].uwRepeatOffs, 1);
		fileReadWords(pFileMod, &pMod->pSampleHeaders[i].uwRepeatLength, 1);
	}
	fileReadBytes(pFileMod, &pMod->ubArrangementLength, 1);
	fileReadBytes(pFileMod, &pMod->ubSongEndPos, 1);
	fileReadBytes(pFileMod, pMod->pArrangement, sizeof(pMod->pArrangement));
	fileReadBytes(pFileMod, (UBYTE*)pMod->pFileFormatTag, sizeof(pMod->pFileFormatTag));

	// Get number of highest pattern
	UBYTE ubLastPattern = 0;
	for(UBYTE i = 0; i < 127; ++i) {
		if(pMod->pArrangement[i] > ubLastPattern) {
			ubLastPattern = pMod->pArrangement[i];
		}
	}
	UBYTE ubPatternCount = ubLastPattern + 1;
	logWrite("Pattern count: %hhu\n", ubPatternCount);

	// Read pattern data
	pMod->ulPatternsSize = (ubPatternCount * MOD_PATTERN_LENGTH);
	pMod->pPatterns = memAllocFast(pMod->ulPatternsSize);
	if(!pMod->pPatterns) {
		logWrite("ERR: Couldn't allocate memory for pattern data!");
		goto fail;
	}
	fileReadLongs(pFileMod, (ULONG*)pMod->pPatterns, pMod->ulPatternsSize / MOD_BYTES_PER_NOTE);

	// Read sample data
	pMod->ulSamplesSize = lSize - fileGetPos(pFileMod);
	if(pMod->ulSamplesSize) {
		pMod->pSamples = memAllocChip(pMod->ulSamplesSize);
		if(!pMod->pSamples) {
			logWrite("ERR: Couldn't allocate memory for sample data!");
			goto fail;
		}
		fileReadWords(pFileMod, pMod->pSamples, pMod->ulSamplesSize / sizeof(UWORD));
	}
	else {
		logWrite("MOD has no samples - be sure to pass sample pack to ptplayer!\n");
	}

	fileClose(pFileMod);
	logBlockEnd("ptplayerModCreate()");
	return pMod;
fail:
	if(pMod->pPatterns) {
		memFree(pMod->pPatterns, pMod->ulPatternsSize);
	}
	if(pMod->pSamples) {
		memFree(pMod->pSamples, pMod->ulSamplesSize);
	}
	if(pMod) {
		memFree(pMod, sizeof(*pMod));
	}
	return 0;
}

void ptplayerModDestroy(tPtplayerMod *pMod) {
	if(ptplayerModIsCurrent(pMod)) {
		ptplayerStop();
	}
	memFree(pMod->pPatterns, pMod->ulPatternsSize);
	if(pMod->pSamples) {
		memFree(pMod->pSamples, pMod->ulSamplesSize);
	}
	memFree(pMod, sizeof(*pMod));
}

//-------------------------------------------------------------------------- SFX

tPtplayerSfx *ptplayerSfxCreateFromFile(const char *szPath, UBYTE isFast) {
	systemUse();
	logBlockBegin("ptplayerSfxCreateFromFile(szPath: '%s', isFast: %hhu)", szPath, isFast);
	tFile *pFileSfx = fileOpen(szPath, "rb");
	tPtplayerSfx *pSfx = 0;
	if(!pFileSfx) {
		logWrite("ERR: File doesn't exist: '%s'\n", szPath);
		goto fail;
	}

	pSfx = memAllocFastClear(sizeof(*pSfx));
	if(!pSfx) {
		goto fail;
	}
	UBYTE ubVersion;
	fileReadBytes(pFileSfx, &ubVersion, 1);
	if(ubVersion == 1) {
		fileReadWords(pFileSfx, &pSfx->uwWordLength, 1);
		ULONG ulByteSize = pSfx->uwWordLength * sizeof(UWORD);

		UWORD uwSampleRateHz;
		fileReadWords(pFileSfx, &uwSampleRateHz, 1);
		pSfx->uwPeriod = (getClockConstant() + uwSampleRateHz/2) / uwSampleRateHz;
		logWrite("Length: %lu, sample rate: %hu, period: %hu\n", ulByteSize, uwSampleRateHz, pSfx->uwPeriod);

		pSfx->pData = isFast ? memAllocFast(ulByteSize) : memAllocChip(ulByteSize);
		if(!pSfx->pData) {
			goto fail;
		}
		fileReadWords(pFileSfx, pSfx->pData, pSfx->uwWordLength);

		// Check if pData[0] is zeroed-out - it should be because after sfx playback
		// ptplayer sets the channel playback to looped first word. This should
		// be done on sfx converter side. If your samples are humming after playback,
		// fix your custom conversion tool or use latest ACE tools!
		if(pSfx->pData[0] != 0) {
			logWrite("ERR: SFX's first word isn't zeroed-out\n");
		}
	}
	else {
		logWrite("ERR: Unknown sample format version: %hhu\n", ubVersion);
		goto fail;
	}

	fileClose(pFileSfx);
	logBlockEnd("ptplayerSfxCreateFromFile()");
	systemUnuse();
	return pSfx;

fail:
	if(pFileSfx) {
		fileClose(pFileSfx);
	}
	ptplayerSfxDestroy(pSfx);
	logBlockEnd("ptplayerSfxCreateFromFile()");
	systemUnuse();
	return 0;
}

void ptplayerSfxDestroy(tPtplayerSfx *pSfx) {
	logBlockBegin("ptplayerSfxDestroy(pSfx: %p)", pSfx);
	if(pSfx) {
		muteChannelsPlayingSfx(pSfx);

		systemUse();
		if(pSfx->pData) {
			memFree(pSfx->pData, pSfx->uwWordLength * sizeof(UWORD));
		}
		memFree(pSfx, sizeof(*pSfx));
		systemUnuse();
	}
	logBlockEnd("ptplayerSfxDestroy()");
}

UBYTE ptplayerSfxLengthInFrames(const tPtplayerSfx *pSfx) {
	// Get rounded sampling rate.
	UWORD uwSamplingRateHz = (getClockConstant() + (pSfx->uwPeriod / 2)) / pSfx->uwPeriod;
	// Get frame count - round it up.
	UWORD uwFrameCount = (pSfx->uwWordLength * 2 * 50 + uwSamplingRateHz - 1) / uwSamplingRateHz;
	return uwFrameCount;
}
