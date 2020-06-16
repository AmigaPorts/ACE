/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/game.h>
#include <ace/managers/system.h> // systemKill

UBYTE s_isGameRunning = 1;

/* Functions */
void gameExit(void) {
	s_isGameRunning = 0;
}

UBYTE gameIsRunning(void) {
	return s_isGameRunning;
}

tGameManager *gameManagerCreate() {
	logBlockBegin("gameManagerCreate()");

	tGameManager *pGameManager = memAllocFastClear(sizeof(tGameManager));

	logBlockEnd("gameManagerCreate()");

	return pGameManager;
}

void gameManagerDestroy(tGameManager *pGameManager) {
	logBlockBegin("gameManagerDestroy(pGameManager: %p)", pGameManager);

	gameStatePopAll(pGameManager);

	memFree(pGameManager, sizeof(tGameManager));

	logBlockEnd("gameManagerDestroy()");
}

tGameState *gameStateCreate(tGameCb cbCreate, tGameCb cbLoop, tGameCb cbDestroy, tGameCb cbSuspend, tGameCb cbResume) {
	logBlockBegin("gameStateCreate(cbCreate: %p, cbLoop: %p, cbDestroy: %p, cbSuspend: %p, cbResume: %p)", cbCreate, cbLoop, cbDestroy, cbSuspend, cbResume);

	tGameState *pGameState = memAllocFast(sizeof(tGameState));

	pGameState->cbCreate = cbCreate;
	pGameState->cbLoop = cbLoop;
	pGameState->cbDestroy = cbDestroy;
	pGameState->cbSuspend = cbSuspend;
	pGameState->cbResume = cbResume;
	pGameState->pStatePrev = 0;  // Setting single zero value looks cheaper that allocating structure with cleared chunk of memory

	logBlockEnd("gameStateCreate()");

	return pGameState;
}

void gameStateDestroy(tGameState *pGameState) {
	logBlockBegin("gameStateDestroy(pGameState: %p)", pGameState);
	
	memFree(pGameState, sizeof(tGameState));

	logBlockEnd("gameStateDestroy()");
}

void gameStatePush(tGameManager *pGameManager, tGameState *pGameState) {
	logBlockBegin("gameStatePush(pGameManager: %p, pGameState: %p)", pGameManager, pGameState);

	if (pGameManager->pState && pGameManager->pState->cbSuspend) {
		pGameManager->pState->cbSuspend();
	}

	pGameState->pStatePrev = pGameManager->pState;
	pGameManager->pState = pGameState;

	if (pGameManager->pState && pGameManager->pState->cbCreate) {
		pGameManager->pState->cbCreate();
	}

	logBlockEnd("gameStatePush()");
}

void gameStatePop(tGameManager *pGameManager) {
	logBlockBegin("gameStatePop(pGameManager: %p)", pGameManager);

	if (pGameManager->pState && pGameManager->pState->cbDestroy) {
		pGameManager->pState->cbDestroy();
	}

	tGameState *pGameStateToPop = pGameManager->pState;
	pGameManager->pState = pGameStateToPop->pStatePrev;

	if (pGameManager->pState && pGameManager->pState->cbResume) {
		pGameManager->pState->cbResume();
	}

	logBlockEnd("gameStatePop()");
}

void gameStatePopAll(tGameManager *pGameManager) {
	logBlockBegin("gameStatePopAll(pGameManager: %p)", pGameManager);

	while (pGameManager->pState) {
		if (pGameManager->pState->cbDestroy) {
			pGameManager->pState->cbDestroy();
		}

		pGameManager->pState = pGameManager->pState->pStatePrev;
	}

	logBlockEnd("gameStatePopAll()");
}

void gameStateChange(tGameManager *pGameManager, tGameState *pGameState) {
	logBlockBegin("gameStateChange(pGameManager: %p, pGameState: %p)", pGameManager, pGameState);

	if (pGameManager->pState && pGameManager->pState->cbDestroy) {
		pGameManager->pState->cbDestroy();
	}

	pGameManager->pState = pGameState;

	if (pGameManager->pState && pGameManager->pState->cbCreate) {
		pGameManager->pState->cbCreate();
	}

	logBlockEnd("gameStateChange()");
}

void gameStateProcess(tGameManager *pGameManager) {
	if (pGameManager->pState && pGameManager->pState->cbLoop) {
		pGameManager->pState->cbLoop();
	}
}