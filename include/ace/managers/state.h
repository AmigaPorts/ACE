/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_STATE_H_
#define _ACE_MANAGERS_STATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h> // Amiga typedefs

/* Types */

typedef void (*tStateCb)(void);

/**
 * State struct.
 */
typedef struct _tState {
	tStateCb cbCreate;     ///< Optional callback that fires when state manager
	                       ///< enters to this state.

	tStateCb cbLoop;       ///< Optional callback that fires at manager process.

	tStateCb cbDestroy;    ///< Optional callback that fires when state manager
	                       ///< exists from this state.

	tStateCb cbSuspend;    ///< Optional callback that fires when state manager
	                       ///< pushes new state over this state.

	tStateCb cbResume;     ///< Optional callback that fires when state manager
	                       ///< pops old state over this state.

	struct _tState *pPrev; ///< Optional pointer to previous state.
	                       ///< Zero if there is no previous state. Will be
	                       ///< overriden when pushed into state manager.
} tState;

/**
 * State manager struct.
 */
typedef struct _tStateManager {
	tState *pCurrent; ///< Pointer to currently handled state.
} tStateManager;

/* Functions */

/**
 * Initializes state manager.
 * @see stateManagerDestroy()
 */
tStateManager *stateManagerCreate(void);

/**
 * Cleans up after state manager.
 * @param pStateManager: Pointer to state manager previously created
 *        with stateManagerCreate.
 * @see stateManagerCreate()
 */
void stateManagerDestroy(tStateManager *pStateManager);

/**
 * Initializes state callbacks collection and chaining.
 * @param cbCreate: Callback that fires when state manager enters to this state.
 * @param cbLoop: Callback that fires at manager process.
 * @param cbDestroy: Callback that fires when state manager exists from this state.
 * @param cbSuspend: Callback that fires when state manager pushes new state over this state.
 * @param cbResume: Callback that fires when state manager pops old state over this state.
 * @see stateDestroy()
 */
tState *stateCreate(
	tStateCb cbCreate, tStateCb cbLoop, tStateCb cbDestroy,
	tStateCb cbSuspend, tStateCb cbResume
);

/**
 * Cleans up after state.
 * @param pState: Pointer to state previously created with stateCreate.
 * @see stateCreate()
 */
void stateDestroy(tState *pState);

/**
 * Pushes given state over current state in given state manager. Calls cbSuspend
 * on old state and cbCreate on new state. Will update pPrev in given state to
 * point the old one.
 * @param pStateManager: Pointer to desired state manager where push will happen.
 * @param pState: Pointer to desired state which will be pushed.
 * @see statePop()
 * @see statePopAll()
 * @see stateChange()
 */
void statePush(tStateManager *pStateManager, tState *pState);

/**
 * Pops single state from given state manager. Calls cbDestroy on current state
 * and cbResume on previous state.
 * @param pStateManager: Pointer to desired state manager where pop will happen.
 * @see statePop()
 * @see statePopAll()
 * @see stateChange()
 */
void statePop(tStateManager *pStateManager);

/**
 * Pops all states from given state manager. Calls cbDestroy on all states
 * when sets pCurrent to zero.
 * @param pStateManager: Pointer to desired state manager where pop will happen.
 * @see statePop()
 * @see statePopAll()
 * @see stateChange()
 */
void statePopAll(tStateManager *pStateManager);

/**
 * Swaps state from given state manager with given state. Calls cbDestroy on
 * previous state and cbCreate on new state.
 * @param pStateManager: Pointer to desired state manager where pop will happen.
 * @param pState: Pointer to desired state which will be swapped.
 * @see statePop()
 * @see statePopAll()
 * @see stateChange()
 */
void stateChange(tStateManager *pStateManager, tState *pState);

/**
 * Calls current state's cbLoop on given state manager,
 * @param pStateManager: Pointer to desired state manager too call cbLoop.
 */
void stateProcess(tStateManager *pStateManager);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_STATE_H_
