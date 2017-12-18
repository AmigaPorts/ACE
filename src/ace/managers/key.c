#include <ace/managers/key.h>

#include <ace/utils/custom.h>
#define KEY_RELEASED_BIT 1

/**
 * Timer VBlank server
 * Increments frame counter
 */
__amigainterrupt __saveds void keyIntServer(__reg("a1") tKeyManager *pManager) {
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
#ifdef AMIGA
	g_sKeyManager.pInt = memAllocChipClear(sizeof(struct Interrupt)); // CHIP is PUBLIC.

	g_sKeyManager.pInt->is_Node.ln_Type = NT_INTERRUPT;
	g_sKeyManager.pInt->is_Node.ln_Pri = -60;
	g_sKeyManager.pInt->is_Node.ln_Name = "ACE_Keyboard_CIA";
	g_sKeyManager.pInt->is_Data = (APTR)&g_sKeyManager;
	g_sKeyManager.pInt->is_Code = keyIntServer;

	AddIntServer(INTB_PORTS, g_sKeyManager.pInt);
#endif // AMIGA
}

void keyDestroy(void) {
#ifdef AMIGA
	RemIntServer(INTB_PORTS, g_sKeyManager.pInt);
	memFree(g_sKeyManager.pInt, sizeof(struct Interrupt));
#endif // AMIGA
}

/* Functions */
void keyProcess(void) {
	// This function is left out for other platforms - they will prob'ly not be
	// interrupt-driven.
}

void keySetState(UBYTE ubKeyCode, UBYTE ubKeyState) {
	g_sKeyManager.pStates[ubKeyCode] = ubKeyState;
	if(ubKeyState == KEY_ACTIVE)
		g_sKeyManager.ubLastKey = ubKeyCode;
}

UBYTE keyCheck(UBYTE ubKeyCode) {
	return g_sKeyManager.pStates[ubKeyCode] != KEY_NACTIVE;
}

UBYTE keyUse(UBYTE ubKeyCode) {
	if (g_sKeyManager.pStates[ubKeyCode] == KEY_ACTIVE) {
		g_sKeyManager.pStates[ubKeyCode] = KEY_USED;
		return 1;
	}
	return 0;
}
