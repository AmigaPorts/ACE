/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/joy.h>
#include <ace/managers/sdl_private.h>

#define JOY_MAX 4
#define INPUTS_PER_JOY 5

typedef struct tJoyState {
	SDL_GameController *pController;
	SDL_JoystickID lInstanceId;
	UBYTE pButtonStates[INPUTS_PER_JOY];
	UBYTE isActive;
} tJoyState;

static tJoyState s_pJoyStates[JOY_MAX];
static UBYTE s_isJoyParallelEnabled;

//------------------------------------------------------------------ PRIVATE FNS

static void onJoyButton(SDL_JoystickID lId, SDL_GameControllerButton eButton, UBYTE isPressed) {
	// TODO: update joy 3&4 only when parallel is enabled?
	for(UBYTE i = 0; i < JOY_MAX; ++i) {
		if(s_pJoyStates[i].isActive && s_pJoyStates[i].lInstanceId == lId) {
			UBYTE ubButtonIndex;
			switch(eButton) {
				case SDL_CONTROLLER_BUTTON_DPAD_UP:
					ubButtonIndex = JOY_UP;
					break;
				case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
					ubButtonIndex = JOY_DOWN;
					break;
				case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
					ubButtonIndex = JOY_LEFT;
					break;
				case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
					ubButtonIndex = JOY_RIGHT;
					break;
				case SDL_CONTROLLER_BUTTON_A:
					ubButtonIndex = JOY_FIRE;
					break;
				default:
					return;
			}
			UBYTE ubButtonState = (isPressed ? JOY_ACTIVE : JOY_NACTIVE);
			joySetState(i * INPUTS_PER_JOY + ubButtonIndex, ubButtonState);
			return;
		}
	}
}

static void onJoyAddRemove(SDL_JoystickID lId, UBYTE isAdded) {
	if(isAdded) {
		for(UBYTE i = 0; i < JOY_MAX; ++i) {
			if(!s_pJoyStates[i].isActive) {
				s_pJoyStates[i].isActive = 1;
				s_pJoyStates[i].pController = SDL_GameControllerOpen(lId);
				SDL_Joystick *pJoy = SDL_GameControllerGetJoystick(s_pJoyStates[i].pController);
				s_pJoyStates[i].lInstanceId = SDL_JoystickInstanceID(pJoy);
				memset(s_pJoyStates[i].pButtonStates, 0, sizeof(s_pJoyStates[i].pButtonStates));
				logWrite("Added joy id %d on index %hu\n", s_pJoyStates[i].lInstanceId, i);
				return;
			}
		}
		logWrite("ERR: No more room for joy %d\n", lId);
	}
	else {
		for(UBYTE i = 0; i < JOY_MAX; ++i) {
			if(s_pJoyStates[i].isActive && s_pJoyStates[i].lInstanceId == lId) {
				s_pJoyStates[i].isActive = 0;
				SDL_GameControllerClose(s_pJoyStates[i].pController);
				s_pJoyStates[i].pController = 0;
				logWrite("Removed joy id %d on index %hu\n", s_pJoyStates[i].lInstanceId, i);
				return;
			}
		}
		logWrite("ERR: joy id %d not found for remove", lId);
	}
}

//------------------------------------------------------------------- PUBLIC FNS

void joyOpen(void) {
	s_isJoyParallelEnabled = 0;

	sdlRegisterJoyAddRemoveHandler(onJoyAddRemove);
	sdlRegisterJoyButtonHandler(onJoyButton);

	for(UBYTE i = 0; i < JOY_MAX; ++i) {
		s_pJoyStates[i].isActive = 0;
		memset(s_pJoyStates[i].pButtonStates, 0, sizeof(s_pJoyStates[i].pButtonStates));
	}
}

void joyClose(void) {
	sdlRegisterJoyAddRemoveHandler(0);
	sdlRegisterJoyButtonHandler(0);
	s_isJoyParallelEnabled = 0;
}

void joyProcess(void) {

}

UBYTE joyEnableParallel(void) {
	s_isJoyParallelEnabled = 1;
	return 1;
}

void joyDisableParallel(void) {
	s_isJoyParallelEnabled = 0;
}

UBYTE joyIsParallelEnabled(void) {
	return s_isJoyParallelEnabled;
}

void joySetState(UBYTE ubJoyCode, UBYTE ubJoyState) {
	UBYTE ubJoyIndex = ubJoyCode / INPUTS_PER_JOY;
	UBYTE ubButtonIndex = ubJoyCode % INPUTS_PER_JOY;
	if(!s_pJoyStates[ubJoyIndex].isActive) {
		return;
	}
	s_pJoyStates[ubJoyIndex].pButtonStates[ubButtonIndex] = ubJoyState;
}

UBYTE joyCheck(UBYTE ubJoyCode) {
	UBYTE ubJoyIndex = ubJoyCode / INPUTS_PER_JOY;
	UBYTE ubButtonIndex = ubJoyCode % INPUTS_PER_JOY;
	if(!s_pJoyStates[ubJoyIndex].isActive) {
		return JOY_NACTIVE;
	}
	return s_pJoyStates[ubJoyIndex].pButtonStates[ubButtonIndex];
}

UBYTE joyUse(UBYTE ubJoyCode) {
	UBYTE ubJoyIndex = ubJoyCode / INPUTS_PER_JOY;
	UBYTE ubButtonIndex = ubJoyCode % INPUTS_PER_JOY;
	if(!s_pJoyStates[ubJoyIndex].isActive) {
		return 0;
	}
	UBYTE isUsed = (s_pJoyStates[ubJoyIndex].pButtonStates[ubButtonIndex] == JOY_ACTIVE);
	if(isUsed) {
		s_pJoyStates[ubJoyIndex].pButtonStates[ubButtonIndex] = JOY_USED;
	}

	return isUsed;
}
