/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_PTPLAYER_PRIVATE_H_
#define _ACE_MANAGERS_PTPLAYER_PRIVATE_H_

#include <ace/managers/ptplayer.h>
#include <ace/managers/system.h>

// Patterns - each has 64 rows, each row has 4 notes, each note has 4 bytes.
#define MOD_ROWS_IN_PATTERN 64
#define MOD_NOTES_PER_ROW 4
#define MOD_BYTES_PER_NOTE 4
// Length of single pattern.
#define MOD_PATTERN_LENGTH (MOD_ROWS_IN_PATTERN * MOD_NOTES_PER_ROW * MOD_BYTES_PER_NOTE)

// Size of period table.
#define MOD_PERIOD_TABLE_LENGTH 36

#endif // _ACE_MANAGERS_PTPLAYER_PRIVATE_H_

/**
 * @brief Mutes sfx channels playing given sample.
 *
 * Used for Amiga impl of ptplayer on destroying samples - ptplayer doesn't mute
 * channels on playback end to save cycles and just loops playback over empty
 * first word.
 * If memory gets reallocated, the word will become non-empty resulting in sfx glitch.
 */
void muteChannelsPlayingSfx(const tPtplayerSfx *pSfx);

/**
 * @brief Get the Paula's Clock Constant.
 *
 * http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node00DE.html
 *
 * @return The current clock constant, dependent on PAL/NTSC mode.
 */
static inline ULONG getClockConstant() {
	return systemIsPal() ? 3546895 : 3579545;
}
