/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/system.h>
#include <stdlib.h>
#include <clib/graphics_protos.h>
#include <clib/dos_protos.h>
#include <hardware/intbits.h>
#include <hardware/dmabits.h>
#include <devices/audio.h>
#include <ace/utils/custom.h>
#include <ace/managers/log.h>
#include <ace/managers/timer.h>
#include <ace/managers/key.h>
#include <exec/execbase.h>
#include <exec/lists.h>
#include <resources/cia.h>
#include <proto/exec.h> // Bartman's compiler needs this
#include <proto/dos.h> // Bartman's compiler needs this
#include <proto/graphics.h> // Bartman's compiler needs this
#include <proto/cia.h>
#if defined(BARTMAN_GCC)
#include <bartman/gcc8_c_support.h> // Idle measurement
#endif

// There are hardware interrupt vectors
// Some may be triggered by more than one event - there are 15 events
// http://eab.abime.net/showthread.php?p=1081007
// http://ada.untergrund.net/?p=boardthread&id=31
// http://palbo.dk/dataskolen/maskinsprog/english/letter_09.pdf
// Clearing bit is done twice for some high end configs
// http://eab.abime.net/showthread.php?t=95263

//---------------------------------------------------------------------- DEFINES

// First OS interrupt is at offset 0x64 - each is 4 bytes wide
#define SYSTEM_INT_VECTOR_FIRST (0x64/4)
#define SYSTEM_INT_VECTOR_COUNT 7
#define SYSTEM_INT_HANDLER_COUNT 15
#define SYSTEM_STACK_CANARY '\xBA'

//------------------------------------------------------------------------ TYPES

typedef void (*tHwIntVector)(void);

typedef struct _tAceInterrupt {
	tAceIntHandler pHandler;
	void *pData;
} tAceInterrupt;

//---------------------------------------------------------------------- GLOBALS

// Store VBR query code in .text so that it plays nice with instruction cache
// TODO: move to .s file after vasm support gets merged into cmake
__attribute__((section("text")))
static const UWORD s_pGetVbrCode[] = {0x4e7a, 0x0801, 0x4e73};

// Saved regs
static UWORD s_uwOsIntEna;
static UWORD s_uwOsDmaCon;
static UWORD s_uwAceDmaCon = 0;
static UWORD s_uwOsInitialDma;

static UWORD s_pOsCiaTimerA[CIA_COUNT];
static UWORD s_pOsCiaTimerB[CIA_COUNT];
static UBYTE s_pOsCiaIcr[CIA_COUNT], s_pOsCiaCra[CIA_COUNT], s_pOsCiaCrb[CIA_COUNT];
static UWORD s_pAceCiaTimerA[CIA_COUNT] = {0xFFFF, 0xFFFF}; // as long as possible
static UWORD s_pAceCiaTimerB[CIA_COUNT] = {0xFFFF, 0xFFFF};
static UBYTE s_pAceCiaCra[CIA_COUNT] = {0, 0};
static UBYTE s_pAceCiaCrb[CIA_COUNT] = {0, 0};

// Interrupts
static void HWINTERRUPT int1Handler(void);
static void HWINTERRUPT int2Handler(void);
static void HWINTERRUPT int3Handler(void);
static void HWINTERRUPT int4Handler(void);
static void HWINTERRUPT int5Handler(void);
static void HWINTERRUPT int6Handler(void);
static void HWINTERRUPT int7Handler(void);
static const tHwIntVector s_pAceHwInterrupts[SYSTEM_INT_VECTOR_COUNT] = {
	int1Handler, int2Handler, int3Handler, int4Handler,
	int5Handler, int6Handler, int7Handler
};
static volatile tHwIntVector * s_pHwVectors = 0;
static tHwIntVector s_pOsHwInterrupts[SYSTEM_INT_VECTOR_COUNT] = {0};
static tAceInterrupt s_pAceInterrupts[SYSTEM_INT_HANDLER_COUNT] = {{0}};
static tAceInterrupt s_pAceCiaInterrupts[CIA_COUNT][5] = {{{0}}};
static UWORD s_uwAceIntEna = INTF_VERTB | INTF_PORTS | INTF_EXTER;

// Manager logic vars
static WORD s_wSystemUses;
static WORD s_wSystemBlitterUses;

struct GfxBase *GfxBase = 0;
struct View *s_pOsView;
static const UWORD s_uwOsMinDma = DMAF_DISK | DMAF_BLITTER;
static struct IOAudio s_sIoAudio = {0};
static struct Library *s_pCiaResource[CIA_COUNT];
static struct Process *s_pProcess;
static struct MsgPort *s_pCdtvIOPort;
static struct IOStdReq *s_pCdtvIOReq;

#if defined(BARTMAN_GCC)
struct DosLibrary *DOSBase = 0;
struct ExecBase *SysBase = 0;
#endif

//----------------------------------------------------------- INTERRUPT HANDLERS

// All interrupt handlers should clear their flags twice
// http://eab.abime.net/showthread.php?p=834185#post834185

FN_HOTSPOT
void HWINTERRUPT int1Handler(void) {
	logPushInt();
	// Soft / diskBlk / TBE (RS232 TX end)
	logPopInt();
}

