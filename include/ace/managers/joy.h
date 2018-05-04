/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GUARD_ACE_MANAGER_JOY_H
#define GUARD_ACE_MANAGER_JOY_H

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

typedef struct {
	UBYTE pStates[20];
} tJoyManager;

/* Globals */
extern tJoyManager g_sJoyManager;

/* Functions */
void joyOpen(void);

void joySetState(UBYTE ubJoyCode, UBYTE ubJoyState);

UBYTE joyPeek(UBYTE ubJoyCode);

UBYTE joyUse(UBYTE ubJoyCode);

void joyProcess(void);

void joyClose(void);

#endif
