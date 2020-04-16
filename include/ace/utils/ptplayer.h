/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Protracker V2.3B Playroutine Version 5.1
// Written by Frank Wille in 2013, 2016, 2017, 2018.
// Rewritten to C by KaiN for ACE

#ifndef _ACE_UTILS_PTPLAYER_H_
#define _ACE_UTILS_PTPLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>

/**
 * @brief Install a CIA-B interrupt for calling _mt_music or mt_sfxonly.
 * The music module is replayed via _mt_music when _mt_Enable is non-zero.
 * Otherwise the interrupt handler calls mt_sfxonly to play sound effects only.
 *
 * @param isPal In CIA mode, Set to 1 on PAL configs, zero on NTSC.
 */
void ptplayerInstallInterrupts(UBYTE isPal);

/**
 * @brief Initialize a new module.
 * Reset speed to 6, tempo to 125 and start at the given position.
 * Master volume is at 64 (maximum).
 * When a1 is NULL the samples are assumed to be stored after the patterns.
 *
 * @param TrackerModule
 * @param Samples
 * @param InitialSongPos
 */
void ptplayerInit(UBYTE *TrackerModule, UWORD *Samples, UWORD InitialSongPos);

/**
 * @brief Stop playing current module.
 */
void ptplayerEnd(void);

/*
  _mt_soundfx(a6=CUSTOM, a0=SamplePointer,
             d0=SampleLength.w, d1=SamplePeriod.w, d2=SampleVolume.w)
    Request playing of an external sound effect on the most unused channel.
    This function is for compatibility with the old API only!
    You should call _mt_playfx instead.
*/

void mt_soundfx(
	APTR SamplePointer, UWORD SampleLength,
	UWORD SamplePeriod, UWORD SampleVolume
);

/*
  _mt_playfx(a6=CUSTOM, a0=SfxStructurePointer)
    Request playing of a prioritized external sound effect, either on a
    fixed channel or on the most unused one.
    Structure layout of SfxStructure:
      APTR sfx_ptr (pointer to sample start in Chip RAM, even address)
      WORD sfx_len (sample length in words)
      WORD sfx_per (hardware replay period for sample)
      WORD sfx_vol (volume 0..64, is unaffected by the song's master volume)
      BYTE sfx_cha (0..3 selected replay channel, -1 selects best channel)
      UBYTE sfx_pri (unsigned priority, must be non-zero)
    When multiple samples are assigned to the same channel the lower
    priority sample will be replaced. When priorities are the same, then
    the older sample is replaced.
    The chosen channel is blocked for music until the effect has
    completely been replayed.
*/

typedef struct _tSfxStructure
{
	APTR sfx_ptr;  /* pointer to sample start in Chip RAM, even address */
	UWORD sfx_len; /* sample length in words */
	UWORD sfx_per; /* hardware replay period for sample */
	UWORD sfx_vol; /* volume 0..64, is unaffected by the song's master volume */
	BYTE sfx_cha;  /* 0..3 selected replay channel, -1 selects best channel */
	UBYTE sfx_pri; /* unsigned priority, must be non-zero */
} tSfxStructure;

void mt_playfx(tSfxStructure *SfxStructurePointer);

/*
  _mt_musicmask(a6=CUSTOM, d0=ChannelMask.b)
    Set bits in the mask define which specific channels are reserved
    for music only. Set bit 0 for channel 0, ..., bit 3 for channel 3.
    When calling _mt_soundfx or _mt_playfx with automatic channel selection
    (sfx_cha=-1) then these masked channels will never be picked.
    The mask defaults to 0.
*/

void mt_musicmask(UBYTE ChannelMask);

/*
  _mt_mastervol(a6=CUSTOM, d0=MasterVolume.w)
    Set a master volume from 0 to 64 for all music channels.
    Note that the master volume does not affect the volume of external
    sound effects (which is desired).
*/

void mt_mastervol(UWORD MasterVolume);

/*
  _mt_music(a6=CUSTOM)
    The replayer routine. Is called automatically after ptplayerInstallInterrupts().
*/
void mt_music(void);

void ptplayerProcess(void);

/**
 * @brief Enables or disabls music playback.
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

#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_PTPLAYER_H_
