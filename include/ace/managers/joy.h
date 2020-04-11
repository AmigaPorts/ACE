/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_JOY_H_
#define _ACE_MANAGERS_JOY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>

/* Types */
#define JPORT1 1
#define JPORT2 2

#define JOY1_FIRE 0
#define JOY1_UP 1
#define JOY1_DOWN 2
#define JOY1_LEFT 3
#define JOY1_RIGHT 4

#define JOY2_FIRE 5
#define JOY2_UP 6
#define JOY2_DOWN 7
#define JOY2_LEFT 8
#define JOY2_RIGHT 9

#define JOY3_FIRE 10
#define JOY3_UP 11
#define JOY3_DOWN 12
#define JOY3_LEFT 13
#define JOY3_RIGHT 14

#define JOY4_FIRE 15
#define JOY4_UP 16
#define JOY4_DOWN 17
#define JOY4_LEFT 18
#define JOY4_RIGHT 19

// Combined access: JOYn + JOY_action
#define JOY_FIRE 0
#define JOY_UP 1
#define JOY_DOWN 2
#define JOY_LEFT 3
#define JOY_RIGHT 4

#define JOY1 0
#define JOY2 5
#define JOY3 10
#define JOY4 15

#define JOY_NACTIVE 0
#define JOY_USED 1
#define JOY_ACTIVE 2

typedef struct _tJoyManager {
	UBYTE pStates[20];
} tJoyManager;

/* Globals */
extern tJoyManager g_sJoyManager;

/* Functions */

/**
 * @brief Initializes joy manager.
 */
void joyOpen(void);

/**
 * @brief Enables additional joystricks through parallel adapter.
 *
 * @return 1 on success, otherwise 0.
 */
UBYTE joyEnableParallel(void);

/**
 * @brief Disables additional joysticks through parallel adapter.
 */
void joyDisableParallel(void);

UBYTE joyIsParallelEnabled(void);

void joySetState(UBYTE ubJoyCode, UBYTE ubJoyState);

UBYTE joyCheck(UBYTE ubJoyCode);

UBYTE joyUse(UBYTE ubJoyCode);

void joyProcess(void);

/**
 * @brief Finishes work of joy manager.
 * This will also call joyDisableParallel() if needed.
 *
 * @see joyDisableParallel()
 */
void joyClose(void);

#ifdef __cplusplus
}
#endif

#endif
