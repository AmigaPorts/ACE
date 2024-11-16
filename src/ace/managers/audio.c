#include <ace/managers/audio.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/utils/custom.h>
#include <ace/utils/disk_file.h>
#include <ace/managers/system.h>

typedef struct _tChannelControls {
	BYTE bPlayCount;
	UBYTE ubChannel;
} tChannelControls;

tChannelControls s_pControls[4];

void INTERRUPT audioIntHandler(
	UNUSED_ARG REGARG(volatile tCustom *pCustom, "a0"),
	UNUSED_ARG REGARG(volatile void *pData, "a1")
) {
	volatile tChannelControls *pCtrl = (volatile tChannelControls *)pData;
	if(pCtrl->bPlayCount != -1) {
		if(pCtrl->bPlayCount) {
			--pCtrl->bPlayCount;
		}
		else {
			audioStop(pCtrl->ubChannel);
		}
	}
}

void audioCreate(void) {
	logBlockBegin("audioCreate()");
	for(UBYTE i = 0; i < 4; ++i) {
		systemSetDmaBit(DMAB_AUD0+i, 0);
		s_pControls[i].bPlayCount = 0;
		s_pControls[i].ubChannel = i;
		systemSetInt(INTB_AUD0+i, audioIntHandler, &s_pControls[i]);
	}
	// Disable audio filter
	g_pCia[CIA_A]->pra ^= BV(1);
	logBlockEnd("audioCreate()");
}

void audioDestroy(void) {
	logBlockBegin("audioDestroy()");
	for(UBYTE i = 0; i < 4; ++i) {
		systemSetDmaBit(DMAB_AUD0+i, 0);
		systemSetInt(INTB_AUD0+i, 0, 0);
	}
	logBlockEnd("audioDestroy()");
}

void audioPlay(
	UBYTE ubChannel, tSample *pSample, UBYTE ubVolume, BYTE bPlayCount
) {
	// Stop playback on given channel
	systemSetDmaBit(ubChannel, 0);

	s_pControls[ubChannel].bPlayCount = bPlayCount;
	volatile struct AudChannel *pChannel = &g_pCustom->aud[ubChannel];
	pChannel->ac_ptr = (UWORD *) pSample->pData;
	pChannel->ac_len = pSample->uwLength >> 1; // word count
	pChannel->ac_vol = ubVolume;
	pChannel->ac_per = pSample->uwPeriod;

	// Now that channel regs are set, start playing
	systemSetDmaBit(ubChannel, 1);
}

void audioStop(UBYTE ubChannel) {
	systemSetDmaBit(ubChannel, 0);
	// Volume to zero for given channel
	g_pCustom->aud[ubChannel].ac_vol = 0;
}

tSample *sampleCreate(UWORD uwLength, UWORD uwPeriod) {
	logBlockBegin("sampleCreate(uwLength: %hu, uwPeriod: %hu)", uwLength, uwPeriod);
	tSample *pSample = memAllocFast(sizeof(tSample));
	pSample->uwLength = uwLength;
	pSample->pData = memAllocChipClear(uwLength);
	pSample->uwPeriod = uwPeriod;
	logBlockEnd("sampleCreate()");
	return pSample;
}

tSample *sampleCreateFromPath(const char *szPath, UWORD uwSampleRateHz)
{
	return sampleCreateFromFd(diskFileOpen(szPath, "rb"), uwSampleRateHz);
}

tSample *sampleCreateFromFd(tFile *pSampleFile, UWORD uwSampleRateHz)
{
	systemUse();
	logBlockBegin(
		"sampleCreateFromFd(pSampleFile: %p, uwSampleRateHz: %hu)",
		pSampleFile, uwSampleRateHz
	);

	if(!pSampleFile) {
		logWrite("ERR: Null file handle\n");
		logBlockEnd("sampleCreateFromFd()");
		return 0;
	}

	LONG lLength = fileGetSize(pSampleFile);
	if(lLength <= 0) {
		logWrite("ERR: File doesn't exist\n");
		fileClose(pSampleFile);
		logBlockEnd("sampleCreateFromFd()");
		systemUnuse();
		return 0;
	}
	// NOTE: 3546895 is for PAL, for NTSC use 3579545
	UWORD uwPeriod = (3546895 + uwSampleRateHz/2) / uwSampleRateHz;
	tSample *pSample = sampleCreate(lLength, uwPeriod);
	fileRead(pSampleFile, pSample->pData, lLength);
	fileClose(pSampleFile);
	logBlockEnd("sampleCreateFromFd()");
	systemUnuse();
	return pSample;
}

void sampleDestroy(tSample *pSample) {
	memFree(pSample->pData, pSample->uwLength);
	memFree(pSample, sizeof(tSample));
}
