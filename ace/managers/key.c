#include <ace/managers/key.h>

/* Globals */
tKeyManager g_sKeyManager;

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