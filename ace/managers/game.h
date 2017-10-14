#ifndef GUARD_ACE_MANAGER_GAME_H
#define GUARD_ACE_MANAGER_GAME_H

#include <ace/types.h> // Amiga typedefs

#include <ace/config.h>

#include <ace/managers/timer.h>
#include <ace/managers/key.h>
#include <ace/managers/window.h>

#include <ace/utils/extview.h>

/* Types */

typedef void (*tVoidFn)(void);

typedef struct _tGameState {
	tVoidFn pCreateCallback;
	tVoidFn pLoopCallback;
	tVoidFn pDestroyCallback;
	tView *pView;
	struct _tGameState *pPrev;
} tGameState;

typedef struct {
	UBYTE ubStateCount;
	tGameState *pStateFirst;
	UBYTE ubIsRunning;
} tGameManager;

/* Globals */

extern tGameManager g_sGameManager;

/* Functions */

void gameCreate(
	IN tVoidFn pCreateCallback,
	IN tVoidFn pLoopCallback,
	IN tVoidFn pDestroyCallback
);

void gameDestroy(void);

UBYTE gameIsRunning(void);

void gamePushState(
	IN tVoidFn pCreateCallback,
	IN tVoidFn pLoopCallback,
	IN tVoidFn pDestroyCallback
);

void gamePopState(void);

void gameChangeState(
	IN tVoidFn pCreateCallback,
	IN tVoidFn pLoopCallback,
	IN tVoidFn pDestroyCallback
);

void gameChangeLoop(
	IN tVoidFn pLoopCallback
);

void gameProcess(void);

void gameClose(void);

void gameKill(
	IN char *szError
);

#endif