FN_HOTSPOT
void HWINTERRUPT int2Handler(void) {
	logPushInt();
	UWORD uwIntReq = g_pCustom->intreqr;
	if(uwIntReq & INTF_PORTS) {
		// CIA A interrupt - Parallel, keyboard
		UBYTE ubIcr = g_pCia[CIA_A]->icr; // Read clears interrupt flags
		if(ubIcr & CIAICRF_SERIAL) {
			// Keyboard
			if(s_pAceCiaInterrupts[CIA_A][CIAICRB_SERIAL].pHandler) {
				s_pAceCiaInterrupts[CIA_A][CIAICRB_SERIAL].pHandler(
					g_pCustom, s_pAceCiaInterrupts[CIA_A][CIAICRB_SERIAL].pData
				);
			}
			if(ubIcr & CIAICRF_TIMER_A) {
				if(s_pAceCiaInterrupts[CIA_A][CIAICRB_TIMER_A].pHandler) {
					s_pAceCiaInterrupts[CIA_A][CIAICRB_TIMER_A].pHandler(
						g_pCustom, s_pAceCiaInterrupts[CIA_A][CIAICRB_TIMER_A].pData
					);
				}
			}
			// CIA-A timer A is used for keyboard, timer B for exec task switch
			// For some reason after save/restore A.ta is used without problems,
			// but A.tb makes OS unstable - we don't use it until it gets fixed
			// if(ubIcr & CIAICRF_TIMER_B) {
			// 	if(s_pAceCiaInterrupts[CIA_A][CIAICRB_TIMER_B].pHandler) {
			// 		s_pAceCiaInterrupts[CIA_A][CIAICRB_TIMER_B].pHandler(
			// 			g_pCustom, s_pAceCiaInterrupts[CIA_A][CIAICRB_TIMER_B].pData
			// 		);
			// 	}
			// }
			// TODO: this could be re-enabled in vblank since we don't need it
			// to retrigger during same frame. Or perhaps it's needed so that kbd
			// controller won't overflow its queue
		}
		g_pCustom->intreq = INTF_PORTS;
		g_pCustom->intreq = INTF_PORTS;
	}
	// TODO: handle "mouse, some of disk functions"
	logPopInt();
}

FN_HOTSPOT
void HWINTERRUPT int3Handler(void) {
	logPushInt();
	// VBL / Copper / Blitter
	UWORD uwIntReq = g_pCustom->intreqr;
	UWORD uwReqClr = 0;

	// Vertical blanking
	if(uwIntReq & INTF_VERTB) {
		// Do ACE-specific stuff
		// TODO when ACE gets ported to C++ this could be constexpr if'ed
		timerOnInterrupt();

		// Process handlers
		if(s_pAceInterrupts[INTB_VERTB].pHandler) {
			s_pAceInterrupts[INTB_VERTB].pHandler(
				g_pCustom, s_pAceInterrupts[INTB_VERTB].pData
			);
		}
		uwReqClr = INTF_VERTB;
	}

	// Copper
	if((uwIntReq & INTF_COPER) && s_pAceInterrupts[INTB_COPER].pHandler) {
		s_pAceInterrupts[INTB_COPER].pHandler(
			g_pCustom, s_pAceInterrupts[INTB_VERTB].pData
		);
		uwReqClr |= INTF_COPER;
	}

	// Blitter
	if((uwIntReq & INTF_BLIT) && s_pAceInterrupts[INTB_BLIT].pHandler) {
		s_pAceInterrupts[INTB_BLIT].pHandler(
			g_pCustom, s_pAceInterrupts[INTB_VERTB].pData
		);
		uwReqClr |= INTF_BLIT;
	}
	logPopInt();
	g_pCustom->intreq = uwReqClr;
	g_pCustom->intreq = uwReqClr;
}

FN_HOTSPOT
void HWINTERRUPT int4Handler(void) {
	logPushInt();
	UWORD uwIntReq = g_pCustom->intreqr;
	UWORD uwReqClr = 0;

	// Audio channel 0
	if((uwIntReq & INTF_AUD0) && s_pAceInterrupts[INTB_AUD0].pHandler) {
		s_pAceInterrupts[INTB_AUD0].pHandler(
			g_pCustom, s_pAceInterrupts[INTB_AUD0].pData
		);
		uwReqClr |= INTF_AUD0;
	}

	// Audio channel 1
	if((uwIntReq & INTF_AUD1) && s_pAceInterrupts[INTB_AUD1].pHandler) {
		s_pAceInterrupts[INTB_AUD1].pHandler(
			g_pCustom, s_pAceInterrupts[INTB_AUD1].pData
		);
		uwReqClr |= INTF_AUD1;
	}

	// Audio channel 2
	if((uwIntReq & INTF_AUD2) && s_pAceInterrupts[INTB_AUD2].pHandler) {
		s_pAceInterrupts[INTB_AUD2].pHandler(
			g_pCustom, s_pAceInterrupts[INTB_AUD2].pData
		);
		uwReqClr |= INTF_AUD2;
	}

	// Audio channel 3
	if((uwIntReq & INTF_AUD3) && s_pAceInterrupts[INTB_AUD3].pHandler) {
		s_pAceInterrupts[INTB_AUD3].pHandler(
			g_pCustom, s_pAceInterrupts[INTB_AUD3].pData
		);
		uwReqClr |= INTF_AUD3;
	}
	logPopInt();
	g_pCustom->intreq = uwReqClr;
	g_pCustom->intreq = uwReqClr;
}

