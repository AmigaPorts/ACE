/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_GAME_H_
#define _ACE_MANAGERS_GAME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h> // Amiga typedefs

#include <ace/managers/timer.h>
#include <ace/managers/key.h>

#include <ace/utils/extview.h>

/* Types */

typedef void (*tGameCb)(void);

typedef struct _tGameState {
	tGameCb cbCreate;
	tGameCb cbLoop;
	tGameCb cbDestroy;
	tGameCb cbSuspend;
	tGameCb cbResume;
	struct _tGameState *pStatePrev;
} tGameState;

typedef struct _tGameManager {
	tGameState *pState;
} tGameManager;

/* Functions */

void gameExit();

UBYTE gameIsRunning(void);

tGameManager *gameManagerCreate(void);

void gameManagerDestroy(
	tGameManager *pGameManager
);

tGameState *gameStateCreate(
	tGameCb cbCreate,
	tGameCb cbLoop,
	tGameCb cbDestroy,
	tGameCb cbSuspend,
	tGameCb cbResume
);

void gameStateDestroy(
	tGameState *pGameState
);

void gameStatePush(
	tGameManager *pGameManager,
	tGameState *pGameState
);

void gameStatePop(
	tGameManager *pGameManager
);

void gameStatePopAll(
	tGameManager *pGameManager
);

void gameStateChange(
	tGameManager *pGameManager,
	tGameState *pGameState
);

void gameStateProcess(
	tGameManager *pGameManager
);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_GAME_H_
