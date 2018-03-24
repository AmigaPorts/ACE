#include <ace/managers/system.h>
#include <hardware/intbits.h>
#include <hardware/dmabits.h>
#include <ace/utils/custom.h>
#include <ace/managers/log.h>

// http://eab.abime.net/showthread.php?p=1081007
// http://ada.untergrund.net/?p=boardthread&id=31
// http://palbo.dk/dataskolen/maskinsprog/english/letter_09.pdf

//---------------------------------------------------------------------- DEFINES

// First OS interrupt is at offset 0x64 - each is 4 bytes wide
#define SYSTEM_OS_INT_FIRST (0x64/4)
#define SYSTEM_OS_INT_COUNT 7

//------------------------------------------------------------------------ TYPES

typedef void (*tHwIntVector)(void);

typedef struct _tAceInterrupt {
	volatile tAceIntHandler pHandler;
	volatile void *pData;
} tAceInterrupt;

//---------------------------------------------------------------------- GLOBALS

// Saved regs
UWORD s_uwOsIntEna;
UWORD s_uwOsDmaCon;

// Interrupts
void HWINTERRUPT int1Handler(void);
void HWINTERRUPT int2Handler(void);
void HWINTERRUPT int3Handler(void);
void HWINTERRUPT int4Handler(void);
void HWINTERRUPT int5Handler(void);
void HWINTERRUPT int6Handler(void);
void HWINTERRUPT int7Handler(void);
tHwIntVector s_pAceHwInterrupts[SYSTEM_OS_INT_COUNT] = {
	int1Handler, int2Handler, int3Handler, int4Handler,
	int5Handler, int6Handler, int7Handler
};
static tHwIntVector s_pOsHwInterrupts[SYSTEM_OS_INT_COUNT] = {0};
static volatile tHwIntVector * const s_pHwVectors = 0;
tAceInterrupt s_pAceInterrupts[15] = {{0}};

// Manager logic vars
WORD s_wSystemUses;

//----------------------------------------------------------- INTERRUPT HANDLERS

void HWINTERRUPT int1Handler(void) {
	// Soft / diskBlk / TBE
}

void HWINTERRUPT int2Handler(void) {
	// Parallel, keyboard, mouse, "some of disk functions"
	// ACE only uses it for keyboard so no decision taking here atm
	// TODO: this could be re-enabled with vblank since we don't need it
	// to retrigger during same frame.
	if(s_pAceInterrupts[INTB_PORTS].pHandler) {
		s_pAceInterrupts[INTB_PORTS].pHandler(
			g_pCustom, s_pAceInterrupts[INTB_PORTS].pData
		);
	}
	g_pCustom->intreq = INTF_PORTS;
}

void HWINTERRUPT int3Handler(void) {
	// VBL / Copper / Blitter
	UWORD uwIntReq = g_pCustom->intreqr;
	UWORD uwReqClr = 0;

	// Vertical blanking
	if(uwIntReq & INTF_VERTB) {
		if(s_pAceInterrupts[INTB_VERTB].pHandler) {
			s_pAceInterrupts[INTB_VERTB].pHandler(
				g_pCustom, s_pAceInterrupts[INTB_VERTB].pData
			);
		}
		uwReqClr = INTF_VERTB;
	}

	// Copper
	if(uwIntReq & INTF_COPER) {
		if(s_pAceInterrupts[INTB_COPER].pHandler) {
			s_pAceInterrupts[INTB_COPER].pHandler(
				g_pCustom, s_pAceInterrupts[INTB_VERTB].pData
			);
		}
		uwReqClr |= INTF_COPER;
	}

	// Blitter
	if(uwIntReq & INTF_BLIT) {
		if(s_pAceInterrupts[INTB_BLIT].pHandler) {
			s_pAceInterrupts[INTB_BLIT].pHandler(
				g_pCustom, s_pAceInterrupts[INTB_VERTB].pData
			);
		}
		uwReqClr |= INTF_BLIT;
	}
	g_pCustom->intreq = INTF_INTEN | INTF_VERTB | INTF_BLIT | INTF_COPER;
}

void HWINTERRUPT int4Handler(void) {
	// Audio
}

void HWINTERRUPT int5Handler(void) {
	// DskSyn / RBF
}

void HWINTERRUPT int6Handler(void) {
	// CIA B
}

void HWINTERRUPT int7Handler(void) {
	// EXTERNAL
}

//-------------------------------------------------------------------- FUNCTIONS

/**
 * @brief The startup code to give ACE somewhat initial state.
 */
