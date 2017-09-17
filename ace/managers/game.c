#include <ace/managers/game.h>

/* Globals */
tGameManager g_sGameManager;

/* Functions */
void gameCreate(tVoidFn pCreateCallback, tVoidFn pLoopCallback, tVoidFn pDestroyCallback) {
	g_sGameManager.ubStateCount = 0;
	g_sGameManager.pStateFirst = 0;
	g_sGameManager.ubIsRunning = 1; // tu albo w pusha bo inaczej gra zaraz wyjdzie
	// chyba lepiej tu, bo nie ma sensu wielokrotnie nadawa� tej jedynki

	gamePushState(pCreateCallback, pLoopCallback, pDestroyCallback);
}

UBYTE gameIsRunning(void) {
	return g_sGameManager.ubIsRunning;
}

void gamePushState(tVoidFn pCreateCallback, tVoidFn pLoopCallback, tVoidFn pDestroyCallback) {
	// extViewFadeOut(g_sWindowManager.pExtView);

	tGameState *pGameState = memAllocFast(sizeof(tGameState));
	pGameState->pCreateCallback = pCreateCallback;
	pGameState->pLoopCallback = pLoopCallback;
	pGameState->pDestroyCallback = pDestroyCallback;
	pGameState->pView = 0;
	pGameState->pPrev = g_sGameManager.pStateFirst;

	g_sGameManager.ubStateCount += 1;
	g_sGameManager.pStateFirst = pGameState;

	if (pGameState->pCreateCallback) {
		pGameState->pCreateCallback();

		// extViewFadeIn(g_sWindowManager.pExtView);
	}
}

void gamePopState(void) {
	if (!g_sGameManager.ubStateCount) {
		gameClose();
	}

	if (g_sGameManager.pStateFirst->pDestroyCallback) {
		// extViewFadeOut(g_sWindowManager.pExtView);

		g_sGameManager.pStateFirst->pDestroyCallback();
	}

	tGameState *pGameState = g_sGameManager.pStateFirst;

	g_sGameManager.ubStateCount -= 1;
	g_sGameManager.pStateFirst = pGameState->pPrev;

	memFree(pGameState, sizeof(tGameState));

	viewLoad(g_sGameManager.pStateFirst->pView);

	// extViewFadeIn(g_sWindowManager.pExtView);
}

void gameChangeState(tVoidFn pCreateCallback, tVoidFn pLoopCallback, tVoidFn pDestroyCallback) {
	logBlockBegin("gameChangeState(pCreateCallback: %p, pLoopCallback: %p, pDestroyCallback: %p)", pCreateCallback, pLoopCallback, pDestroyCallback);
	if (g_sGameManager.pStateFirst->pDestroyCallback) {
		// extViewFadeOut(g_sWindowManager.pExtView);
		g_sGameManager.pStateFirst->pDestroyCallback();
	}
	g_sGameManager.pStateFirst->pCreateCallback = pCreateCallback;
	g_sGameManager.pStateFirst->pLoopCallback = pLoopCallback;
	g_sGameManager.pStateFirst->pDestroyCallback = pDestroyCallback;
	g_sGameManager.ubIsRunning = 1;

	if (g_sGameManager.pStateFirst->pCreateCallback) {
		g_sGameManager.pStateFirst->pCreateCallback();

		// extViewFadeIn(g_sWindowManager.pExtView);
	}
	logBlockEnd("gameChangeState");
}

void gameChangeLoop(tVoidFn pLoopCallback) {
	g_sGameManager.pStateFirst->pLoopCallback = pLoopCallback;
}

void gameProcess(void) {
	if (g_sGameManager.pStateFirst->pLoopCallback)
		g_sGameManager.pStateFirst->pLoopCallback();
}

void gameClose(void) {
	g_sGameManager.ubIsRunning = 0;
}

void gameDestroy(void) {
	tGameState *pGameState, *pPrev;

	pGameState = g_sGameManager.pStateFirst;
	while (pGameState) {
		if (g_sGameManager.pStateFirst->pDestroyCallback) {
			g_sGameManager.pStateFirst->pDestroyCallback();
		}
		pPrev = pGameState->pPrev;
		memFree(pGameState, sizeof(tGameState));
		pGameState = pPrev;
	}
}

void gameKill(char *szError) {
	gameDestroy();
	windowKill(szError);
}
