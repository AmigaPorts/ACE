/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/ptplayer.h>
#include <ace/utils/custom.h>
#include <hardware/intbits.h>
#include <hardware/dmabits.h>

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

UBYTE mt_MusicChannels = 0;
UBYTE mt_E8Trigger = 0;
UBYTE mt_Enable = 0;

tChannelStatus mt_chan1;
tChannelStatus mt_chan2;
tChannelStatus mt_chan3;
tChannelStatus mt_chan4;
ULONG mt_SampleStarts[31];
ULONG mt_mod;
ULONG mt_oldLev6;
ULONG mt_timerval;
UBYTE mt_oldtimers[4];
ULONG mt_MasterVolTab;
UWORD mt_Lev6Ena;
UWORD mt_PatternPos;
UWORD mt_PBreakPos;
UBYTE mt_PosJumpFlag;
UBYTE mt_PBreakFlag;
UBYTE mt_Speed;
UBYTE mt_Counter;
UBYTE mt_SongPos;
UBYTE mt_PattDelTime;
UBYTE mt_PattDelTime2;
UBYTE mt_SilCntValid;

ULONG *mt_Lev6Int;

void mt_TimerAInt(void) {

}

void mt_reset(void) {

}

void mt_install_cia(UNUSED_ARG APTR custom, APTR *AutoVecBase, UBYTE PALflag) {
	mt_Enable = 0;

	// Level 6 interrupt vector
	mt_Lev6Int = (ULONG*)&AutoVecBase[0x78/sizeof(ULONG)];

	// remember level 6 interrupt enable
	AutoVecBase[0x78/sizeof(ULONG)] = mt_Lev6Int;
	mt_Lev6Ena = (g_pCustom->intenar & INTF_EXTER) | INTF_SETCLR;

	// disable level 6 EXTER interrupts, set player interrupt vector
	g_pCustom->intena = INTF_EXTER;
	mt_oldLev6 = *mt_Lev6Int;
	*mt_Lev6Int = (ULONG)mt_TimerAInt;

	// disable CIA-B interrupts, stop and save all timers
	g_pCiaB->icr = 0x7f;
	g_pCiaB->cra = 0x10;
	g_pCiaB->crb = 0x10;
	mt_oldtimers[0] = g_pCiaB->talo;
	mt_oldtimers[1] = g_pCiaB->tahi;
	mt_oldtimers[2] = g_pCiaB->tblo;
	mt_oldtimers[3] = g_pCiaB->tbhi;

	// determine if 02 clock for timers is based on PAL or NTSC
	if(PALflag) {
		mt_timerval = 1773447;
	}
	else {
		mt_timerval = 1789773;
	}

	// load TimerA in continuous mode for the default tempo of 125
	g_pCiaB->talo = 125;
	g_pCiaB->tahi = 8;
	g_pCiaB->cra = 0x11; // load timer, start continuous

	// load TimerB with 496 ticks for setting DMA and repeat
	g_pCiaB->tblo = 496 & 0xFF;
	g_pCiaB->tbhi = 496 >> 8;

	// TimerA and TimerB interrupt enable
	g_pCiaB->icr = 0x83;

	// enable level 6 interrupts
	g_pCustom->intena = INTF_SETCLR | INTF_EXTER;

	mt_reset();
}

void mt_remove_cia(UNUSED_ARG APTR custom) {
	// disable level 6 and CIA-B interrupts
	g_pCiaB->icr = 0x7F;
	g_pCustom->intena = INTF_EXTER;

	// restore old timer values
	g_pCiaB->talo = mt_oldtimers[0];
	g_pCiaB->tahi = mt_oldtimers[1];
	g_pCiaB->tblo = mt_oldtimers[2];
	g_pCiaB->tbhi = mt_oldtimers[3];
	g_pCiaB->cra = 0x10;
	g_pCiaB->crb = 0x10;

	// restore original level 6 interrupt vector
	*mt_Lev6Int = mt_oldLev6;

	// reenable CIA-B ALRM interrupt, which was set by AmigaOS
	g_pCiaB->icr = 0x84;

	// reenable previous level 6 interrupt
	g_pCustom->intena = mt_Lev6Ena;
}
