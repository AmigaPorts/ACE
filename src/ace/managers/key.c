#include <ace/managers/key.h>
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/utils/custom.h>
#include <hardware/intbits.h> // INTB_PORTS
#define KEY_RELEASED_BIT 1

/**
 * Timer VBlank server
 * Increments frame counter
 */
FN_HOTSPOT
void INTERRUPT keyIntServer(
	UNUSED_ARG REGARG(volatile tCustom *pCustom, "a0"),
	UNUSED_ARG REGARG(volatile void *pData, "a1")
) {
	UBYTE ubKeyCode = ~g_pCiaA->sdr;

	// Start handshake
	g_pCiaA->cra |= CIACRA_SPMODE;
	UWORD uwStart = (g_pCiaA->tbhi << 8) | g_pCiaA->tblo;

	// Get keypress flag and shift keyCode
	UBYTE ubKeyReleased = ubKeyCode & KEY_RELEASED_BIT;
	ubKeyCode >>= 1;

	if(ubKeyReleased)
		keySetState(ubKeyCode, KEY_NACTIVE);
	else if (!keyCheck(ubKeyCode))
		keySetState(ubKeyCode, KEY_ACTIVE);

	// End handshake
	while(uwStart - ((g_pCiaA->tbhi << 8) | g_pCiaA->tblo) < 65)
		continue;
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
	systemSetInt(INTB_PORTS, keyIntServer, 0);
}

void keyDestroy(void) {
	systemSetInt(INTB_PORTS, 0, 0);
}

void keyProcess(void) {
	// This function is left out for other platforms - they will prob'ly not be
	// interrupt-driven.
}
