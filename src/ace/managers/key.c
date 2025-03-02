/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/key.h>
#include <ace/managers/log.h>
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/utils/custom.h>
#include <hardware/intbits.h> // INTB_PORTS
#define KEY_INTERRUPT_RELEASED_BIT 1
#define KEY_INPUT_HANDLER_RELEASED_MASK BV(7)

#if defined ACE_DEBUG
static UBYTE s_bInitCount = 0;
#endif

static inline void keyIntSetState(
	tKeyManager *pManager, UBYTE ubKeyCode, UBYTE ubKeyState
) {
	pManager->pStates[ubKeyCode] = ubKeyState;
	if(ubKeyState == KEY_ACTIVE) {
		pManager->ubLastKey = ubKeyCode;
	}
}

static inline UBYTE keyIntCheck(const tKeyManager *pManager, UBYTE ubKeyCode) {
	return pManager->pStates[ubKeyCode] != KEY_NACTIVE;
}

UBYTE keyCheck(UBYTE ubKeyCode) {
	return keyIntCheck(&g_sKeyManager, ubKeyCode);
}

void keySetState(UBYTE ubKeyCode, UBYTE ubKeyState) {
	keyIntSetState(&g_sKeyManager, ubKeyCode, ubKeyState);
}

void onRawKeyInput(UBYTE ubRawKey) {
	UBYTE isKeyReleased = ubRawKey & KEY_INPUT_HANDLER_RELEASED_MASK;
	ubRawKey &= ~KEY_INPUT_HANDLER_RELEASED_MASK;
	keySetState(ubRawKey, isKeyReleased ? KEY_NACTIVE : KEY_ACTIVE);
}

/**
 * Key interrupt server
 * Gets key press/release from kbd controller and confirms reception
 * by handshake
 */
FN_HOTSPOT
void INTERRUPT onKeyInterrupt(
	REGARG(volatile tCustom *pCustom, "a0"),
	REGARG(volatile void *pData, "a1")
) {
	tKeyManager *pKeyManager = (tKeyManager*)pData;
	volatile tRayPos *pRayPos = (tRayPos*)&pCustom->vposr;

	// Get the key code and start handshake
	UBYTE ubKeyCode = ~g_pCia[CIA_A]->sdr;
	g_pCia[CIA_A]->cra |= CIACRA_SPMODE;
	UWORD uwStart = pRayPos->bfPosY;

	// Get keypress flag and shift key code
	UBYTE ubKeyReleased = ubKeyCode & KEY_INTERRUPT_RELEASED_BIT;
	ubKeyCode >>= 1;
	keyIntSetState(
		pKeyManager, ubKeyCode, ubKeyReleased ? KEY_NACTIVE : KEY_ACTIVE
	);

	// End handshake
	UWORD uwDelta;
	do {
		UWORD uwEnd = pRayPos->bfPosY;
		if(uwEnd >= uwStart) {
			uwDelta = uwEnd - uwStart;
		}
		else {
			uwDelta = 0xFFFF - uwStart + uwEnd;
		}
	} while(uwDelta < 3);
	g_pCia[CIA_A]->cra &= ~CIACRA_SPMODE;
	INTERRUPT_END;
}

/* Globals */
tKeyManager g_sKeyManager;

const UBYTE g_pFromAscii[] = { 0

};
const UBYTE g_pToAscii[] = {
	'`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\\', '\0', '0',
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\0', '1', '2', '3',
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '\0', '\0', '4', '5', '6',
	'\0', 'z', 'x', 'c', 'v', 'b', 'n' ,'m', ',', '.', '/', '\b', '.', '7', '8', '9',
	' ', '\0', '\t', '\r', '\r', '\x1B', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '(', ')', '/', '*', '+', '-',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
};

void keyCreate(void) {
	logBlockBegin("keyCreate()");
#if defined(ACE_DEBUG)
	if(s_bInitCount++ != 0) {
		// You should call keyCreate() only once
		logWrite("ERR: Keyboard already initialized\n");
	}
#endif
	systemSetCiaInt(CIA_A, CIAICRB_SERIAL, onKeyInterrupt, &g_sKeyManager);
	systemSetKeyInputHandler(onRawKeyInput);
	logBlockEnd("keyCreate()");
}

void keyDestroy(void) {
	logBlockBegin("keyDestroy()");
#if defined(ACE_DEBUG)
	if(s_bInitCount-- != 1) {
		// You should call keyDestroy() only once for each keyCreate()
		logWrite("ERR: Keyboard was initialized multiple times\n");
	}
#endif
	systemSetCiaInt(CIA_A, CIAICRB_SERIAL, 0, 0);
	systemSetKeyInputHandler(0);
	logBlockEnd("keyDestroy()");
}

void keyProcess(void) {
	// This function is left out for other platforms - they will prob'ly not be
	// interrupt-driven.
}
