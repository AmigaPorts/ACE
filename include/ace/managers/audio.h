#ifndef GUARD_ACE_MANAGERS_AUDIO_H
#define GUARD_ACE_MANAGERS_AUDIO_H

#include <ace/types.h>
#include <hardware/dmabits.h> // DMAF_* flags

#define AUDIO_CHANNEL_0 DMAB_AUD0
#define AUDIO_CHANNEL_1 DMAB_AUD1
#define AUDIO_CHANNEL_2 DMAB_AUD2
#define AUDIO_CHANNEL_3 DMAB_AUD3

#define SAMPLE_FLAG_16BIT 1

typedef struct _tSample {
	UWORD uwLength;
	UWORD uwPeriod;
	UBYTE ubFlags;
	UBYTE *pData;
} tSample;

void audioCreate(void);

void audioDestroy(void);

void audioPlay(
	UBYTE ubChannel, tSample *pSample, UBYTE ubVolume, BYTE bPlayCount
);

void audioStop(UBYTE ubChannel);

tSample *sampleCreate(UWORD uwLength, UWORD uwPeriod);

void sampleDestroy(tSample *pSample);

#endif // GUARD_ACE_MANAGERS_AUDIO_H
