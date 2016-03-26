#include <ace/managers/key.h>

/* Globals */
tKeyManager g_sKeyManager;

const UBYTE g_pFromAscii[] = { 0
	
};
const UBYTE g_pToAscii[] = {
	'`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '|', '', '0',
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '1', '2', '3',
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '4', '5', '6',
};

/* Functions */
void keySetState(UBYTE ubKeyCode, UBYTE ubKeyState) {
	g_sKeyManager.pStates[ubKeyCode] = ubKeyState;
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