FN_HOTSPOT
void HWINTERRUPT int5Handler(void) {
	logPushInt();
	// DskSyn / RBF
	logPopInt();
}

FN_HOTSPOT
void HWINTERRUPT int6Handler(void) {
	logPushInt();
	// TODO: perhaps I should disable CIA interrupts while I process them?
	UWORD uwIntReq = g_pCustom->intreqr;
	UWORD uwReqClr = 0;

	if(uwIntReq & INTF_EXTER) {
		// INTF_EXTER - CIA B
		// Check and clear CIA interrupt flags
		UBYTE ubIcr = g_pCia[CIA_B]->icr;
		if(ubIcr & CIAICRF_SERIAL) {
			if(s_pAceCiaInterrupts[CIA_B][CIAICRB_SERIAL].pHandler) {
				s_pAceCiaInterrupts[CIA_B][CIAICRB_SERIAL].pHandler(
					g_pCustom, s_pAceCiaInterrupts[CIA_B][CIAICRB_SERIAL].pData
				);
			}
		}
		if(ubIcr & CIAICRF_TIMER_A) {
			if(s_pAceCiaInterrupts[CIA_B][CIAICRB_TIMER_A].pHandler) {
				s_pAceCiaInterrupts[CIA_B][CIAICRB_TIMER_A].pHandler(
					g_pCustom, s_pAceCiaInterrupts[CIA_B][CIAICRB_TIMER_A].pData
				);
			}
		}
		if(ubIcr & CIAICRF_TIMER_B) {
			if(s_pAceCiaInterrupts[CIA_B][CIAICRB_TIMER_B].pHandler) {
				s_pAceCiaInterrupts[CIA_B][CIAICRB_TIMER_B].pHandler(
					g_pCustom, s_pAceCiaInterrupts[CIA_B][CIAICRB_TIMER_B].pData
				);
			}
		}

		// Zorro/Clockport/other expansions can trigger this interrupt and it sets
		// the EXTER bit too. But you can't reliably detect and handle those devices
		// unless you exactly know that they are present and how to poll their status.
		// Leave the EXTER bit check in place since it should be cleared anyway.
		uwReqClr |= INTF_EXTER;
	}

	logPopInt();
	// Clear int flags
	g_pCustom->intreq = uwReqClr;
	g_pCustom->intreq = uwReqClr;
}

FN_HOTSPOT
void HWINTERRUPT int7Handler(void) {
	logPushInt();
	// EXTERNAL
	logPopInt();
}

//-------------------------------------------------------------------- FUNCTIONS

// Messageport creation for KS1.3
// http://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node02EC.html
static struct MsgPort *msgPortCreate(char *name, LONG pri) {
	LONG sigBit;
	struct MsgPort *mp;

	if((sigBit = AllocSignal(-1L)) == -1) {
		return NULL;
	}

	mp = (struct MsgPort *) AllocMem(sizeof(*mp), MEMF_PUBLIC | MEMF_CLEAR);
	if (!mp) {
		FreeSignal(sigBit);
		return NULL;
	}
	mp->mp_Node.ln_Name = name;
	mp->mp_Node.ln_Pri = pri;
	mp->mp_Node.ln_Type = NT_MSGPORT;
	mp->mp_Flags = PA_SIGNAL;
	mp->mp_SigBit = sigBit;
	mp->mp_SigTask = (struct Task *)s_pProcess;

	// NewList(&(mp->mp_MsgList)); // Init message list - not in headers
	mp->mp_MsgList.lh_Head = (struct Node*)&mp->mp_MsgList.lh_Tail;
	mp->mp_MsgList.lh_Tail = NULL;
	mp->mp_MsgList.lh_TailPred = (struct Node*)&mp->mp_MsgList.lh_Head;
	return mp;
}

// http://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node02ED.html
static void msgPortDelete(struct MsgPort *mp) {
	mp->mp_SigTask = (struct Task *) -1; // Make it difficult to re-use the port
	mp->mp_MsgList.lh_Head = (struct Node *) -1;

	FreeSignal(mp->mp_SigBit);
	FreeMem(mp, sizeof(*mp));
}

static void ioRequestInitialize(struct IORequest *pIoReq, struct MsgPort *pMsgPort, UWORD uwLen) {
	pIoReq->io_Message.mn_Node.ln_Type = NT_MESSAGE;
	pIoReq->io_Message.mn_ReplyPort = pMsgPort;
	pIoReq->io_Message.mn_Length = uwLen;
}

static struct IORequest *ioRequestCreate(struct MsgPort *pMsgPort, UWORD uwLen) {
	struct IORequest *pIoReq = (struct IORequest *) AllocMem(uwLen, MEMF_PUBLIC | MEMF_CLEAR);
	if (!pIoReq) {
		return NULL;
	}
	ioRequestInitialize(pIoReq, pMsgPort, uwLen);
	return pIoReq;
}

static void ioRequestDestroy(struct IORequest *pIoReq) {
	FreeMem(pIoReq, pIoReq->io_Message.mn_Length);
}

