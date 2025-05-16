/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Protracker V2.3B Playroutine Version 6.3
// Original written in assembly by Frank Wille in 2013, 2016-2023.
// Rewritten for ACE usage to C.

#ifndef _ACE_MANAGERS_PTPLAYER_H_
#define _ACE_MANAGERS_PTPLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PTPLAYER_VOLUME_MAX 64
#define PTPLAYER_SFX_CHANNEL_ANY 0xFF
#define PTPLAYER_MOD_SAMPLE_COUNT 31

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

typedef struct _tPtplayerSampleHeader {
	char szName[22];
	UWORD uwLength; ///< Sample data length, in words.
	UBYTE ubFineTune; ///< Finetune. Only lower nibble is used.
	                  ///  Values translate to finetune: {0..7, -8..-1}
	UBYTE ubVolume; ///< Sample volume. 0..64
	UWORD uwRepeatOffs; ///< In words.
	UWORD uwRepeatLength; ///< In words.
} tPtplayerSampleHeader;

typedef struct _tPtplayerMod {
	char szSongName[20];
	tPtplayerSampleHeader pSampleHeaders[PTPLAYER_MOD_SAMPLE_COUNT];
	UBYTE ubArrangementLength; ///< Length of arrangement, not to be confused with
	                           /// pattern count in file. Max 128.
	UBYTE ubSongEndPos;
	UBYTE pArrangement[128]; ///< song arrangmenet list (pattern Table).
	                         /// These list up to 128 pattern numbers
	                         /// and the order they should be played in.
	char pFileFormatTag[4];  ///< Should be "M.K." for 31-sample format.
	// MOD pattern/sample data follows

	UBYTE *pPatterns;
	UWORD *pSampleStarts[PTPLAYER_MOD_SAMPLE_COUNT];
	ULONG ulPatternsSize;
	UBYTE isOwningSamples;
} tPtplayerMod;

typedef struct tPtplayerSamplePack {
	UBYTE ubSampleCount;
	tPtplayerSfx pSamples[PTPLAYER_MOD_SAMPLE_COUNT];
} tPtplayerSamplePack;

typedef void (*tPtplayerCbSongEnd)(void);
typedef void (*tPtplayerCbE8)(UBYTE ubE8);

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
 * @brief Sets PAL/NTSC mode. Relevant in CIA-based playback mode.
 *
 * Note that each tSfx stores period calculated for mode which was set during
 * call to ptplayerSfxCreateFromFd(). You may need to correct their values
 * manually.
 *
 * @param isPal In CIA mode, Set to 1 on PAL configs, zero on NTSC.
 */
void ptplayerSetPal(UBYTE isPal);

/**
 * @brief Loads new MOD from file.
 * @note This function may use OS.
 *
 * @param szPath Path to .mod file.
 * @return Pointer to new MOD structure, initialized with file contents.
 */
tPtplayerMod *ptplayerModCreateFromPath(const char *szPath);

/**
 * @brief Loads new MOD from file.
 * @note This function may use OS.
 *
 * @param pFileMod Handle to the .mod file. Will be closed on function return.
 * @return Pointer to new MOD structure, initialized with file contents.
 */
tPtplayerMod *ptplayerModCreateFromFd(tFile *pFileMod);

/**
 * @brief Frees given MOD from memory.
 * @note This function may use OS.
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
 * @param pExternalSamples When set to 0, the samples are assumed to be stored inside
 * the MOD, after the patterns. Otherwise, uses samples from given samplepack.
 * @param uwInitialSongPos
 *
 * @see ptplayerEnableMusic()
 * @see ptplayerStop()
 */
