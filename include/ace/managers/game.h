/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_GAME_H_
#define _ACE_MANAGERS_GAME_H_

#include <ace/types.h> // Amiga typedefs

#include <ace/types.h>

#include <ace/managers/timer.h>
#include <ace/managers/key.h>

#include <ace/utils/extview.h>

/* Types */

typedef void (*tGameCb)(void);

typedef struct _tGameState {
	tGameCb cbCreate;
	tGameCb cbLoop;
	tGameCb cbDestroy;
	struct _tGameState *pPrev;
} tGameState;

typedef struct {
	UBYTE ubStateCount;
	UBYTE isRunning;
	tGameState *pStateFirst;
} tGameManager;

/* Globals */

extern tGameManager g_sGameManager;

/* Functions */

void gameCreate(void); /* First gameState needs to be added by gamePushState after calling this function */

void gameDestroy(void);

UBYTE gameIsRunning(void);

void gamePushState(tGameCb cbCreate, tGameCb cbLoop, tGameCb cbDestroy);

void gamePopState(void);

void gameChangeState(tGameCb cbCreate, tGameCb cbLoop, tGameCb cbDestroy);

void gameChangeLoop(tGameCb cbLoop);

void gameProcess(void);

void gameClose(void);

void gameKill(char *szError);

#endif // _ACE_MANAGERS_GAME_H_