static UBYTE audioChannelAlloc(struct IOAudio *pIoAudio) {
	struct MsgPort *pMsgPort = 0;

	pMsgPort = msgPortCreate("audio alloc", ADALLOC_MAXPREC);
	if(!pMsgPort) {
		logWrite("ERR: Couldn't open message port for audio alloc\n");
		goto fail;
	}

	memset(pIoAudio, 0, sizeof(*pIoAudio));
	ioRequestInitialize(&pIoAudio->ioa_Request, pMsgPort, sizeof(*pIoAudio));

	UBYTE isError = OpenDevice(
		(CONST_STRPTR)"audio.device", 0, (struct IORequest *)pIoAudio, 0
	);
	if(isError) {
		logWrite(
			"ERR: Couldn't alloc Audio channels, code: %d\n",
			pIoAudio->ioa_Request.io_Error
		);
		goto fail;
	}

	UBYTE ubChannelMask = 0b1111; // Allocate all 4 channels.
	pIoAudio->ioa_Data = &ubChannelMask;
	pIoAudio->ioa_Length = sizeof(ubChannelMask);
	pIoAudio->ioa_Request.io_Command = ADCMD_ALLOCATE;
	pIoAudio->ioa_Request.io_Flags = ADIOF_NOWAIT;
	isError = DoIO((struct IORequest *)pIoAudio);

	if(isError) {
		logWrite(
			"ERR: io audio request fail, code: %d\n", pIoAudio->ioa_Request.io_Error
		);
		goto fail;
	}
	return 1;

fail:
	if(pMsgPort) {
		msgPortDelete(pMsgPort);
	}
	return 0;
}

void audioChannelFree(struct IOAudio *pIoAudio) {
	struct MsgPort *pMsgPort = pIoAudio->ioa_Request.io_Message.mn_ReplyPort;

	pIoAudio->ioa_Request.io_Command = ADCMD_FINISH;
	pIoAudio->ioa_Request.io_Flags = 0;
	pIoAudio->ioa_Request.io_Unit = (APTR)1;
	UBYTE isError = DoIO((struct IORequest *)pIoAudio);
	if(isError) {
		logWrite(
			"ERR: io audio request fail, code: %d\n", pIoAudio->ioa_Request.io_Error
		);
	}

	CloseDevice((struct IORequest *)pIoAudio);
	msgPortDelete(pMsgPort);
}

static void cdtvPortAlloc(void) {
	if (!(s_pCdtvIOPort = msgPortCreate(NULL, ADALLOC_MAXPREC))) {
		logWrite("ERR: Couldn't allocate message port for cdtv alloc\n");
		return;
	}

	if (!(s_pCdtvIOReq = (struct IOStdReq *)ioRequestCreate(s_pCdtvIOPort, sizeof(struct IOStdReq)))) {
		logWrite("ERR: Couldn't allocate io request for cdtv messages\n");
		msgPortDelete(s_pCdtvIOPort);
		s_pCdtvIOPort = NULL;
		return;
	}

	if (OpenDevice((CONST_STRPTR)"cdtv.device", 0, (struct IORequest *) s_pCdtvIOReq, 0)) {
		msgPortDelete(s_pCdtvIOPort);
		s_pCdtvIOPort = NULL;
		ioRequestDestroy((struct IORequest*)s_pCdtvIOReq);
		s_pCdtvIOReq = NULL;
		return;
	}
}

static void cdtvPortFree(void) {
	if (s_pCdtvIOPort) {
		msgPortDelete(s_pCdtvIOPort);
	}
	if (s_pCdtvIOReq) {
		ioRequestDestroy((struct IORequest *)s_pCdtvIOReq);
	}
}

/**
 * @brief Flushes FDD disk activity.
 * Originally written by Asman in asm, known as osflush
 * Rewritten by Asman for issue #76, reformatted for ACE codestyle
 * More explanation about how it works:
 * http://eab.abime.net/showthread.php?t=87202&page=2
 */
static void systemFlushIo(void) {
	struct StandardPacket *pPacket = (struct StandardPacket*)AllocMem(
		sizeof(struct StandardPacket), MEMF_CLEAR
	);

	if(!pPacket) {
		return;
	}

	// Get filesystem message port
	struct MsgPort *pMsgPort = DeviceProc((CONST_STRPTR)"sys");
	if (pMsgPort) {
		// Get our message port
		struct MsgPort *pProcessMsgPort = &s_pProcess->pr_MsgPort;

		// Fill in packet
		struct DosPacket *pDosPacket = &pPacket->sp_Pkt;
		struct Message *pMsg = &pPacket->sp_Msg;

		// It is how Tripos packet system was hacked to exec messaging system
		pMsg->mn_Node.ln_Name = (char*)pDosPacket;
		pMsg->mn_ReplyPort = pProcessMsgPort;

		pDosPacket->dp_Link = pMsg;
		pDosPacket->dp_Port = pProcessMsgPort;
		pDosPacket->dp_Type = ACTION_FLUSH;

		// Send packet
		PutMsg(pMsgPort, pMsg);

		// Wait for reply
		WaitPort(pProcessMsgPort);
		GetMsg(pProcessMsgPort);
	}

	FreeMem(pPacket, sizeof(struct StandardPacket));
}

