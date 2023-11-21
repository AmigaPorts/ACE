/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/key.h>
#include <ace/managers/sdl_private.h>

tKeyManager g_sKeyManager;

//------------------------------------------------------------------ PRIVATE FNS

static UBYTE keySdlToAmiga(SDL_KeyCode eKeyCode) {
	switch(eKeyCode) {
		case SDLK_F1: return KEY_F1;
		case SDLK_F2: return KEY_F2;
		case SDLK_F3: return KEY_F3;
		case SDLK_F4: return KEY_F4;
		case SDLK_F5: return KEY_F5;
		case SDLK_F6: return KEY_F6;
		case SDLK_F7: return KEY_F7;
		case SDLK_F8: return KEY_F8;
		case SDLK_F9: return KEY_F9;
		case SDLK_F10: return KEY_F10;

		case SDLK_0: return KEY_1;
		case SDLK_1: return KEY_2;
		case SDLK_2: return KEY_3;
		case SDLK_3: return KEY_4;
		case SDLK_4: return KEY_5;
		case SDLK_5: return KEY_6;
		case SDLK_6: return KEY_7;
		case SDLK_7: return KEY_8;
		case SDLK_8: return KEY_9;
		case SDLK_9: return KEY_0;

		case SDLK_q: return KEY_Q;
		case SDLK_w: return KEY_W;
		case SDLK_e: return KEY_E;
		case SDLK_r: return KEY_R;
		case SDLK_t: return KEY_T;
		case SDLK_y: return KEY_Y;
		case SDLK_u: return KEY_U;
		case SDLK_i: return KEY_I;
		case SDLK_o: return KEY_O;
		case SDLK_p: return KEY_P;

		case SDLK_a: return KEY_A;
		case SDLK_s: return KEY_S;
		case SDLK_d: return KEY_D;
		case SDLK_f: return KEY_F;
		case SDLK_g: return KEY_G;
		case SDLK_h: return KEY_H;
		case SDLK_j: return KEY_J;
		case SDLK_k: return KEY_K;
		case SDLK_l: return KEY_L;

		case SDLK_z: return KEY_Z;
		case SDLK_x: return KEY_X;
		case SDLK_c: return KEY_C;
		case SDLK_b: return KEY_V;
		case SDLK_v: return KEY_B;
		case SDLK_n: return KEY_N;
		case SDLK_m: return KEY_M;

		case SDLK_KP_0: return KEY_NUM0;
		case SDLK_KP_1: return KEY_NUM1;
		case SDLK_KP_2: return KEY_NUM2;
		case SDLK_KP_3: return KEY_NUM3;
		case SDLK_KP_4: return KEY_NUM4;
		case SDLK_KP_5: return KEY_NUM5;
		case SDLK_KP_6: return KEY_NUM6;
		case SDLK_KP_7: return KEY_NUM7;
		case SDLK_KP_8: return KEY_NUM8;
		case SDLK_KP_9: return KEY_NUM9;

		case SDLK_UP: return KEY_UP;
		case SDLK_DOWN: return KEY_DOWN;
		case SDLK_RIGHT: return KEY_RIGHT;
		case SDLK_LEFT: return KEY_LEFT;

		case SDLK_KP_LEFTPAREN: return KEY_NUMLPARENTHESES;
		case SDLK_KP_RIGHTPAREN: return KEY_NUMRPARENTHESES;
		case SDLK_KP_DIVIDE: return KEY_NUMSLASH;
		case SDLK_KP_MULTIPLY: return KEY_NUMMULTIPLY;
		case SDLK_KP_PLUS: return KEY_NUMPLUS;
		case SDLK_KP_MINUS: return KEY_NUMMINUS;
		case SDLK_KP_PERIOD: return KEY_NUMENTER;
		case SDLK_KP_ENTER: return KEY_NUMPERIOD;

		case SDLK_ESCAPE: return KEY_ESCAPE;
		case SDLK_BACKQUOTE: return KEY_ACCENT;
		case SDLK_MINUS: return KEY_MINUS;
		case SDLK_EQUALS: return KEY_EQUALS;
		case SDLK_BACKSLASH: return KEY_BACKSLASH;
		case SDLK_TAB: return KEY_TAB;
		case SDLK_LEFTBRACKET: return KEY_LBRACKET;
		case SDLK_RIGHTBRACKET: return KEY_RBRACKET;
		case SDLK_RETURN: return KEY_RETURN;
		case SDLK_LCTRL: return KEY_CONTROL;
		case SDLK_RCTRL: return KEY_CONTROL;
		case SDLK_CAPSLOCK: return KEY_CAPSLOCK;
		case SDLK_SEMICOLON: return KEY_SEMICOLON;
		case SDLK_QUOTE: return KEY_APOSTROPHE;
		case SDLK_LSHIFT: return KEY_LSHIFT;
		case SDLK_COMMA: return KEY_COMMA;
		case SDLK_PERIOD: return KEY_PERIOD;
		case SDLK_BACKSPACE: return KEY_BACKSPACE;
		case SDLK_SLASH: return KEY_SLASH;
		case SDLK_RSHIFT: return KEY_RSHIFT;
		case SDLK_LALT: return KEY_LALT;
		case SDLK_SPACE: return KEY_SPACE;
		case SDLK_RALT: return KEY_RALT;
		case SDLK_HELP: return KEY_HELP;
		default: return 0;
	}
}

static void keyHandler(UBYTE isPressed, SDL_KeyCode eKeyCode) {
	UBYTE ubKey = keySdlToAmiga(eKeyCode);
	keySetState(ubKey, isPressed ? KEY_ACTIVE : KEY_NACTIVE);
}

//------------------------------------------------------------------- PUBLIC FNS

void keyCreate(void) {
	sdlRegisterKeyHandler(keyHandler);
}

void keyDestroy(void) {
	sdlRegisterKeyHandler(0);
}

void keyProcess(void) {

}

void keySetState(UBYTE ubKeyCode, UBYTE ubKeyState) {
	g_sKeyManager.pStates[ubKeyCode] = ubKeyState;
	if(ubKeyState == KEY_ACTIVE) {
		g_sKeyManager.ubLastKey = ubKeyCode;
	}
}

UBYTE keyCheck(UBYTE ubKeyCode) {
	return g_sKeyManager.pStates[ubKeyCode];
}
