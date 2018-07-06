/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/key.h>
#include <ace/managers/log.h>
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/utils/custom.h>
#include <hardware/intbits.h> // INTB_PORTS
#define KEY_RELEASED_BIT 1

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

/**
 * Timer VBlank server
 * Increments frame counter
 */
FN_HOTSPOT
void INTERRUPT keyIntServer(
	UNUSED_ARG REGARG(volatile tCustom *pCustom, "a0"),
	REGARG(volatile void *pData, "a1")
) {
	tKeyManager *pKeyManager = (tKeyManager*)pData;

	UBYTE ubKeyCode = ~g_pCiaA->sdr;

	// Start handshake
	g_pCiaA->cra |= CIACRA_SPMODE;
	UWORD uwStart = ciaGetTimerB(g_pCiaA);

	// Get keypress flag and shift keyCode
	UBYTE ubKeyReleased = ubKeyCode & KEY_RELEASED_BIT;
	ubKeyCode >>= 1;

	if(ubKeyReleased) {
		keyIntSetState(pKeyManager, ubKeyCode, KEY_NACTIVE);
	}
	else {
		keyIntSetState(pKeyManager, ubKeyCode, KEY_ACTIVE);
	}

	// End handshake
	UWORD uwDelta;
	do {
		UWORD uwEnd = ciaGetTimerB(g_pCiaA);
		if(uwEnd > uwStart) {
			uwDelta = uwEnd - uwStart;
		}
		else {
			uwDelta = 0xFFFF - uwStart + uwEnd;
		}
	} while(uwDelta < 65);
	g_pCiaA->cra &= ~CIACRA_SPMODE;
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
};

void keyCreate(void) {
	logBlockBegin("keyCreate()");
	systemSetInt(INTB_PORTS, keyIntServer, &g_sKeyManager);
	logBlockEnd("keyCreate()");
}

void keyDestroy(void) {
	logBlockBegin("keyDestroy()");
	systemSetInt(INTB_PORTS, 0, 0);
	logBlockEnd("keyDestroy()");
}

void keyProcess(void) {
	// This function is left out for other platforms - they will prob'ly not be
	// interrupt-driven.
}
