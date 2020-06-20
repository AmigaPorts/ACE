/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game.h"
#include <ace/managers/joy.h>
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include "menu/menu.h"
#include "test/blit.h"
#include "test/copper.h"
#include "test/font.h"
#include "test/blitsmalldest.h"
#include "test/interleaved.h"
#include "test/lines.h"
#include "test/buffer_scroll.h"

tStateManager *g_pGameStateManager = 0;
tState *g_pGameStates[GAME_STATE_COUNT] = {0};

#define GENERIC_MAIN_LOOP_CONDITION gameIsRunning() && g_pGameStateManager->pCurrent
#include <ace/generic/main.h>

void genericCreate(void) {
	joyOpen();
	keyCreate();

    createGameStates();
    stateChange(g_pGameStateManager, g_pGameStates[GAME_STATE_MENU]);
}

void genericProcess(void) {
	joyProcess();
	keyProcess();

    stateProcess(g_pGameStateManager);
}

void genericDestroy(void) {
    destroyGameStates();

	keyDestroy();
	joyClose();
}

void createGameStates(void) {
    g_pGameStateManager = stateManagerCreate();
    
    g_pGameStates[GAME_STATE_MENU] = stateCreate(gsMenuCreate, gsMenuLoop, gsMenuDestroy, 0, 0, 0);
    g_pGameStates[GAME_STATE_BLIT] = stateCreate(gsTestBlitCreate, gsTestBlitLoop, gsTestBlitDestroy, 0, 0, 0);
    g_pGameStates[GAME_STATE_FONT] = stateCreate(gsTestFontCreate, gsTestFontTableLoop, gsTestFontDestroy, 0, 0, 0);
    g_pGameStates[GAME_STATE_COPPER] = stateCreate(gsTestCopperCreate, gsTestCopperLoop, gsTestCopperDestroy, 0, 0, 0);
    g_pGameStates[GAME_STATE_LINES] = stateCreate(gsTestLinesCreate, gsTestLinesLoop, gsTestLinesDestroy, 0, 0, 0);
    g_pGameStates[GAME_STATE_BLIT_SMALL_DEST] = stateCreate(gsTestBlitSmallDestCreate, gsTestBlitSmallDestLoop, gsTestBlitSmallDestDestroy, 0, 0, 0);
    g_pGameStates[GAME_STATE_INTERLEAVED] = stateCreate(gsTestInterleavedCreate, gsTestInterleavedLoop, gsTestInterleavedDestroy, 0, 0, 0);
    g_pGameStates[GAME_STATE_BUFFER_SCROLL] = stateCreate(gsTestBufferScrollCreate, gsTestBufferScrollLoop, gsTestBufferScrollDestroy, 0, 0, 0);
}

void destroyGameStates(void) {
    stateManagerDestroy(g_pGameStateManager);

	UBYTE ubStateIndex = GAME_STATE_COUNT;
	while (ubStateIndex--) {
		stateDestroy(g_pGameStates[ubStateIndex]);
	}
}
