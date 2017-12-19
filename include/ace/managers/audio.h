#ifndef GUARD_ACE_MANAGERS_AUDIO_H
#define GUARD_ACE_MANAGERS_AUDIO_H

#include <ace/types.h>

#define AUDIO_CHANNEL_0 0
#define AUDIO_CHANNEL_1 1
#define AUDIO_CHANNEL_2 2
#define AUDIO_CHANNEL_3 3

#define SAMPLE_FLAG_16BIT 1

typedef struct _tSample {
	UWORD uwLength;
	UWORD uwPeriod;
	UBYTE ubFlags;
	UBYTE *pData;
} tSample;

typedef struct _tAudioManager {
	UBYTE a;
} tAudioManager;

extern tAudioManager g_sAudioManager;

void audioCreate(void);

void audioDestroy(void);

void audioPlay(
	IN UBYTE ubChannel,
	IN tSample *pSample,
	IN UBYTE ubVolume
);

tSample *sampleCreate(
	IN UWORD uwLength,
	IN UWORD uwPeriod
);

void sampleDestroy(
	IN tSample *pSample
);

#endif // GUARD_ACE_MANAGERS_AUDIO_H
