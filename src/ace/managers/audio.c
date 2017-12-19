#include <ace/managers/audio.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/utils/custom.h>
#include <hardware/dmabits.h> // DMAF_* flags

void audioCreate(void) {
	logBlockBegin("audioCreate()");
	custom.dmacon = DMAF_AUDIO;
	logBlockEnd("audioCreate()");
}

void audioDestroy(void) {
	logBlockBegin("audioDestroy()");
	custom.dmacon = DMAF_AUDIO;
	logBlockEnd("audioDestroy()");
}

void audioPlay(UBYTE ubChannel, tSample *pSample, UBYTE ubVolume) {
	// Stop playback on given channel
	custom.dmacon = BV(ubChannel);

	struct AudChannel *pChannel = &custom.aud[ubChannel];
	pChannel->ac_ptr = (UWORD *) pSample->pData;
	pChannel->ac_len = pSample->uwLength >> 1; // word count
	pChannel->ac_vol = ubVolume;
	pChannel->ac_per = pSample->uwPeriod;

	// Now that channel regs are set, start playing
	custom.dmacon = DMAF_SETCLR | BV(ubChannel);
}

void audioStop(UBYTE ubChannel) {
	custom.dmacon = BV(ubChannel);
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
