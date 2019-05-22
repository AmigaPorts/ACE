/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/ptplayer.h>

UBYTE mt_MusicChannels = 0;
UBYTE mt_E8Trigger = 0;
UBYTE mt_Enable = 0;

typedef struct _tChannelStatus {
	UWORD n_note;
	UBYTE n_cmd;
	UBYTE n_cmdlo;
	ULONG n_start;
	ULONG n_loopstart;
	UWORD n_length;
	UWORD n_replen;
	UWORD n_period;
	UWORD n_volume;
	ULONG n_pertab;
	UWORD n_dmabit;
	UWORD n_noteoff;
	UWORD n_toneportspeed;
	UWORD n_wantedperiod;
	UWORD n_pattpos;
	UWORD n_funk;
	ULONG n_wavestart;
	UWORD n_reallength;
	UWORD n_intbit;
	UWORD n_audreg;
	UWORD n_sfxlen;
	ULONG n_sfxptr;
	UWORD n_sfxper;
	UWORD n_sfxvol;
	UBYTE n_looped;
	UBYTE n_minusft;
	UBYTE n_vibratoamp;
	UBYTE n_vibratospd;
	UBYTE n_vibratopos;
	UBYTE n_vibratoctrl;
	UBYTE n_tremoloamp;
	UBYTE n_tremolospd;
	UBYTE n_tremolopos;
	UBYTE n_tremoloctrl;
	UBYTE n_gliss;
	UBYTE n_sampleoffset;
	UBYTE n_loopcount;
	UBYTE n_funkoffset;
	UBYTE n_retrigcount;
	UBYTE n_sfxpri;
	UBYTE n_freecnt;
	UBYTE n_musiconly;
} tChannelStatus;
