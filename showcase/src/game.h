/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _SHOWCASE_GAME_H_
#define _SHOWCASE_GAME_H_

#include <ace/managers/state.h>

//---------------------------------------------------------------------- DEFINES

#define SHOWCASE_BPP 5

typedef enum tTestState {
	TEST_STATE_MENU,
	TEST_STATE_BLIT,
	TEST_STATE_INPUT,
	TEST_STATE_FONT,
	TEST_STATE_COPPER,
	TEST_STATE_LINES,
	TEST_STATE_BLIT_SMALL_DEST,
	TEST_STATE_INTERLEAVED,
	TEST_STATE_BUFFER_SCROLL,
	TEST_STATE_TWISTER,
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
