/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Info regarding MOD file format:
// - https://www.fileformat.info/format/mod/corion.htm
// - http://lclevy.free.fr/mo3/mod.txt

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
UWORD *mt_SampleStarts[31];
UBYTE *mt_mod;
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
UWORD mt_dmaon = DMAF_SETCLR;

ULONG *mt_Lev6Int;

// The replayer routine. Is called automatically after mt_install_cia().
// Called from interrupt.
// Play next position when Counter equals Speed.
// Effects are always handled.
void mt_music(UNUSED_ARG APTR custom) {

}

// Called from interrupt.
// Plays sound effects on free channels.
void mt_sfxonly(void) {

}

// TimerA interrupt calls _mt_music at a selectable tempo (Fxx command),
// which defaults to 50 times per second.
void mt_TimerAInt(void) {
	// clear EXTER interrupt flag
	g_pCustom->intreq = INTF_EXTER;

	// check and clear CIAB interrupt flags
	if(g_pCiaB->icr & CIAICR_TIMER_A) {
		// it was a TA interrupt, do music when enabled
		if(mt_Enable) {
			// music with sfx inserted
			mt_music(g_pCustom);
		}
		else {
			// no music, only sfx
			mt_sfxonly();
		}
	}
}

// One-shot TimerB interrupt to set repeat samples after another 496 ticks.
void mt_TimerBsetrep(void) {
	// check and clear CIAB interrupt flags
	if(g_pCiaB->icr & CIAICR_TIMER_B) {

		// clear EXTER and possible audio interrupt flags
		// KaiN's note: Audio DMAs are 0..3 whereas INTs are (0..3) << 7
		g_pCustom->intreq = INTF_EXTER | (mt_dmaon & 0xFF) << 7;

		// it was a TB interrupt, set repeat sample pointers and lengths
		g_pCustom->aud[0].ac_ptr = mt_chan1.n_loopstart;
		g_pCustom->aud[0].ac_len = mt_chan1.n_replen;
		g_pCustom->aud[1].ac_ptr = mt_chan2.n_loopstart;
		g_pCustom->aud[1].ac_len = mt_chan2.n_replen;
		g_pCustom->aud[2].ac_ptr = mt_chan3.n_loopstart;
		g_pCustom->aud[2].ac_len = mt_chan3.n_replen;
		g_pCustom->aud[3].ac_ptr = mt_chan4.n_loopstart;
		g_pCustom->aud[3].ac_len = mt_chan4.n_replen;


		// restore TimerA music interrupt vector
		*mt_Lev6Int = (ULONG)mt_TimerAInt;
	}

	// just clear EXTER interrupt flag and return
	g_pCustom->intreq = INTF_EXTER;
}

// One-shot TimerB interrupt to enable audio DMA after 496 ticks.
void mt_TimerBdmaon(void) {
	// clear EXTER interrupt flag
	g_pCustom->intreq = INTF_EXTER;

	// check and clear CIAB interrupt flags
	if(g_pCiaB->icr & CIAICR_TIMER_B) {
		// it was a TB interrupt, restart timer to set repeat, enable DMA
		g_pCiaB->crb = 0x19;
		g_pCustom->dmacon = mt_dmaon;

		// set level 6 interrupt to mt_TimerBsetrep
		*mt_Lev6Int = (ULONG)mt_TimerBsetrep;
	}
}

// Stop playing current module.
void mt_end(void) {
	g_pCustom->aud[0].ac_vol = 0;
	g_pCustom->aud[1].ac_vol = 0;
	g_pCustom->aud[2].ac_vol = 0;
	g_pCustom->aud[3].ac_vol = 0;
	g_pCustom->dmacon = DMAF_AUDIO;
}

