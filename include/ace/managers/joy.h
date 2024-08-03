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

// Combined access: JOYn + JOY_action
#define JOY_FIRE  0
#define JOY_UP    1
#define JOY_DOWN  2
#define JOY_LEFT  3
#define JOY_RIGHT 4
#define JOY_FIRE2 5

#define JOY1 0
#define JOY2 6
#define JOY3 12
#define JOY4 18

#define JOY1_FIRE	(JOY1 + JOY_FIRE)
#define JOY1_UP		(JOY1 + JOY_UP)
#define JOY1_DOWN	(JOY1 + JOY_DOWN)
#define JOY1_LEFT	(JOY1 + JOY_LEFT)
#define JOY1_RIGHT	(JOY1 + JOY_RIGHT)
#define JOY1_FIRE2	(JOY1 + JOY_FIRE2)

#define JOY2_FIRE	(JOY2 + JOY_FIRE)
#define JOY2_UP		(JOY2 + JOY_UP)
#define JOY2_DOWN	(JOY2 + JOY_DOWN)
#define JOY2_LEFT	(JOY2 + JOY_LEFT)
#define JOY2_RIGHT	(JOY2 + JOY_RIGHT)
#define JOY2_FIRE2	(JOY2 + JOY_FIRE2)

#define JOY3_FIRE	(JOY3 + JOY_FIRE)
#define JOY3_UP		(JOY3 + JOY_UP)
#define JOY3_DOWN	(JOY3 + JOY_DOWN)
#define JOY3_LEFT	(JOY3 + JOY_LEFT)
#define JOY3_RIGHT	(JOY3 + JOY_RIGHT)
#define JOY3_FIRE2	(JOY3 + JOY_FIRE2)

#define JOY4_FIRE	(JOY4 + JOY_FIRE)
#define JOY4_UP		(JOY4 + JOY_UP)
#define JOY4_DOWN	(JOY4 + JOY_DOWN)
#define JOY4_LEFT	(JOY4 + JOY_LEFT)
#define JOY4_RIGHT	(JOY4 + JOY_RIGHT)
#define JOY4_FIRE2	(JOY4 + JOY_FIRE2)


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
