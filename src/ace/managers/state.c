/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/state.h>
#include <ace/managers/log.h>
#include <ace/utils/assume.h>

/* Functions */

tStateManager *stateManagerCreate(void) {
	logBlockBegin("stateManagerCreate()");

	tStateManager *pStateManager = memAllocFastClear(sizeof(tStateManager));

	logBlockEnd("stateManagerCreate()");

	return pStateManager;
}

void stateManagerDestroy(tStateManager *pStateManager) {
	logBlockBegin("stateManagerDestroy(pStateManager: %p)", pStateManager);
	assumeNotNull(pStateManager);

	statePopAll(pStateManager);
	memFree(pStateManager, sizeof(tStateManager));

	logBlockEnd("stateManagerDestroy()");
}

tState *stateCreate(
	tStateCb cbCreate, tStateCb cbLoop, tStateCb cbDestroy,
	tStateCb cbSuspend, tStateCb cbResume
) {
	logBlockBegin(
		"stateCreate(cbCreate: %p, cbLoop: %p, cbDestroy: %p, cbSuspend: %p, cbResume: %p)",
		cbCreate, cbLoop, cbDestroy, cbSuspend, cbResume
	);

	tState *pState = memAllocFast(sizeof(tState));
	assumeNotNull(pState); // TODO: gracefully fail?

	pState->cbCreate = cbCreate;
	pState->cbLoop = cbLoop;
	pState->cbDestroy = cbDestroy;
	pState->cbSuspend = cbSuspend;
	pState->cbResume = cbResume;
	pState->pPrev = 0;

	logBlockEnd("stateCreate()");

	return pState;
}

void stateDestroy(tState *pState) {
	logBlockBegin("stateDestroy(pState: %p)", pState);
	assumeNotNull(pState);

	memFree(pState, sizeof(tState));

	logBlockEnd("stateDestroy()");
}

void statePush(tStateManager *pStateManager, tState *pState) {
	logBlockBegin(
		"statePush(pStateManager: %p, pState: %p)",
		pStateManager, pState
	);
	assumeNotNull(pStateManager);
	assumeNotNull(pState);

	if (pStateManager->pCurrent && pStateManager->pCurrent->cbSuspend) {
		pStateManager->pCurrent->cbSuspend();
	}

	pState->pPrev = pStateManager->pCurrent;
	pStateManager->pCurrent = pState;

	if (pStateManager->pCurrent && pStateManager->pCurrent->cbCreate) {
		pStateManager->pCurrent->cbCreate();
	}

	logBlockEnd("statePush()");
}

void statePop(tStateManager *pStateManager) {
	logBlockBegin("statePop(pStateManager: %p)", pStateManager);
	assumeNotNull(pStateManager);

	if (pStateManager->pCurrent && pStateManager->pCurrent->cbDestroy) {
		pStateManager->pCurrent->cbDestroy();
	}

	tState *pOldState = pStateManager->pCurrent;
	pStateManager->pCurrent = pOldState->pPrev;

	if (pStateManager->pCurrent && pStateManager->pCurrent->cbResume) {
		pStateManager->pCurrent->cbResume();
	}

	logBlockEnd("statePop()");
}

void statePopAll(tStateManager *pStateManager) {
	logBlockBegin("statePopAll(pStateManager: %p)", pStateManager);
	assumeNotNull(pStateManager);

	while (pStateManager->pCurrent) {
		if (pStateManager->pCurrent->cbDestroy) {
			pStateManager->pCurrent->cbDestroy();
		}

		pStateManager->pCurrent = pStateManager->pCurrent->pPrev;
	}

	logBlockEnd("statePopAll()");
}

void stateChange(tStateManager *pStateManager, tState *pState) {
	logBlockBegin(
		"stateChange(pStateManager: %p, pState: %p)",
		pStateManager, pState
	);
	assumeNotNull(pStateManager);
	assumeNotNull(pState);

	if (pStateManager->pCurrent && pStateManager->pCurrent->cbDestroy) {
		pStateManager->pCurrent->cbDestroy();
	}

	if (pStateManager->pCurrent) {
		pState->pPrev = pStateManager->pCurrent->pPrev;
	}
	else {
		pState->pPrev = 0;
	}

	pStateManager->pCurrent = pState;

	if (pStateManager->pCurrent && pStateManager->pCurrent->cbCreate) {
		pStateManager->pCurrent->cbCreate();
	}

	logBlockEnd("stateChange()");
}

void stateProcess(tStateManager *pStateManager) {
	assumeNotNull(pStateManager);

	if (pStateManager->pCurrent && pStateManager->pCurrent->cbLoop) {
		pStateManager->pCurrent->cbLoop();
	}
}
