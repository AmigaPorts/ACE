/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _SHOWCASE_GAME_H_
#define _SHOWCASE_GAME_H_

#include <ace/managers/state.h>

//---------------------------------------------------------------------- DEFINES

#define SHOWCASE_BPP 5

typedef enum tTestState {
	TEST_STATE_MENU = 0,
	TEST_STATE_BLIT = 1,
	TEST_STATE_FONT = 2,
	TEST_STATE_COPPER = 3,
	TEST_STATE_LINES = 4,
	TEST_STATE_BLIT_SMALL_DEST = 5,
	TEST_STATE_INTERLEAVED = 6,
	TEST_STATE_BUFFER_SCROLL = 7,
	TEST_STATE_TWISTER = 8,
	TEST_STATE_COUNT
} tTestState;

//------------------------------------------------------------------------ TYPES

//---------------------------------------------------------------------- GLOBALS

extern tStateManager *g_pGameStateManager;
extern tState g_pTestStates[TEST_STATE_COUNT];

//-------------------------------------------------------------------- FUNCTIONS

void genericCreate(void);
void genericProcess(void);
void genericDestroy(void);

void createGameStates(void);
void destroyGameStates(void);

//---------------------------------------------------------------------- INLINES

//----------------------------------------------------------------------- MACROS

#endif // _SHOWCASE_GAME_H_
