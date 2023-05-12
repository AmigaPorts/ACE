/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_KEY_H_
#define _ACE_MANAGERS_KEY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>
#ifdef AMIGA
#include <exec/interrupts.h>  // struct Interrupt
#endif // AMIGA

//---------------------------------------------------------------------- DEFINES

/**
 * Key codes
 * see http://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node0479.html
 */

// F-keys
#define KEY_F1 0x50
#define KEY_F2 0x51
#define KEY_F3 0x52
#define KEY_F4 0x53
#define KEY_F5 0x54
#define KEY_F6 0x55
#define KEY_F7 0x56
#define KEY_F8 0x57
#define KEY_F9 0x58
#define KEY_F10 0x59

// Digits
#define KEY_1 0x01
#define KEY_2 0x02
#define KEY_3 0x03
#define KEY_4 0x04
#define KEY_5 0x05
#define KEY_6 0x06
#define KEY_7 0x07
#define KEY_8 0x08
#define KEY_9 0x09
#define KEY_0 0x0A

// Chars - row 1
#define KEY_Q 0x10
#define KEY_W 0x11
#define KEY_E 0x12
#define KEY_R 0x13
#define KEY_T 0x14
#define KEY_Y 0x15
#define KEY_U 0x16
#define KEY_I 0x17
#define KEY_O 0x18
#define KEY_P 0x19

// Chars - row 2
#define KEY_A 0x20
#define KEY_S 0x21
#define KEY_D 0x22
#define KEY_F 0x23
#define KEY_G 0x24
#define KEY_H 0x25
#define KEY_J 0x26
#define KEY_K 0x27
#define KEY_L 0x28

// Chars - row 3
#define KEY_Z 0x31
#define KEY_X 0x32
#define KEY_C 0x33
#define KEY_V 0x34
#define KEY_B 0x35
#define KEY_N 0x36
#define KEY_M 0x37

// Numpad: digits
#define KEY_NUM0 0x0F
#define KEY_NUM1 0x1D
#define KEY_NUM2 0x1E
#define KEY_NUM3 0x1F
#define KEY_NUM4 0x2D
#define KEY_NUM5 0x2E
#define KEY_NUM6 0x2F
#define KEY_NUM7 0x3D
#define KEY_NUM8 0x3E
#define KEY_NUM9 0x3F

// Arrows
#define KEY_UP 0x4C
#define KEY_DOWN 0x4D
#define KEY_RIGHT 0x4E
#define KEY_LEFT 0x4F

// Numpad: ()/*+-. and Enter
#define KEY_NUMLPARENTHESES 0x5A
#define KEY_NUMRPARENTHESES 0x5B
#define KEY_NUMSLASH 0x5C
#define KEY_NUMMULTIPLY 0x5D
#define KEY_NUMPLUS 0x5E
#define KEY_NUMMINUS 0x4A
#define KEY_NUMPERIOD 0x3C
#define KEY_NUMENTER 0x43

// Misc
#define KEY_ESCAPE 0x45
#define KEY_ACCENT 0x00
#define KEY_MINUS 0x0B
#define KEY_EQUALS 0x0C
#define KEY_BACKSLASH 0x0D
#define KEY_TAB 0x42
#define KEY_LBRACKET 0x1A
#define KEY_RBRACKET 0x1B
#define KEY_RETURN 0x44
#define KEY_CONTROL 0x63
#define KEY_CAPSLOCK 0x62
#define KEY_SEMICOLON 0x29
#define KEY_APOSTROPHE 0x2A
#define KEY_REGIONAL1 0x2B
#define KEY_LSHIFT 0x60
#define KEY_REGIONAL2 0x30
#define KEY_COMMA 0x38
#define KEY_PERIOD 0x39
#define KEY_BACKSPACE 0x41
#define KEY_SLASH 0x3A
#define KEY_RSHIFT 0x61
#define KEY_LALT 0x64
#define KEY_LAMIGA 0x66
#define KEY_SPACE 0x40
#define KEY_RAMIGA 0x67
#define KEY_RALT 0x64
#define KEY_DEL 0x46
#define KEY_HELP 0x5F

// Key state flags
#define KEY_NACTIVE 0
#define KEY_USED 1
#define KEY_ACTIVE 2

#define KEY_COUNT 104

//------------------------------------------------------------------------ TYPES

typedef struct _tKeyManager {
	volatile UBYTE pStates[KEY_COUNT];
	volatile UBYTE ubLastKey;
} tKeyManager;

//---------------------------------------------------------------------- GLOBALS

extern tKeyManager g_sKeyManager;
extern const UBYTE g_pFromAscii[];
extern const UBYTE g_pToAscii[];

//-------------------------------------------------------------------- FUNCTIONS

/**
 * Initializes Key manager.
 */
void keyCreate(void);

/**
 * Cleans up after Key manager.
 */
void keyDestroy(void);

/**
 * Processes key manager, updating keys' states.
 * This function should be called once per frame.
 */
void keyProcess(void);

/**
 * Sets state of given key.
 * @param ubKeyCode: Code of key, which state should be changed.
 * @param ubKeyState: Key state (KEY_ACTIVE, KEY_NACTIVE or KEY_USED).
 */
void keySetState(UBYTE ubKeyCode, UBYTE ubKeyState);

/**
 * Polls state of key with given code.
 * @param ubKeyCode: Code of key, which state should be polled.
 * @return 1 if key is pressed, otherwise 0.
 * @see keyUse()
 */
UBYTE keyCheck(UBYTE ubKeyCode);

/**
 * Checks if given key was recently pressed.
 * If key's code is ACTIVE, fn returns 1 and changes key state to USED.
 * @param ubKeyCode: Code of key, which state should be polled.
 * @return 1 if key was recently pressed, otherwise 0.
 * @see keyCheck()
 */
static inline UBYTE keyUse(UBYTE ubKeyCode) {
	if (g_sKeyManager.pStates[ubKeyCode] == KEY_ACTIVE) {
		g_sKeyManager.pStates[ubKeyCode] = KEY_USED;
		return 1;
	}
	return 0;
}

static inline void keyReset(void) {
	for(UBYTE i = 0; i < KEY_COUNT; ++i) {
		g_sKeyManager.pStates[i] = KEY_NACTIVE;
	}
}

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_KEY_H_
