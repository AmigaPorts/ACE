#ifndef GUARD_ACE_MANAGER_JOY_H
#define GUARD_ACE_MANAGER_JOY_H

#include <clib/exec_protos.h> // Amiga typedefs

#include "config.h"

/* Types */
#define JPORT1 1
#define JPORT2 2
#define JOYADDR1 ((WORD *) (0xdff008 + 2 * JPORT1))
#define JOYADDR2 ((WORD *) (0xdff008 + 2 * JPORT2))
#define CIAADDR ((UBYTE *) 0xbfe001)

// enum sux cuz is using 16/32bit int
// typedef enum {
	// JOY1_FIRE,
	// JOY1_UP,
	// JOY1_DOWN,
	// JOY1_LEFT,
	// JOY1_RIGHT,

	// JOY2_FIRE,
	// JOY2_UP,
	// JOY2_DOWN,
	// JOY2_LEFT,
	// JOY2_RIGHT,

	// JOY3_FIRE,
	// JOY3_UP,
	// JOY3_DOWN,
	// JOY3_LEFT,
	// JOY3_RIGHT,

	// JOY4_FIRE,
	// JOY4_UP,
	// JOY4_DOWN,
	// JOY4_LEFT,
	// JOY4_RIGHT
// } tJoyCode;

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

// enum sux cuz is using 16/32bit int
// typedef enum {
	// JOY_NACTIVE,
	// JOY_USED,
	// JOY_ACTIVE,
// } tJoyState;

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
void joySetState(
	IN UBYTE ubJoyCode,
	IN UBYTE ubJoyState
);

UBYTE joyPeek(
	IN UBYTE ubJoyCode
);

UBYTE joyUse(
	IN UBYTE ubJoyCode
);

void joyProcess(void);

void joyClose(void);

extern LONG getport(void); // Get parallel port access

extern void freeport(void); // Free parallel port access

extern UBYTE rdport(void); // Read port data

extern UBYTE rdbusy(void); // Read BUSY state (0/1)

extern UBYTE rdsel(void); // Read SEL state (0/1)

#endif