void systemCreate(void) {
	// save the system copperlists and flush the view
	// copCreate();

	// Reset active system uses counter so that systemUnuse will do a takeover
	s_wSystemUses = 1;

	systemUnuse();

	for(UBYTE i = 0; i < 50; ++i) {
		while(g_pRayPos->bfPosY < 200) {};
		while(g_pRayPos->bfPosY > 50) {};
	}
}

/**
 * @brief Cleanup after app, restore anything that systemCreate took over.
 */
void systemDestroy(void) {
	// disable app interrupts
	g_pCustom->intena = 0x7FFF;
	g_pCustom->intreq = 0x7FFF;

	// Wait for vbl before disabling sprite DMA
	while (!(g_pCustom->intreqr & INTF_VERTB)) {}
	g_pCustom->dmacon = 0x7FFF;
	g_pCustom->intreq = 0x7FFF;

	// restore system copperlists
	// copDestroy();

	if(s_wSystemUses != 0) {
		logWrite("ERR: unclosed system usage count: %hd", s_wSystemUses);
		s_wSystemUses = 0;
	}

	systemUse();

	// restore old view
}

void systemUnuse(void) {
	--s_wSystemUses;
	if(!s_wSystemUses) {
		// get VBR location on 68010+ machine

		// save the state of the hardware registers (INTENA, DMA, ADKCON etc.)
		s_uwOsIntEna = g_pCustom->intenar;
		s_uwOsDmaCon = g_pCustom->dmaconr;

		// Disable interrupts (this is the actual "kill system/OS" part)
		g_pCustom->intena = 0x7FFF;
		g_pCustom->intreq = 0x7FFF;

		// Wait for vbl before disabling sprite DMA
		while (!(g_pCustom->intreqr & INTF_VERTB)) {}
		g_pCustom->dmacon = 0x7FFF;

		// Save OS interrupt vectors
		g_pCustom->intreq = 0x7FFF;
		for(UWORD i = 0; i < SYSTEM_OS_INT_COUNT; ++i) {
			s_pOsHwInterrupts[i] = s_pHwVectors[SYSTEM_OS_INT_FIRST + i];
			s_pHwVectors[SYSTEM_OS_INT_FIRST + i] = s_pAceHwInterrupts[i];
		}

		// Enable needed DMA (and interrupt) channels
		// Everything that's supported by ACE to simplify things for now
		g_pCustom->intena = INTF_SETCLR | INTF_INTEN | (
			INTF_BLIT | INTF_COPER | INTF_VERTB |
			INTF_PORTS
		);
	}
// #ifdef GAME_DEBUG
	if(s_wSystemUses < 0) {
		logWrite("ERR: System uses less than 0!\n");
		s_wSystemUses = 0;
	}
// #endif
}

void systemUse(void) {
	if(!s_wSystemUses) {
		// disable app interrupts/dma
		g_pCustom->intena = 0x7FFF;
		g_pCustom->intreq = 0x7FFF;
		g_pCustom->dmacon = 0x7FFF; // ^ (DMAF_COPPER | DMAF_RASTER);

		// restore old DMA/INTENA/ADKCON etc. settings
		for(UWORD i = 0; i < SYSTEM_OS_INT_COUNT; ++i) {
			s_pHwVectors[SYSTEM_OS_INT_FIRST + i] = s_pOsHwInterrupts[i];
		}
		g_pCustom->dmacon = DMAF_SETCLR | DMAF_MASTER | s_uwOsDmaCon;
		g_pCustom->intena = INTF_SETCLR | INTF_INTEN  | s_uwOsIntEna;
	}
	++s_wSystemUses;
}

void systemSetInt(
	UBYTE ubIntNumber, tAceIntHandler pHandler, volatile void *pIntData
) {

	// Disable interrupts during handler swap to ensure atomic op
	UBYTE isIntEnabled = 0;
	if(g_pCustom->intenar & INTF_INTEN) {
		isIntEnabled = 1;
		g_pCustom->intena = INTF_INTEN;
	}

	s_pAceInterrupts[ubIntNumber].pData = pIntData;
	s_pAceInterrupts[ubIntNumber].pHandler = pHandler;
	if(pHandler) {
		// Handler was passed - enable listening for given interrupt
		g_pCustom->intena = INTF_SETCLR | BV(ubIntNumber);
	}
	else {
		// No handler given - disable this interrupt
		g_pCustom->intena = BV(ubIntNumber);
	}

	// Reenable interrupts if they were enabled
	if(isIntEnabled) {
		g_pCustom->intena = INTF_SETCLR | INTF_INTEN;
	}
}

// void systemSetDma(UBYTE ubDmaNumber, UBYTE isEnabled) {
// 	// TODO
// }
