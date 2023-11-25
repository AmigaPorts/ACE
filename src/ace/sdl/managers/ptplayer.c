/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/ptplayer.h>
#include <SDL.h>

#define AUDIO_CHANNEL_COUNT 4
#define AUDIO_MAX_VOLUME 64
#define AUDIO_TARGET_RATE 44100
#define AUDIO_SOURCE_FORMAT AUDIO_S8
#define AUDIO_TARGET_FORMAT AUDIO_S16
#define AUDIO_SOURCE_SAMPLE_SIZE 1
#define AUDIO_TARGET_SAMPLE_SIZE 2

typedef struct tChannelState {
	const tPtplayerSfx *pSfx;
	SDL_AudioStream *pStream;
	ULONG ulPos;
	UBYTE isActive;
	UBYTE isLooped;
	UBYTE ubPriority;
	UBYTE ubVolume; // 0..64
} tChannelState;

static tChannelState s_pChannelStates[AUDIO_CHANNEL_COUNT];

static SDLCALL void onMoreAudioData(
	void *pUserData, UBYTE *pStreamBytes, int lByteFillLength
) {
	ULONG ulRequiredSampleCount = lByteFillLength / AUDIO_TARGET_SAMPLE_SIZE;
	WORD *pOutputSamples = (WORD*)pStreamBytes;
	memset(pOutputSamples, 0, lByteFillLength);

	for(UBYTE ubChannelIndex = 0; ubChannelIndex < AUDIO_CHANNEL_COUNT; ++ubChannelIndex) {
		tChannelState *pChannelState = &s_pChannelStates[ubChannelIndex];
		if(!pChannelState->isActive) {
			continue;
		}

		ULONG ulAvailSamples = SDL_AudioStreamAvailable(pChannelState->pStream) / AUDIO_TARGET_SAMPLE_SIZE;
		if(pChannelState->isLooped) {
			while(ulAvailSamples < ulRequiredSampleCount) {
				SDL_AudioStreamPut(
					pChannelState->pStream, pChannelState->pSfx->pData,
					pChannelState->pSfx->uwWordLength * pChannelState->pSfx->pData[0]
				);
				ulAvailSamples = SDL_AudioStreamAvailable(pChannelState->pStream) / AUDIO_TARGET_SAMPLE_SIZE;
			}
		}
		else {
			if(ulAvailSamples < ulRequiredSampleCount) {
				SDL_AudioStreamFlush(pChannelState->pStream);
				ulAvailSamples = SDL_AudioStreamAvailable(pChannelState->pStream) / AUDIO_TARGET_SAMPLE_SIZE;
			}
		}
		ulAvailSamples = MIN(ulAvailSamples, ulRequiredSampleCount);

		WORD pResampledChannel[ulAvailSamples];
		SDL_AudioStreamGet(pChannelState->pStream, pResampledChannel, ulAvailSamples * AUDIO_TARGET_SAMPLE_SIZE);

		for(ULONG ulSampleIndex = 0; ulSampleIndex < ulAvailSamples; ++ulSampleIndex) {
			pOutputSamples[ulSampleIndex] += (pResampledChannel[ulSampleIndex] * pChannelState->ubVolume) / (AUDIO_MAX_VOLUME * AUDIO_CHANNEL_COUNT);
		}

		if(!pChannelState->isLooped && SDL_AudioStreamAvailable(pChannelState->pStream) == 0) {
			ptplayerSfxStopOnChannel(ubChannelIndex);
		}
	}
}

void ptplayerCreate(void) {
	for(UBYTE i = 0; i < 4; ++i) {
		s_pChannelStates[i].isActive = 0;
		s_pChannelStates[i].pStream = 0;
	}

	SDL_AudioSpec sSpec = {
		.callback = onMoreAudioData,
		.channels = 1,
		.format = AUDIO_TARGET_FORMAT,
		.freq = AUDIO_TARGET_RATE,
		.samples = 1024,
		.userdata = 0
	};

	if(SDL_OpenAudio(&sSpec, 0) < 0) {
		logWrite("ERR: Opening audio failed");
	}

	SDL_PauseAudio(0);
}

void ptplayerDestroy(void) {
	SDL_CloseAudio();
}

void ptplayerProcess(void) {

}

void ptplayerLoadMod(
	tPtplayerMod *pMod, tPtplayerSamplePack *pSamples, UWORD uwInitialSongPos
) {

}

UBYTE ptplayerModIsCurrent(const tPtplayerMod *pMod) {
	return 0;
}

void ptplayerStop(void) {

}

void ptplayerSetMusicChannelMask(UBYTE ubChannelMask) {

}

void ptplayerSetMasterVolume(UBYTE ubMasterVolume) {

}

void ptplayerSetChannelsForPlayer(UBYTE ubChannelMask) {

}

void ptplayerEnableMusic(UBYTE isEnabled) {

}

UBYTE ptplayerGetE8(void) {
	return 0;
}

