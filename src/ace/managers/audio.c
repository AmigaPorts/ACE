#include <ace/managers/audio.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/utils/custom.h>
#include <ace/managers/system.h>

typedef struct _tChannelControls {
	BYTE bPlayCount;
	UBYTE ubChannel;
} tChannelControls;

tChannelControls s_pControls[4];

void INTERRUPT audioIntHandler(
	REGARG(volatile tCustom *pCustom, "a0"),
	REGARG(volatile void *pData, "a1")
) {
	volatile tChannelControls *pCtrl = (volatile tChannelControls *)pData;
	if(pCtrl->bPlayCount != -1) {
		if(pCtrl->bPlayCount) {
			--pCtrl->bPlayCount;
		}
		else {
			UBYTE ubChannel = pCtrl->ubChannel;
			pCustom->aud[ubChannel].ac_len = 0;
			pCustom->aud[ubChannel].ac_per = 0;
			pCustom->aud[ubChannel].ac_vol = 0;
		}
	}
}

void audioCreate(void) {
	logBlockBegin("audioCreate()");
	for(UBYTE i = 0; i < 4; ++i) {
		systemSetDma(DMAB_AUD0+i, 0);
		s_pControls[i].bPlayCount = 0;
		s_pControls[i].ubChannel = i;
		systemSetInt(INTB_AUD0+i, audioIntHandler, &s_pControls[i]);
	}
	logBlockEnd("audioCreate()");
}

void audioDestroy(void) {
	// logBlockBegin("audioDestroy()");
	systemSetDma(DMAB_AUD0, 0);
	systemSetDma(DMAB_AUD1, 0);
	systemSetDma(DMAB_AUD2, 0);
	systemSetDma(DMAB_AUD3, 0);
	// logBlockEnd("audioDestroy()");
}

void audioPlay(
	UBYTE ubChannel, tSample *pSample, UBYTE ubVolume, BYTE bPlayCount
) {
	// Stop playback on given channel
	systemSetDma(ubChannel, 0);

	s_pControls[ubChannel].bPlayCount = bPlayCount;
	volatile struct AudChannel *pChannel = &g_pCustom->aud[ubChannel];
	pChannel->ac_ptr = (UWORD *) pSample->pData;
	pChannel->ac_len = pSample->uwLength >> 1; // word count
	pChannel->ac_vol = ubVolume;
	pChannel->ac_per = pSample->uwPeriod;

	// Now that channel regs are set, start playing
	systemSetDma(ubChannel, 1);
}

void audioStop(UBYTE ubChannel) {
	systemSetDma(ubChannel, 0);
}

tSample *sampleCreate(UWORD uwLength, UWORD uwPeriod) {
	tSample *pSample = memAllocFast(sizeof(tSample));
	pSample->uwLength = uwLength;
	pSample->pData = memAllocChipClear(uwLength);
	pSample->uwPeriod = uwPeriod;
	return pSample;
}

void sampleDestroy(tSample *pSample) {
	memFree(pSample->pData, pSample->uwLength);
	memFree(pSample, sizeof(tSample));
}