void systemKill(const char *szMsg) {
	logWrite("ERR: ACE SYSKILL: '%s'", szMsg);
	if(DOSBase) {
		BPTR lOut = Output();
		Write(lOut, "ERR: ACE SYSKILL: ", sizeof("ERR: ACE SYSKILL: "));
		Write(lOut, (char*)szMsg, strlen(szMsg));
	}

	if(GfxBase) {
		CloseLibrary((struct Library *) GfxBase);
	}
	if(DOSBase) {
		CloseLibrary((struct Library *) DOSBase);
	}
	exit(EXIT_FAILURE);
}

void systemCreate(void) {
#if defined(BARTMAN_GCC)
	// Bartman's startup code doesn't initialize anything
	SysBase = *((struct ExecBase**)4UL);
#endif

	DOSBase = (struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library", 0);
	if (!DOSBase) {
		systemKill("Can't open DOS Library\n");
		return;
	}

	GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 0L);
	if (!GfxBase) {
		systemKill("Can't open Gfx Library\n");
		return;
	}

	// Determine original stack size
	s_pProcess = (struct Process *)FindTask(NULL);
	char *pStackLower = (char *)s_pProcess->pr_Task.tc_SPLower;
	ULONG ulStackSize = (char *)s_pProcess->pr_Task.tc_SPUpper - pStackLower;
	if(s_pProcess->pr_CLI) {
		ulStackSize = *(ULONG *)s_pProcess->pr_ReturnAddr;
	}
	logWrite("Stack size: %lu\n", ulStackSize);
	*pStackLower = SYSTEM_STACK_CANARY;

	// Reserve all audio channels - apparantly this allows for int flag polling
	// From https://gist.github.com/johngirvin/8fb0c4bb83b7c80427e2f439bb074e95
	audioChannelAlloc(&s_sIoAudio);

	// prepare disabling/enabling CDTV device as per Alpine9000 code
	// from https://eab.abime.net/showthread.php?t=89836
	cdtvPortAlloc();

	s_pCiaResource[CIA_A] = OpenResource((CONST_STRPTR)CIAANAME);
	s_pCiaResource[CIA_B] = OpenResource((CONST_STRPTR)CIABNAME);

	// Disable as much of OS stuff as possible so that it won't trash stuff when
	// re-enabled periodically.
	// Save the system copperlists and flush the view

	s_pOsView = GfxBase->ActiView;
	WaitTOF();
	LoadView(0);
	WaitTOF();
	WaitTOF(); // Wait for interlaced screen to finish

	// get VBR location on 68010+ machine
	// http://eab.abime.net/showthread.php?t=65430&page=3
	if (SysBase->AttnFlags & AFF_68010) {
		s_pHwVectors = (tHwIntVector *)Supervisor((void *)s_pGetVbrCode);
	}

	// Finish disk activity
	systemFlushIo();

	// save the state of the hardware registers (INTENA, DMA, ADKCON etc.)
	s_uwOsIntEna = g_pCustom->intenar;
	s_uwOsDmaCon = g_pCustom->dmaconr;
	s_uwOsInitialDma = s_uwOsDmaCon;

	// Disable interrupts (this is the actual "kill system/OS" part)
	g_pCustom->intena = 0x7FFF;
	g_pCustom->intreq = 0x7FFF;

	// Disable all DMA - only once
	// Wait for vbl before disabling sprite DMA
	while (!(g_pCustom->intreqr & INTF_VERTB)) continue;
	g_pCustom->dmacon = 0x07FF;

	// Unuse system so that it gets backed up once and then re-enable
	// as little as needed
	s_wSystemUses = 1;
	s_wSystemBlitterUses = 1;
	systemUnuse();
	systemUse();

	systemGetBlitterFromOs();
}

void systemDestroy(void) {
	// Disable all interrupts
	g_pCustom->intena = 0x7FFF;
	g_pCustom->intreq = 0x7FFF;

	// Wait for vbl before disabling sprite DMA
	while (!(g_pCustom->intreqr & INTF_VERTB)) continue;
	g_pCustom->dmacon = 0x07FF;
	g_pCustom->intreq = 0x7FFF;

	// Start waking up OS
	if(s_wSystemUses != 1) {
		logWrite("ERR: unclosed/overclosed system usage count: %hd\n", s_wSystemUses);
		// Ensure that OS gets restured - along with
	}

	// Restore every single OS DMA & interrupt
	s_wSystemUses = 0;
	systemUse();
	systemReleaseBlitterToOs();
	g_pCustom->dmacon = DMAF_SETCLR | DMAF_MASTER | s_uwOsInitialDma;

	// Free audio channels
	audioChannelFree(&s_sIoAudio);

	// Free CDTV request port
	cdtvPortFree();

	// restore old view
	WaitTOF();
	LoadView(s_pOsView);
	WaitTOF();

	systemCheckStack();

	logWrite("Closing graphics.library...\n");
	CloseLibrary((struct Library *) GfxBase);
	logWrite("Closing dos.library...\n");
	CloseLibrary((struct Library *) DOSBase);
}