void ptplayerReserveChannelsForMusic(UBYTE ubChannelCount) {

}

void ptplayerSetSampleVolume(UBYTE ubSampleIndex, UBYTE ubVolume) {

}

void ptplayerSfxStopOnChannel(UBYTE ubChannel) {
	tChannelState *pChannelState = &s_pChannelStates[ubChannel];
	pChannelState->isActive = 0;
	if(pChannelState->pStream) {
		SDL_FreeAudioStream(pChannelState->pStream);
		pChannelState->pStream = 0;
	}
}

void ptplayerConfigureSongRepeat(UBYTE isRepeat, tPtplayerCbSongEnd cbSongEnd) {

}

void ptplayerWaitForSfx(void) {
	UBYTE isAnyPlaying = 0;
	do {
		isAnyPlaying = 0;
		for(UBYTE ubChannelIndex = 0; ubChannelIndex < AUDIO_CHANNEL_COUNT; ++ubChannelIndex) {
			if(s_pChannelStates[ubChannelIndex].isActive) {
				isAnyPlaying = 1;
				SDL_Delay(1);
				break;
			}
		}
	} while(isAnyPlaying);
}

tPtplayerSamplePack *ptplayerSamplePackCreate(const char *szPath) {
	return 0;
}

void ptplayerSamplePackDestroy(tPtplayerSamplePack *pSamplePack) {

}

//-------------------------------------------------------------------------- SFX

void muteChannelsPlayingSfx(const tPtplayerSfx *pSfx) {
	for(UBYTE ubChannelIndex = 0; ubChannelIndex < AUDIO_CHANNEL_COUNT; ++ubChannelIndex) {
		if(s_pChannelStates[ubChannelIndex].isActive && s_pChannelStates[ubChannelIndex].pSfx == pSfx) {
			ptplayerSfxStopOnChannel(ubChannelIndex);
		}
	}
}

void ptplayerSfxPlay(
	const tPtplayerSfx *pSfx, UBYTE ubChannel, UBYTE ubVolume, UBYTE ubPriority
) {
	if(ubChannel == 255) {
		for(UBYTE ubChannelIndex = 0; ubChannelIndex < AUDIO_CHANNEL_COUNT; ++ubChannelIndex) {
			if(
				!s_pChannelStates[ubChannel].isActive
			) {
				ubChannel = ubChannelIndex;
				break;
			}
		}
	}

	if(ubChannel == 255) {
		for(UBYTE ubChannelIndex = 0; ubChannelIndex < AUDIO_CHANNEL_COUNT; ++ubChannelIndex) {
			if(
				ubPriority >= s_pChannelStates[ubChannel].ubPriority
			) {
				ubChannel = ubChannelIndex;
				break;
			}
		}
	}

	tChannelState *pChannelState = &s_pChannelStates[ubChannel];
	if(!pChannelState->isActive || ubPriority >= pChannelState->ubPriority) {
		ptplayerSfxStopOnChannel(ubChannel); // prevent callback interference

		// Determine source frequency
		// aminet.net/aminet/docs/lists/amiga_sample_periods.txt
		ULONG ulSourceRate = 7093789.2f / (2 * pSfx->uwPeriod);

		pChannelState->pSfx = pSfx;
		pChannelState->ulPos = 0;
		pChannelState->ubPriority = ubPriority;
		pChannelState->isLooped = 0;
		pChannelState->ubVolume = ubVolume;
		pChannelState->pStream = SDL_NewAudioStream(AUDIO_SOURCE_FORMAT, 1, ulSourceRate, AUDIO_TARGET_FORMAT, 1, AUDIO_TARGET_RATE);
		SDL_AudioStreamPut(pChannelState->pStream, pSfx->pData, pSfx->uwWordLength * sizeof(pSfx->pData[0]));
		pChannelState->isActive = 1;
	}
}

void ptplayerSfxPlayLooped(
	const tPtplayerSfx *pSfx, UBYTE ubChannel, UBYTE ubVolume
) {
	tChannelState *pChannelState = &s_pChannelStates[ubChannel];
	ptplayerSfxStopOnChannel(ubChannel); // prevent callback interference

	// Determine source frequency
	// aminet.net/aminet/docs/lists/amiga_sample_periods.txt
	ULONG ulSourceRate = 7093789.2f / (2 * pSfx->uwPeriod);

	pChannelState->pSfx = pSfx;
	pChannelState->ulPos = 0;
	pChannelState->ubPriority = 0xFF;
	pChannelState->isLooped = 1;
	pChannelState->ubVolume = ubVolume;
	pChannelState->pStream = SDL_NewAudioStream(AUDIO_SOURCE_FORMAT, 1, ulSourceRate, AUDIO_S16, 1, AUDIO_TARGET_RATE);
	SDL_AudioStreamPut(pChannelState->pStream, pSfx->pData, pSfx->uwWordLength * sizeof(pSfx->pData[0]));
	pChannelState->isActive = 1;
}