void ptplayerLoadMod(
	tPtplayerMod *pMod, tPtplayerSamplePack *pExternalSamples,
	UWORD uwInitialSongPos
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
 * for music only.
 *
 * When calling ptplayerSfxPlay with automatic channel selection
 * (sfx_cha=-1) then these masked channels will never be picked.
 * The mask defaults to 0.
 *
 * @param ubChannelMask Mask of channels reserved for playing music. Set bit 0
 * for channel 0, bit 1 for channel 1, etc.
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
 * @brief Define which channels are used for the player. Set bit 0 for channel 0, etc.
 *
 * @param ubChannelMask Bit mask of music channels to be used. Uses bits 0..3.
 */
void ptplayerSetChannelsForPlayer(UBYTE ubChannelMask);

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
 * @return Value of last-played E8 command.
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

/**
 * @brief Redefine a sample's volume on currently played MOD.
 * May also be done while the song is playing. This change is persistent and can
 * only be reverted by setting volume to previous value or reloading the MOD.
 *
 * @param ubSampleIndex Sample number (0-31).
 * @param ubVolume Volume (0-64).
 */
void ptplayerSetSampleVolume(UBYTE ubSampleIndex, UBYTE ubVolume);

//-------------------------------------------------------------------------- SFX

/**
 * @brief Loads SFX from given file.
 * Note that this function sets SFX period based on currently set PAL/NTSC video
 * mode. If you plan to change it after loading SFX, be sure to adjust
 * the period value for new mode.
 * @note This function may use OS.
 *
 * @param szPath Path to .sfx file.
 * @param isFast Set to 1 if you wish to load SFX to FAST memory.
 * Useful for software-based audio-mixing, unusable with ptplayer.
 * @return Newly loaded SFX.
 *
 * @see ptplayerSfxDestroy()
 * @see ptplayerSfxPlay()
 */
tPtplayerSfx *ptplayerSfxCreateFromPath(const char *szPath, UBYTE isFast);

/**
 * @brief Loads SFX from given file.
 * Note that this function sets SFX period based on currently set PAL/NTSC video
 * mode. If you plan to change it after loading SFX, be sure to adjust
 * the period value for new mode.
 * @note This function may use OS.
 *
 * @param pFileSfx Handle to the .sfx file. Will be closed on function return.
 * @param isFast Set to 1 if you wish to load SFX to FAST memory.
 * Useful for software-based audio-mixing, unusable with ptplayer.
 * @return Newly loaded SFX.
 *
 * @see ptplayerSfxDestroy()
 * @see ptplayerSfxPlay()
 */
tPtplayerSfx *ptplayerSfxCreateFromFd(tFile *pFileSfx, UBYTE isFast);

/**
 * @brief Destroys given SFX, freeing its resources to OS.
 * @note This function may use OS.
 *
 * @param pSfx SFX to be destroyed.
 *
 * @see ptplayerSfxCreateFromFd()
 */
void ptplayerSfxDestroy(tPtplayerSfx *pSfx);

/**
 * @brief Request playing of a prioritized external sound effect, either on a
 * fixed channel or on the most unused one.
 *
 * When multiple samples are assigned to the same channel the lower priority
 * sample will be replaced. When priorities are the same, then the older sample
 * is replaced.
 * The chosen channel is blocked for music until the effect has completely
 * been replayed.
 *
 * @param pSfx SFX sample to be played. Must be allocated in CHIP memory.
 * @param bChannel Selected replay channel (0..3), PTPLAYER_SFX_CHANNEL_ANY
 * selects best channel.
 * @param ubVolume Playback volume 0..64, unaffected by the song's master volume.
 * @param ubPriority Playback priority. The bigger the value, the higher
 * the priority. Must be non-zero.
 *
 * @see ptplayerSetMusicChannelMask()
 * @see ptplayerSfxPlayLooped()
 */
void ptplayerSfxPlay(
	const tPtplayerSfx *pSfx, UBYTE ubChannel, UBYTE ubVolume, UBYTE ubPriority
);

/**
 * @brief Request playing of a looped external sound effect, on a fixed channel.
 *
 * @param pSfx Sfx sample to be played. Must be allocated in CHIP memory.
 * @param ubChannel Selected replay channel (0..3).
 * @param ubVolume Playback volume 0..64, unaffected by the song's master volume.
 *
 * @see ptplayerSfxPlay()
 */
void ptplayerSfxPlayLooped(
	const tPtplayerSfx *pSfx, UBYTE ubChannel, UBYTE ubVolume
);

/**
 * @brief Stops SFX played on given channel.
 *
 * @param ubChannel Selected SFX channel (0..3).
 */
void ptplayerSfxStopOnChannel(UBYTE ubChannel);

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

UBYTE ptplayerSfxLengthInFrames(const tPtplayerSfx *pSfx);

/**
 * @brief Loads MOD sample pack from file at given path.
 * @note This function may use OS.
 *
 * @param szPath Path to sample pack to be loaded.
 * @return Pointer to newly allocated sample pack, zero on failure.
 */
tPtplayerSamplePack *ptplayerSampleDataCreateFromPath(const char *szPath);

/**
 * @brief Loads MOD sample pack from file at given path.
 * @note This function may use OS.
 *
 * @param pFileSamples Handle to the sample pack file to be loaded. Will be closed on function return.
 * @return Pointer to newly allocated sample pack, zero on failure.
 */
tPtplayerSamplePack *ptplayerSampleDataCreateFromFd(tFile *pFileSamples);

/**
 * @brief Destroys given sample pack, freeing its resources to OS.
 * @note This function may use OS.
 *
 * @param pSamplePack Sample pack to be destroyed.
 */
void ptplayerSamplePackDestroy(tPtplayerSamplePack *pSamplePack);

/**
 * @brief Sets the function to call on parsing the E8 command.
 *
 * @param cbOnE8 Function to be called. E8 argument nibble is passed
 * as argument. Set to zero if not needed.
 */
void ptplayerSetE8Callback(tPtplayerCbE8 cbOnE8);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_PTPLAYER_H_
