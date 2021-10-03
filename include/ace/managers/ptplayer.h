/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Protracker V2.3B Playroutine Version 5.1
// Written by Frank Wille in 2013, 2016, 2017, 2018.
// Rewritten to C by KaiN for ACE

#ifndef _ACE_MANAGERS_PTPLAYER_H_
#define _ACE_MANAGERS_PTPLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PTPLAYER_VOLUME_MAX 64

#include <ace/types.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/font.h>

#if defined(ACE_DEBUG_ALL) && !defined(ACE_DEBUG_PTPLAYER)
#define ACE_DEBUG_PTPLAYER
#endif

typedef struct _tPtplayerSfx {
	UWORD *pData;       ///< Sample start in Chip RAM, even address.
	UWORD uwWordLength; ///< Sample length in words.
	UWORD uwPeriod;     ///< Hardware replay period for sample.
} tPtplayerSfx;

typedef struct _tPtplayerMod {
	UBYTE *pData;
	ULONG ulSize;
	// TODO: move some vars from ptplayer here
} tPtplayerMod;

typedef void (*tPtplayerCbSongEnd)(void);

/**
 * @brief Install a CIA-B interrupt for calling _mt_music or mt_sfxonly.
 * The music module is replayed via _mt_music when _mt_Enable is non-zero.
 * Otherwise the interrupt handler calls mt_sfxonly to play sound effects only.
 *
 * @param isPal In CIA mode, Set to 1 on PAL configs, zero on NTSC.
 */
void ptplayerCreate(UBYTE isPal);

void ptplayerDestroy(void);

void ptplayerProcess(void);

/**
 * @brief Loads new MOD from file.
 *
 * @param szPath Path to .mod file.
 * @return Pointer to new MOD structure, initialized with file contents.
 */
tPtplayerMod *ptplayerModCreate(const char *szPath);

/**
 * @brief Frees given MOD structure from memory.
 *
 * @param pMod MOD struct to be destroyed.
 */
void ptplayerModDestroy(tPtplayerMod *pMod);

/**
 * @brief Initialize a new module but doesn't automatically play it.
 * You still need to call ptplayerEnableMusic().
 * Reset speed to 6, tempo to 125 and start at the given position.
 * Master volume is at 64 (maximum).
 *
 * @param pMod Pointer to MOD struct.
 * @param pSamples When set to 0, the samples are assumed to be stored inside
 *                 the MOD, after the patterns.
 * @param uwInitialSongPos
 *
 * @see ptplayerEnableMusic()
 * @see ptplayerStop()
 */
void ptplayerLoadMod(
	tPtplayerMod *pMod, UWORD *pSampleData, UWORD uwInitialSongPos
);

/**
 * @brief Stop playing current module.
 * After calling this function, you need to call ptplayerLoadMod() again.
 *
 * @see ptplayerLoadMod()
 */
void ptplayerStop(void);

/**
 * @brief Set bits in the mask define which specific channels are reserved
 *        for music only.
 *
 * When calling ptplayerSfxPlay with automatic channel selection
 * (sfx_cha=-1) then these masked channels will never be picked.
 * The mask defaults to 0.
 *
 * @param ubChannelMask Mask of channels reserved for playing music. Set bit 0
 *                      for channel 0, bit 1 for channel 1, etc.
 * @see ptplayerSfxPlay()
 */
void ptplayerSetMusicChannelMask(UBYTE ubChannelMask);

/**
 * @brief Set a master volume for all music channels.
 * Note that the master volume does not affect the volume of external
 * sound effects (which is desired).
 *
 * @param ubMasterVolume Value between 0 and 64. Bigger is louder.
 */
void ptplayerSetMasterVolume(UBYTE ubMasterVolume);

/**
 * @brief Enables or disables music playback.
 * You can still play sound effects, while music is stopped.
 * Music is by default disabled after ptplayerInit().
 *
 * @param isEnabled Set to 1 to enable music playback, zero to disable.
 */
void ptplayerEnableMusic(UBYTE isEnabled);

/**
 * @brief Gets the value of the last E8 command.
 * It is reset to 0 after ptplayerInit().
 *
 * @return UBYTE
 */
UBYTE ptplayerGetE8(void);

/**
 * @brief Define the number of channels which should be dedicated only for music.
 * If set to zero, sound effects can take over any channel if they have high
 * enough priority. When set to 4, no sound effects are played.
 *
 * At initialization, zero channels are reserved for music.
 *
 * @param ubChannelCount Number of channels to be used only for playing music.
 */
void ptplayerReserveChannelsForMusic(UBYTE ubChannelCount);

//-------------------------------------------------------------------------- SFX

tPtplayerSfx *ptplayerSfxCreateFromFile(const char *szPath);

void ptplayerSfxDestroy(tPtplayerSfx *pSfx);

/**
 * @brief Request playing of a prioritized external sound effect, either on a
 *        fixed channel or on the most unused one.
 *
 * When multiple samples are assigned to the same channel the lower priority
 * sample will be replaced. When priorities are the same, then the older sample
 * is replaced.
 * The chosen channel is blocked for music until the effect has completely
 * been replayed.
 *
 * @param pSfx Sfx sample to be played.
 * @param bChannel Selected replay channel (0..3), -1 selects best channel.
 * @param ubVolume Playback volume 0..64, unaffected by the song's master volume.
 * @param ubPriority Playback priority. The bigger the value, the higher
 *                   the priority. Must be non-zero.
 *
 * @see ptplayerSetMusicChannelMask
 */
void ptplayerSfxPlay(
	const tPtplayerSfx *pSfx, BYTE bChannel, UBYTE ubVolume, UBYTE ubPriority
);

/**
 * @brief Configure behavior on song being played to the end.
 * By default the player loops the song indefinitely.
 *
 * If you want to repeat the song given number of times, disable repeats
 * and put additional logic in cbSongEnd.
 *
 * @param isRepeat Set to 1 for endless repeat, 0 makes the song play only once.
 * @param cbSongEnd Pointer to song end callback - set to zero if not needed.
 */
void ptplayerConfigureSongRepeat(UBYTE isRepeat, tPtplayerCbSongEnd cbSongEnd);

/**
 * @brief Waits until all sound effects have been played.
 */
void ptplayerWaitForSfx(void);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_PTPLAYER_H_
