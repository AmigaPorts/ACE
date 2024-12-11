#ifndef _ACE_MANAGERS_AUDIO_H_
#define _ACE_MANAGERS_AUDIO_H_

/**
 * @file audio.h
 * @brief Basic audio manager. Allows for playback of single and repeated sounds.
 *
 * @warning This module is deprecated and most probably will be removed.
 * Use ptplayer module if possible.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>
#include <hardware/dmabits.h> // DMAF_* flags
#include <ace/utils/file.h>

#define AUDIO_CHANNEL_0 DMAB_AUD0
#define AUDIO_CHANNEL_1 DMAB_AUD1
#define AUDIO_CHANNEL_2 DMAB_AUD2
#define AUDIO_CHANNEL_3 DMAB_AUD3

#define AUDIO_VOLUME_MAX 64
#define AUDIO_REPLAY_CONTINUOUS -1

#define SAMPLE_FLAG_16BIT 1

typedef struct _tSample {
	UWORD uwLength;
	UWORD uwPeriod;
	UBYTE ubFlags;
	UBYTE *pData;
} tSample;

/**
 * @brief Initializes audio manager.
 */
void audioCreate(void);

/**
 * @brief Does cleanup after audio manager.
 */
void audioDestroy(void);

/**
 * @brief Plays given sample at specified volume on given chanel.
 *
 * @param ubChannel Index of channel to be used (0-3).
 * @param pSample Sample to be played.
 * @param ubVolume Volume to be used for playback. See AUDIO_VOLUME_MAX define.
 * @param bPlayCount Number of times a sample should be played.
 *                   Set to AUDIO_REPLAY_CONTINUOUS for playing until
 *                   manual stop.
 */
void audioPlay(
	UBYTE ubChannel, tSample *pSample, UBYTE ubVolume, BYTE bPlayCount
);

/**
 * @brief Stops playback on given channel.
 *
 * @param ubChannel Index of channel to be silenced (0-3).
 */
void audioStop(UBYTE ubChannel);

/**
 * @brief Creates sample of given length with specified period.
 *
 * @param uwLength Number of amplitude values stored in sample.
 * @param uwPeriod Period of sample.
 * @return Pointer to newly allocated sample, 0 on fail.
 */
tSample *sampleCreate(UWORD uwLength, UWORD uwPeriod);

/**
 * @brief Creates sample from file, assigning to it specified playback rate.
 *
 * @param szPath Path to sample file.
 * @param uwSampleRateHz Playback rate to be assigned to sample, in Hz.
 * @return tSample* Pointer to newly allocated sample, 0 on fail.
 */
tSample *sampleCreateFromPath(const char *szPath, UWORD uwSampleRateHz);

/**
 * @brief Creates sample from file handle, assigning to it specified playback rate.
 *
 * @param pSampleFile Handle to sample file. Will be closed on function return.
 * @param uwSampleRateHz Playback rate to be assigned to sample, in Hz.
 * @return tSample* Pointer to newly allocated sample, 0 on fail.
 */
tSample *sampleCreateFromFd(tFile *pSampleFile, UWORD uwSampleRateHz);

/**
 * @brief Destroys sample, freeing all associated memory.
 *
 * @param pSample Pointer to sample to be freed.
 */
void sampleDestroy(tSample *pSample);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_AUDIO_H_
