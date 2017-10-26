#include <ace/managers/joy.h>

/* Globals */
tJoyManager g_sJoyManager;

/* Functions */
void joyOpen() {
	getport();
}

void joySetState(UBYTE ubJoyCode, UBYTE ubJoyState) {
	g_sJoyManager.pStates[ubJoyCode] = ubJoyState;
}

UBYTE joyPeek(UBYTE ubJoyCode) {
	return g_sJoyManager.pStates[ubJoyCode] != JOY_NACTIVE;
}

UBYTE joyUse(UBYTE ubJoyCode) {
	if (g_sJoyManager.pStates[ubJoyCode] == JOY_ACTIVE) {
		g_sJoyManager.pStates[ubJoyCode] = JOY_USED;
		return 1;
	}
	return 0;
}

void joyProcess() {
	UBYTE ubParData = rdport();
	UWORD pJoyLookup[] = {
		!(*CIAADDR & (64 * JPORT2)),        // Joy 1 fire  (PORT 2)
		(*JOYADDR2 >> 1 ^ *JOYADDR2) & 256, // Joy 1 up    (PORT 2)
		(*JOYADDR2 >> 1 ^ *JOYADDR2) & 1,   // Joy 1 down  (PORT 2)
		*JOYADDR2 & 512,                    // Joy 1 left  (PORT 2)
		*JOYADDR2 & 2,                      // Joy 1 right (PORT 2)
		
		!(*CIAADDR & (64 * JPORT1)),        // Joy 2 fire  (PORT 1)
		(*JOYADDR1 >> 1 ^ *JOYADDR1) & 256,	// Joy 2 up    (PORT 1)
		(*JOYADDR1 >> 1 ^ *JOYADDR1) & 1,   // Joy 2 down  (PORT 1)
		*JOYADDR1 & 512,                    // Joy 2 left  (PORT 1)
		*JOYADDR1 & 2,						          // Joy 2 right (PORT 1)
		
		!rdsel(),                           // Joy 3 fire
		!(ubParData & 1),                   // Joy 3 up
		!(ubParData & 2),                   // Joy 3 down
		!(ubParData & 4),                   // Joy 3 left
		!(ubParData & 8),                   // Joy 3 right
		
		!rdbusy(),                          // Joy 4 fire
		!(ubParData & 16),                  // Joy 4 up
		!(ubParData & 32),                  // Joy 4 down
		!(ubParData & 64),                  // Joy 4 left
		!(ubParData & 128),                 // Joy 4 right
	};

	UBYTE ubJoyCode = 20;
	while (ubJoyCode--) {
		if (pJoyLookup[ubJoyCode]) {
			if (g_sJoyManager.pStates[ubJoyCode] == JOY_NACTIVE) {
				joySetState(ubJoyCode, JOY_ACTIVE);
			}
		}
		else {
			if (g_sJoyManager.pStates[ubJoyCode] != JOY_NACTIVE) {
				joySetState(ubJoyCode, JOY_NACTIVE);
			}
		}
	}
}

void joyClose() {
	freeport();
}