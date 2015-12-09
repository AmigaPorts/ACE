#ifndef GUARD_ACE_MANAGER_GAME_H
#define GUARD_ACE_MANAGER_GAME_H

#include <clib/exec_protos.h> // Amiga typedefs

#include "ACE:config.h"

#include "ACE:managers/timer.h"
#include "ACE:managers/key.h"
#include "ACE:managers/window.h"

#include "ACE:utils/extview.h"

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

inline UBYTE gameIsRunning(void);

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

inline void gameChangeLoop(
	IN tVoidFn pLoopCallback
);

void gameProcess(void);

inline void gameClose(void);

void gameKill(
	IN char *szError
);

#endif