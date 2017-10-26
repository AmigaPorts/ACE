#include <ace/managers/key.h>

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

/* Functions */
void keyProcess() {
	ULONG ulWindowSignal;
	ULONG ulSignals;
	struct IntuiMessage *pMsg;

	ulWindowSignal = 1L << g_sWindowManager.pWindow->UserPort->mp_SigBit;
	ulSignals = SetSignal(0L, 0L);

	if (ulSignals & ulWindowSignal) {
		while (pMsg = (struct IntuiMessage *) GetMsg(g_sWindowManager.pWindow->UserPort)) {
			ReplyMsg((struct Message *) pMsg);

			switch (pMsg->Class) {
				case IDCMP_RAWKEY:
					if (pMsg->Code & IECODE_UP_PREFIX) {
						pMsg->Code -= IECODE_UP_PREFIX;
						keySetState(pMsg->Code, KEY_NACTIVE);
					}
					else if (!keyCheck(pMsg->Code)) {
						keySetState(pMsg->Code, KEY_ACTIVE);
					}
					break;
			}
		}
	}
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
