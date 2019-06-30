/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/game.h>
#include <ace/managers/system.h> // systemKill

/* Globals */
tGameManager g_sGameManager;

/* Functions */
void gameCreate(void) {
	logBlockBegin("gameCreate()");
	g_sGameManager.ubStateCount = 0;
	g_sGameManager.pStateFirst = 0;
	// Here or in push or else game will quit instantly.
	// I guess it should stay here, makes each push a bit faster
	g_sGameManager.isRunning = 1;
	logBlockEnd("gameCreate()");
}

UBYTE gameIsRunning(void) {
	return g_sGameManager.isRunning;
}

void gamePushState(tGameCb cbCreate, tGameCb cbLoop, tGameCb cbDestroy) {
	logBlockBegin(
		"gamePushState(cbCreate: %p, cbLoop: %p, cbDestroy: %p)",
		cbCreate, cbLoop, cbDestroy
	);
	tGameState *pGameState = memAllocFast(sizeof(tGameState));
	pGameState->cbCreate = cbCreate;
	pGameState->cbLoop = cbLoop;
	pGameState->cbDestroy = cbDestroy;
	pGameState->pPrev = g_sGameManager.pStateFirst;

	g_sGameManager.ubStateCount += 1;
	g_sGameManager.pStateFirst = pGameState;
	logBlockEnd("gamePushState()\n");

	if (pGameState->cbCreate) {
		pGameState->cbCreate();
	}
}

void gamePopState(void) {
	if (!g_sGameManager.ubStateCount) {
		gameClose();
	}

	tGameState *pGameState = g_sGameManager.pStateFirst;
	if (pGameState->cbDestroy) {
		pGameState->cbDestroy();
	}

	g_sGameManager.ubStateCount -= 1;
	g_sGameManager.pStateFirst = pGameState->pPrev;

	memFree(pGameState, sizeof(tGameState));
}

void gameChangeState(tGameCb cbCreate, tGameCb cbLoop, tGameCb cbDestroy) {
	logBlockBegin(
		"gameChangeState(cbCreate: %p, cbLoop: %p, cbDestroy: %p)",
		cbCreate, cbLoop, cbDestroy
	);
	if (g_sGameManager.pStateFirst->cbDestroy) {
		g_sGameManager.pStateFirst->cbDestroy();
	}
	g_sGameManager.pStateFirst->cbCreate = cbCreate;
	g_sGameManager.pStateFirst->cbLoop = cbLoop;
	g_sGameManager.pStateFirst->cbDestroy = cbDestroy;
	g_sGameManager.isRunning = 1;

	if (g_sGameManager.pStateFirst->cbCreate) {
		g_sGameManager.pStateFirst->cbCreate();
	}
	logBlockEnd("gameChangeState");
}

void gameChangeLoop(tGameCb cbLoop) {
	g_sGameManager.pStateFirst->cbLoop = cbLoop;
}

void gameProcess(void) {
	if (g_sGameManager.pStateFirst->cbLoop) {
		g_sGameManager.pStateFirst->cbLoop();
	}
}

void gameClose(void) {
	g_sGameManager.isRunning = 0;
}

void gameDestroy(void) {
	tGameState *pGameState, *pPrev;

	pGameState = g_sGameManager.pStateFirst;
	while (pGameState) {
		if (pGameState->cbDestroy) {
			pGameState->cbDestroy();
		}
		pPrev = pGameState->pPrev;
		memFree(pGameState, sizeof(tGameState));
		pGameState = pPrev;
	}
}

void gameKill(char *szError) {
	gameDestroy();
	systemKill(szError);
}