void mt_reset(void) {
	// reset speed and counters
	mt_Speed = 6;
	mt_Counter = 0;
	mt_PatternPos = 0;

	// Disable the filter
	g_pCiaA->pra |= 0x02;

	// set master volume to 64
	mt_MasterVolTab = MasterVolTab64;

	// initialise channel DMA, interrupt bits and audio register base
	mt_chan1.n_dmabit = DMAF_AUD0;
	mt_chan2.n_dmabit = DMAF_AUD1;
	mt_chan3.n_dmabit = DMAF_AUD2;
	mt_chan4.n_dmabit = DMAF_AUD3;
	mt_chan1.n_intbit = INTF_AUD0;
	mt_chan2.n_intbit = INTF_AUD1;
	mt_chan3.n_intbit = INTF_AUD2;
	mt_chan4.n_intbit = INTF_AUD3;
	mt_chan1.n_audreg = &g_pCustom->aud[0].ac_ptr;
	mt_chan2.n_audreg = &g_pCustom->aud[1].ac_ptr;
	mt_chan3.n_audreg = &g_pCustom->aud[2].ac_ptr;
	mt_chan4.n_audreg = &g_pCustom->aud[3].ac_ptr;

	// make sure n_period doesn't start as 0
	mt_chan1.n_period = 320;
	mt_chan2.n_period = 320;
	mt_chan3.n_period = 320;
	mt_chan4.n_period = 320;

	// disable sound effects
	mt_chan1.n_sfxlen = 0;
	mt_chan2.n_sfxlen = 0;
	mt_chan3.n_sfxlen = 0;
	mt_chan4.n_sfxlen = 0;

	mt_SilCntValid = 0;
	mt_E8Trigger = 0;
	mt_end();
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

#define MOD_OFFS_PATTERN_COUNT 950
#define MOD_OFFS_SONG_END_POS 951
// Pattern play sequences (128 bytes)
#define MOD_OFFS_ARRANGEMENT 952
// Patterns - each has 64 rows, each row has 4-8 notes, each note has 4 bytes
#define MOD_OFFS_PATTERN_START 1084
// Length of single pattern.
#define MOD_PATTERN_LENGTH 1024

typedef struct _tSampleHeader {
	char szName[22];
	UWORD uwLength; ///< Sample data length, in words.
	UBYTE ubNibble; ///< Finetune.
	UBYTE ubVolume;
	UWORD uwRepeatOffs; ///< In words.
	UWORD uwRepeatLength; ///< In words.
} tSampleHeader;

typedef struct _tModule {
	char szSongName[20];
	tSampleHeader pSamples[31];
	UBYTE ubPatternCount;
	UBYTE ubSongEndPos;
	UBYTE ubArrangement[128];
	char pFileFormatTag[4];
} tModule;

void mt_init(
	UNUSED_ARG APTR custom, UBYTE *TrackerModule, UBYTE *Samples,
	UWORD InitialSongPos
) {
	// Initialize new module.
	// Reset speed to 6, tempo to 125 and start at given song position.
	// Master volume is at 64 (maximum).

	mt_mod = TrackerModule;

	// set initial song position
	if(InitialSongPos >= 950) {
		InitialSongPos = 0;
	}
	mt_SongPos = InitialSongPos;

	// sample data location is given?
	if(!Samples) {
		// song arrangmenet list (pattern Table).
		// These list up to 128 pattern numbers and the order they
		// should be played in.
		UBYTE *pArrangement = &TrackerModule[MOD_OFFS_ARRANGEMENT];

		// Get number of highest pattern
		UBYTE ubLastPattern = 0;
		for(UBYTE i = 0; i < 127; ++i) {
			if(pArrangement[i] > ubLastPattern) {
				ubLastPattern = pArrangement[i];
			}
		}
		UBYTE ubPatternCount = ubLastPattern + 1;

		// now we can calculate the base address of the sample data
		ULONG ulSampleOffs = (
			MOD_OFFS_PATTERN_START + ubPatternCount * MOD_PATTERN_LENGTH
		);
		Samples = &TrackerModule[ulSampleOffs];
		// FIXME: use as pointer for empty samples
	}

	tSampleHeader *pHeaders = (tSampleHeader*)&TrackerModule[42];

	// save start address of each sample
	UBYTE *pSampleCurr = Samples;
	for(UBYTE i = 0; i < 31; ++i) {
		mt_SampleStarts[i] = (UWORD*)&pSampleCurr;

		// make sure sample starts with two 0-bytes
		*mt_SampleStarts[i] = 0;

		// go to next sample
		pSampleCurr += pHeaders[i].uwLength * 2;
	};

	// reset CIA timer A to default (125)
	UWORD uwTimer = mt_timerval / 125;
	g_pCiaB->talo = uwTimer;
	g_pCiaB->tahi = uwTimer >> 8;

	mt_reset();
}

}

// Request playing of an external sound effect on the most unused channel.
// This function is for compatibility with the old API only!
// You should call mt_playfx instead.
void mt_soundfx(
	APTR custom, APTR SamplePointer, UWORD SampleLength,
	UWORD SamplePeriod, UWORD SampleVolume
) {
	tSfxStructure sSfx;
	sSfx.sfx_ptr = SamplePointer;
	sSfx.sfx_len = SampleLength;
	sSfx.sfx_per = SamplePeriod;
	sSfx.sfx_vol = SampleVolume;
	// any channel, priority = 1
	sSfx.sfx_cha = -1;
	sSfx.sfx_pri = 1;
	mt_playfx(custom, &sSfx);
}
