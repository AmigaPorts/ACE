/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game.h"
#include <ace/managers/joy.h>
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include "menu/menu.h"
#include "test/blit.h"
#include "test/input.h"
#include "test/copper.h"
#include "test/font.h"
#include "test/blitsmalldest.h"
#include "test/interleaved.h"
#include "test/lines.h"
#include "test/buffer_scroll.h"
#include "test/twister.h"

tStateManager *g_pGameStateManager = 0;
tState g_pTestStates[TEST_STATE_COUNT] = {
    [TEST_STATE_MENU] = {.cbCreate = gsMenuCreate, .cbLoop = gsMenuLoop, .cbDestroy = gsMenuDestroy},
    [TEST_STATE_BLIT] = {.cbCreate = gsTestBlitCreate, .cbLoop = gsTestBlitLoop, .cbDestroy = gsTestBlitDestroy},
    [TEST_STATE_INPUT] = {.cbCreate = gsTestInputCreate, .cbLoop = gsTestInputLoop, .cbDestroy = gsTestInputDestroy},
    [TEST_STATE_FONT] = {.cbCreate = gsTestFontCreate, .cbLoop = gsTestFontTableLoop, .cbDestroy = gsTestFontDestroy},
    [TEST_STATE_COPPER] = {.cbCreate = gsTestCopperCreate, .cbLoop = gsTestCopperLoop, .cbDestroy = gsTestCopperDestroy},
    [TEST_STATE_LINES] = {.cbCreate = gsTestLinesCreate, .cbLoop = gsTestLinesLoop, .cbDestroy = gsTestLinesDestroy},
    [TEST_STATE_BLIT_SMALL_DEST] = {.cbCreate = gsTestBlitSmallDestCreate, .cbLoop = gsTestBlitSmallDestLoop, .cbDestroy = gsTestBlitSmallDestDestroy},
    [TEST_STATE_INTERLEAVED] = {.cbCreate = gsTestInterleavedCreate, .cbLoop = gsTestInterleavedLoop, .cbDestroy = gsTestInterleavedDestroy},
    [TEST_STATE_BUFFER_SCROLL] = {.cbCreate = gsTestBufferScrollCreate, .cbLoop = gsTestBufferScrollLoop, .cbDestroy = gsTestBufferScrollDestroy},
    [TEST_STATE_TWISTER] = {.cbCreate = gsTestTwisterCreate, .cbLoop = gsTestTwisterLoop, .cbDestroy = gsTestTwisterDestroy},
};

#define GENERIC_MAIN_LOOP_CONDITION gameIsRunning() && g_pGameStateManager->pCurrent
#include <ace/generic/main.h>

void genericCreate(void) {
	joyOpen();
	keyCreate();

    createGameStates();
    stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
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
}

void destroyGameStates(void) {
    stateManagerDestroy(g_pGameStateManager);
}