void systemUnuse(void) {
	if(s_wSystemUses == 1) {
		// Do before counter is decreased, otherwise it'll fall into infinite loop!
		logWrite("Turning off the system...\n");
	}

	--s_wSystemUses;
	if(!s_wSystemUses) {
		if(g_pCustom->dmaconr & DMAF_DISK) {
			// Flush disk activity if it was used
			// This 'if' is here because otherwise systemUnuse() called
			// by systemCreate() would indefinitely wait for OS when it's killed.
			// systemUse() restores disk DMA, so it's an easy check if OS was
			// actually restored.
			systemFlushIo();
		}

		// Disable CDTV CD drive if present
		if (s_pCdtvIOReq) {
			s_pCdtvIOReq->io_Command = CMD_STOP;
			s_pCdtvIOReq->io_Offset  = 0;
			s_pCdtvIOReq->io_Length  = 0;
			s_pCdtvIOReq->io_Data    = NULL;
			DoIO((struct IORequest *)s_pCdtvIOReq);
		}

		// Disable interrupts (this is the actual "kill system/OS" part)
		g_pCustom->intena = 0x7FFF;
		g_pCustom->intreq = 0x7FFF;

		// Query CIA ICR bits set by OS for later CIA takeover restore
		s_pOsCiaIcr[CIA_A] = AbleICR(s_pCiaResource[CIA_A], 0);
		s_pOsCiaIcr[CIA_B] = AbleICR(s_pCiaResource[CIA_B], 0);

		// Disable CIA interrupts
		g_pCia[CIA_A]->icr = 0x7F;
		g_pCia[CIA_B]->icr = 0x7F;

		// Save CRA bits
		s_pOsCiaCra[CIA_A] = g_pCia[CIA_A]->cra;
		s_pOsCiaCrb[CIA_A] = g_pCia[CIA_A]->crb;
		s_pOsCiaCra[CIA_B] = g_pCia[CIA_B]->cra;
		s_pOsCiaCrb[CIA_B] = g_pCia[CIA_B]->crb;

		// Disable timers and trigger reload of value to read preset val
		g_pCia[CIA_A]->cra = CIACRA_LOAD; // CIACRA_START=0, timer is stopped
		g_pCia[CIA_A]->crb = CIACRB_LOAD;
		g_pCia[CIA_B]->cra = CIACRA_LOAD; // CIACRA_START=0, timer is stopped
		g_pCia[CIA_B]->crb = CIACRB_LOAD;

		// Save OS CIA timer values
		s_pOsCiaTimerA[CIA_A] = ciaGetTimerA(g_pCia[CIA_A]);
		s_pOsCiaTimerB[CIA_A] = ciaGetTimerB(g_pCia[CIA_A]);
		s_pOsCiaTimerA[CIA_B] = ciaGetTimerA(g_pCia[CIA_B]);
		s_pOsCiaTimerB[CIA_B] = ciaGetTimerB(g_pCia[CIA_B]);

		// set ACE CIA timers
		ciaSetTimerA(g_pCia[CIA_A], s_pAceCiaTimerA[CIA_A]);
		ciaSetTimerB(g_pCia[CIA_A], s_pAceCiaTimerB[CIA_A]);
		ciaSetTimerA(g_pCia[CIA_B], s_pAceCiaTimerA[CIA_B]);
		ciaSetTimerB(g_pCia[CIA_B], s_pAceCiaTimerB[CIA_B]);

		// Enable ACE CIA interrupts
		g_pCia[CIA_A]->icr = (
			CIAICRF_SETCLR | CIAICRF_SERIAL | CIAICRF_TIMER_A | CIAICRF_TIMER_B
		);
		g_pCia[CIA_B]->icr = (
			CIAICRF_SETCLR | CIAICRF_SERIAL | CIAICRF_TIMER_A | CIAICRF_TIMER_B
		);

		// Restore ACE CIA CRA/CRB state
		g_pCia[CIA_A]->cra = CIACRA_LOAD | s_pAceCiaCra[CIA_A];
		g_pCia[CIA_A]->crb = CIACRA_LOAD | s_pAceCiaCrb[CIA_A];
		g_pCia[CIA_B]->cra = CIACRA_LOAD | s_pAceCiaCra[CIA_B];
		g_pCia[CIA_B]->crb = CIACRA_LOAD | s_pAceCiaCrb[CIA_B];

		// Game's bitplanes & copperlists are still used so don't disable them
		// Wait for vbl before disabling sprite DMA
		while (!(g_pCustom->intreqr & INTF_VERTB)) continue;
		g_pCustom->dmacon = s_uwOsMinDma;

		// Save OS interrupt vectors and enable ACE's
		g_pCustom->intreq = 0x7FFF;
		for(UWORD i = 0; i < SYSTEM_INT_VECTOR_COUNT; ++i) {
			s_pOsHwInterrupts[i] = s_pHwVectors[SYSTEM_INT_VECTOR_FIRST + i];
			s_pHwVectors[SYSTEM_INT_VECTOR_FIRST + i] = s_pAceHwInterrupts[i];
		}

		// Enable needed DMA (and interrupt) channels
		g_pCustom->dmacon = DMAF_SETCLR | DMAF_MASTER | s_uwAceDmaCon;
		// Everything that's supported by ACE to simplify things for now,
		// but not audio channels since ptplayer relies on polling them, and I'm not
		// currently being able to debug audio interrupt handler variant.
		g_pCustom->intena = INTF_SETCLR | INTF_INTEN | s_uwAceIntEna;

		// HACK: OS resets potgo in vblank int, preventing scanning fire2 on most joysticks.
		// TODO: make sure this doesn't break anything, e.g. mice
		// TODO: add setting this value to OS-friendly vblank handler with proper priority
		// https://eab.abime.net/showthread.php?t=74981
		g_pCustom->potgo = 0xFF00;
	}
#if defined(ACE_DEBUG)
	if(s_wSystemUses < 0) {
		logWrite("ERR: System uses less than 0\n");
		s_wSystemUses = 0;
	}
#endif
}

