/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/state.h>

#include <ace/managers/log.h>

/* Functions */
tStateManager *stateManagerCreate() {
	logBlockBegin("stateManagerCreate()");

	tStateManager *pStateManager = memAllocFastClear(sizeof(tStateManager));

	logBlockEnd("stateManagerCreate()");

	return pStateManager;
}

void stateManagerDestroy(tStateManager *pStateManager) {
	logBlockBegin("stateManagerDestroy(pStateManager: %p)", pStateManager);

	statePopAll(pStateManager);

	memFree(pStateManager, sizeof(tStateManager));

	logBlockEnd("stateManagerDestroy()");
}

tState *stateCreate(
	tStateCb cbCreate, tStateCb cbLoop, tStateCb cbDestroy,
	tStateCb cbSuspend, tStateCb cbResume,
	tState *pPrev
) {
	logBlockBegin(
		"stateCreate(cbCreate: %p, cbLoop: %p, cbDestroy: %p, cbSuspend: %p, cbResume: %p, pPrev: %p)",
		cbCreate, cbLoop, cbDestroy, cbSuspend, cbResume, pPrev
	);

	tState *pState = memAllocFast(sizeof(tState));

	pState->cbCreate = cbCreate;
	pState->cbLoop = cbLoop;
	pState->cbDestroy = cbDestroy;
	pState->cbSuspend = cbSuspend;
	pState->cbResume = cbResume;
	pState->pPrev = pPrev;

	logBlockEnd("stateCreate()");

	return pState;
}

void stateDestroy(tState *pState) {
	logBlockBegin("stateDestroy(pState: %p)", pState);
	
	memFree(pState, sizeof(tState));

	logBlockEnd("stateDestroy()");
}

void statePush(tStateManager *pStateManager, tState *pState) {
	logBlockBegin(
		"statePush(pStateManager: %p, pState: %p)",
		pStateManager, pState
	);

	if (pStateManager->pState && pStateManager->pState->cbSuspend) {
		pStateManager->pState->cbSuspend();
	}

	pState->pPrev = pStateManager->pState;
	pStateManager->pState = pState;

	if (pStateManager->pState && pStateManager->pState->cbCreate) {
		pStateManager->pState->cbCreate();
	}

	logBlockEnd("statePush()");
}

void statePop(tStateManager *pStateManager) {
	logBlockBegin("statePop(pStateManager: %p)", pStateManager);

	if (pStateManager->pState && pStateManager->pState->cbDestroy) {
		pStateManager->pState->cbDestroy();
	}

	tState *pOldState = pStateManager->pState;
	pStateManager->pState = pOldState->pPrev;

	if (pStateManager->pState && pStateManager->pState->cbResume) {
		pStateManager->pState->cbResume();
	}

	logBlockEnd("statePop()");
}

void statePopAll(tStateManager *pStateManager) {
	logBlockBegin("statePopAll(pStateManager: %p)", pStateManager);

	while (pStateManager->pState) {
		if (pStateManager->pState->cbDestroy) {
			pStateManager->pState->cbDestroy();
		}

		pStateManager->pState = pStateManager->pState->pPrev;
	}

	logBlockEnd("statePopAll()");
}

void stateChange(tStateManager *pStateManager, tState *pState) {
	logBlockBegin(
		"stateChange(pStateManager: %p, pState: %p)",
		pStateManager, pState
	);

	if (pStateManager->pState && pStateManager->pState->cbDestroy) {
		pStateManager->pState->cbDestroy();
	}

	pStateManager->pState = pState;

	if (pStateManager->pState && pStateManager->pState->cbCreate) {
		pStateManager->pState->cbCreate();
	}

	logBlockEnd("stateChange()");
}

void stateProcess(tStateManager *pStateManager) {
	if (pStateManager->pState && pStateManager->pState->cbLoop) {
		pStateManager->pState->cbLoop();
	}
}