void systemUse(void) {
	if(!s_wSystemUses) {
		// Disable app interrupts/dma, keep display-related DMA
		g_pCustom->intena = 0x7FFF;
		g_pCustom->intreq = 0x7FFF;
		g_pCustom->dmacon = s_uwOsMinDma;
		while(!(g_pCustom->intreqr & INTF_VERTB)) continue;

		// Disable CIA interrupts
		g_pCia[CIA_A]->icr = 0x7F;
		g_pCia[CIA_B]->icr = 0x7F;

		// Restore interrupt vectors - also CIA
		for(UWORD i = 0; i < SYSTEM_INT_VECTOR_COUNT; ++i) {
			s_pHwVectors[SYSTEM_INT_VECTOR_FIRST + i] = s_pOsHwInterrupts[i];
		}

		// Stop the timers
		g_pCia[CIA_A]->cra = 0;
		g_pCia[CIA_A]->crb = 0;
		g_pCia[CIA_B]->cra = 0;
		g_pCia[CIA_B]->crb = 0;

		// Restore old CIA timer values
		ciaSetTimerA(g_pCia[CIA_A], s_pOsCiaTimerA[CIA_A]);
		ciaSetTimerB(g_pCia[CIA_A], s_pOsCiaTimerB[CIA_A]);
		ciaSetTimerA(g_pCia[CIA_B], s_pOsCiaTimerA[CIA_B]);
		ciaSetTimerB(g_pCia[CIA_B], s_pOsCiaTimerB[CIA_B]);

		// Restore OS's CIA interrupts
		// According to UAE debugger there's nothing in CIA_A
		g_pCia[CIA_A]->icr = CIAICRF_SETCLR | s_pOsCiaIcr[CIA_A];
		g_pCia[CIA_B]->icr = CIAICRF_SETCLR | s_pOsCiaIcr[CIA_B];

		// Restore CRA/CRB registers, load the timers again
		g_pCia[CIA_A]->cra = CIACRA_LOAD | s_pOsCiaCra[CIA_A];
		g_pCia[CIA_A]->crb = CIACRB_LOAD | s_pOsCiaCrb[CIA_A];
		g_pCia[CIA_B]->cra = CIACRA_LOAD | s_pOsCiaCra[CIA_B];
		g_pCia[CIA_B]->crb = CIACRB_LOAD | s_pOsCiaCrb[CIA_B];

		// restore old DMA/INTENA/ADKCON etc. settings
		// All interrupts but only needed DMA
		g_pCustom->dmacon = DMAF_SETCLR | DMAF_MASTER | (s_uwOsDmaCon & s_uwOsMinDma);
		g_pCustom->intena = INTF_SETCLR | INTF_INTEN  | s_uwOsIntEna;

		// Nasty keyboard hack - if any key gets pressed / released while system is
		// inactive, we won't be able to catch it.
		keyReset();

		// Re-enable CDTV CD drive if present
		if (s_pCdtvIOReq) {
			s_pCdtvIOReq->io_Command = CMD_START;
			s_pCdtvIOReq->io_Offset  = 0;
			s_pCdtvIOReq->io_Length  = 0;
			s_pCdtvIOReq->io_Data    = NULL;
			DoIO((struct IORequest *)s_pCdtvIOReq);
		}
	}
	++s_wSystemUses;

	if(s_wSystemUses == 1) {
		// It should be "turned" but I prefer the message being consistent.
		logWrite("Turning on the system...\n");
	}
}

UBYTE systemIsUsed(void) {
	return s_wSystemUses > 0;
}

void systemGetBlitterFromOs(void) {
	--s_wSystemBlitterUses;
	if(!s_wSystemBlitterUses) {
		// Make OS finish its pending operations before it loses blitter!
		systemFlushIo();
		OwnBlitter();
		WaitBlit();
	}

#if defined(ACE_DEBUG)
	if(s_wSystemBlitterUses < 0) {
		logWrite("ERR: System Blitter uses less than 0\n");
		s_wSystemUses = 0;
	}
#endif
}

void systemReleaseBlitterToOs(void) {
	if (!s_wSystemBlitterUses){
		DisownBlitter();
		WaitBlit();
	}
	++s_wSystemBlitterUses;
}

UBYTE systemBlitterIsUsed(void) {
	return s_wSystemBlitterUses > 0;
}

void systemSetInt(
	UBYTE ubIntNumber, tAceIntHandler pHandler, void *pIntData
) {
	// Disable interrupt during data swap to not get stuck inside ACE's ISR
	if(!s_wSystemUses) {
		g_pCustom->intena = BV(ubIntNumber);
	}

	// Disable handler or re-enable it if 0 was passed
	if(pHandler == 0) {
		s_pAceInterrupts[ubIntNumber].pHandler = 0;
		s_uwAceIntEna &= ~BV(ubIntNumber);
	}
	else {
		s_pAceInterrupts[ubIntNumber].pHandler = pHandler;
		s_pAceInterrupts[ubIntNumber].pData = pIntData;
		s_uwAceIntEna |= BV(ubIntNumber);
		if(!s_wSystemUses) {
			g_pCustom->intena = INTF_SETCLR | BV(ubIntNumber);
		}
	}
}

void systemSetCiaInt(
	UBYTE ubCia, UBYTE ubIntBit, tAceIntHandler pHandler, void *pIntData
) {
	// Disable ACE handler during data swap to ensure atomic op
	s_pAceCiaInterrupts[ubCia][ubIntBit].pHandler = 0;
	s_pAceCiaInterrupts[ubCia][ubIntBit].pData = pIntData;

	// Re-enable handler or disable it if 0 was passed
	s_pAceCiaInterrupts[ubCia][ubIntBit].pHandler = pHandler;
}

void systemSetCiaCr(UBYTE ubCia, UBYTE isCrB, UBYTE ubCrValue) {
	if(isCrB) {
		s_pAceCiaCrb[ubCia] = ubCrValue;
		if(!s_wSystemUses) {
			g_pCia[ubCia]->crb = ubCrValue;
		}
	}
	else {
		s_pAceCiaCra[ubCia] = ubCrValue;
		if(!s_wSystemUses) {
			g_pCia[ubCia]->cra = ubCrValue;
		}
	}
}

void systemSetDmaBit(UBYTE ubDmaBit, UBYTE isEnabled) {
	UWORD uwDmaFlag = BV(ubDmaBit);
	systemSetDmaMask(uwDmaFlag, isEnabled);
}

void systemSetDmaMask(UWORD uwDmaMask, UBYTE isEnabled) {
	if(isEnabled) {
		s_uwAceDmaCon |= uwDmaMask;
		s_uwOsDmaCon |= uwDmaMask;
		if(!s_wSystemUses || !(uwDmaMask & s_uwOsMinDma)) {
			// Enable right now if OS is asleep or it's not critical for it to live
			g_pCustom->dmacon = DMAF_SETCLR | uwDmaMask;
		}
	}
	else {
		s_uwAceDmaCon &= ~uwDmaMask;
		if(!(uwDmaMask & s_uwOsMinDma)) {
			s_uwOsDmaCon &= ~uwDmaMask;
		}
		if(!s_wSystemUses || !(uwDmaMask & s_uwOsMinDma)) {
			// Disable right now if OS is asleep or it's not critical for it to live
			g_pCustom->dmacon = uwDmaMask;
		}
	}
}

void systemSetTimer(UBYTE ubCia, UBYTE ubTimer, UWORD uwTicks) {
	// Save timer values since backing them up on the fly in continuous mode
	// is impossible
	UWORD *pTimers = ((ubTimer == 0) ? s_pAceCiaTimerA : s_pAceCiaTimerB);
	pTimers[ubCia] = uwTicks;

	if(!s_wSystemUses) {
		// OS is already dead - set CIA timer right now
		if(!ubTimer) {
			ciaSetTimerA(g_pCia[ubCia], uwTicks);
		}
		else {
			ciaSetTimerB(g_pCia[ubCia], uwTicks);
		}
	}
}

void systemDump(void) {
	// logBlockBegin("systemDump()");

	logWrite("OS Usage counter: %hd\n", s_wSystemUses);

	// Print handlers
	// logWrite("ACE handlers:\n");
	// for(UWORD i = 0; i < SYSTEM_INT_HANDLER_COUNT; ++i) {
	// 	logWrite(
	// 		"Int %hu: code %p, data %p\n",
	// 		i, s_pAceInterrupts[i].pHandler, s_pAceInterrupts[i].pData
	// 	);
	// }

	// Print vectors
	// logWrite("Vectors:\n");
	// for(UWORD i = 0; i < SYSTEM_INT_VECTOR_COUNT; ++i) {
	// 	logWrite(
	// 		"vec %hu: ACE %p, OS %p, curr %p\n",
	// 		i, s_pAceHwInterrupts[i], s_pOsHwInterrupts[i],
	// 		s_pHwVectors[SYSTEM_INT_VECTOR_FIRST+ i]
	// 	);
	// }

	// logBlockEnd("systemDump()");
}

void systemIdleBegin(void) {
#if defined(BARTMAN_GCC)
	debug_start_idle();
#endif
}

void systemIdleEnd(void) {
#if defined(BARTMAN_GCC)
	debug_stop_idle();
#endif
}

UBYTE systemGetVerticalBlankFrequency(void){
	return SysBase->VBlankFrequency;
}

UBYTE systemIsPal(void) {

	UBYTE ubVBlankFreq = systemGetVerticalBlankFrequency();
	if (ubVBlankFreq == 60) {
		return 0; // NTSC
	}

	return 1; // PAL
}

void systemCheckStack(void) {
	char *pStackLower = (char *)s_pProcess->pr_Task.tc_SPLower;
	register ULONG *pCurrentStackPos __asm("sp");

	if(*pStackLower != SYSTEM_STACK_CANARY) {
		logWrite("ERR: Stack has probably overflown");
		while(1) {}
	}

	if((ULONG)pCurrentStackPos < (ULONG)(pStackLower)) {
		logWrite("ERR: out of stack bounds\n");
		while(1) {}
	}
}

UWORD systemGetVersion(void) {
	return SysBase->LibNode.lib_Version;
}
