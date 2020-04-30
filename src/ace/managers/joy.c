/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/joy.h>
#include <ace/managers/log.h>
#include <ace/managers/system.h>
#include <ace/utils/custom.h>
#include <resources/misc.h> // OS-friendly parallel joys: misc.resource
#include <clib/misc_protos.h> // OS-friendly parallel joys: misc.resource

/* Globals */
tJoyManager g_sJoyManager;

struct Library *MiscBase = 0;
static UBYTE s_isParallelEnabled = 0;
static UBYTE s_ubOldDataDdr, s_ubOldStatusDdr, s_ubOldDataVal, s_ubOldStatusVal;

/* Functions */

// Wrapper function to omit casting
static inline const char *myAllocMiscResource(
	ULONG ulUnitNum, const char *szOwner
) {
	return (const char*)AllocMiscResource(ulUnitNum, (CONST_STRPTR)szOwner);
}


void joyOpen(void) {

}

void joyClose(void) {
	if(s_isParallelEnabled) {
		joyDisableParallel();
	}
}

void joySetState(UBYTE ubJoyCode, UBYTE ubJoyState) {
	g_sJoyManager.pStates[ubJoyCode] = ubJoyState;
}

UBYTE joyCheck(UBYTE ubJoyCode) {
	return g_sJoyManager.pStates[ubJoyCode] != JOY_NACTIVE;
}

UBYTE joyUse(UBYTE ubJoyCode) {
	if (g_sJoyManager.pStates[ubJoyCode] == JOY_ACTIVE) {
		g_sJoyManager.pStates[ubJoyCode] = JOY_USED;
		return 1;
	}
	return 0;
}

void joyProcess(void) {
	UBYTE ubCiaAPra = g_pCia[CIA_A]->pra;
	UWORD uwJoyDataPort1 = g_pCustom->joy0dat;
	UWORD uwJoyDataPort2 = g_pCustom->joy1dat;

	UWORD pJoyLookup[20] = {
		!BTST(ubCiaAPra, 7),                           // Joy 1 fire  (PORT 2)
		BTST(uwJoyDataPort2 >> 1 ^ uwJoyDataPort2, 8), // Joy 1 up    (PORT 2)
		BTST(uwJoyDataPort2 >> 1 ^ uwJoyDataPort2, 0), // Joy 1 down  (PORT 2)
		BTST(uwJoyDataPort2, 9),                       // Joy 1 left  (PORT 2)
		BTST(uwJoyDataPort2, 1),                       // Joy 1 right (PORT 2)

		!BTST(ubCiaAPra, 6),                           // Joy 2 fire  (PORT 1)
		BTST(uwJoyDataPort1 >> 1 ^ uwJoyDataPort1, 8), // Joy 2 up    (PORT 1)
		BTST(uwJoyDataPort1 >> 1 ^ uwJoyDataPort1, 0), // Joy 2 down  (PORT 1)
		BTST(uwJoyDataPort1, 9),                       // Joy 2 left  (PORT 1)
		BTST(uwJoyDataPort1, 1),						           // Joy 2 right (PORT 1)
	};

	UBYTE ubJoyCode;
	if(s_isParallelEnabled) {
		ubJoyCode = 20;
		UBYTE ubParData = g_pCia[CIA_A]->prb;
		UBYTE ubParStatus = g_pCia[CIA_B]->pra;

		pJoyLookup[10] = !BTST(ubParStatus, 2); // Joy 3 fire
		pJoyLookup[11] = !BTST(ubParData, 0);   // Joy 3 up
		pJoyLookup[12] = !BTST(ubParData, 1);   // Joy 3 down
		pJoyLookup[13] = !BTST(ubParData, 2);   // Joy 3 left
		pJoyLookup[14] = !BTST(ubParData, 3);   // Joy 3 right

		pJoyLookup[15] = !BTST(ubParStatus , 0); // Joy 4 fire
		pJoyLookup[16] = !BTST(ubParData , 4);   // Joy 4 up
		pJoyLookup[17] = !BTST(ubParData , 5);   // Joy 4 down
		pJoyLookup[18] = !BTST(ubParData , 6);   // Joy 4 left
		pJoyLookup[19] = !BTST(ubParData , 7);   // Joy 4 right
	}
	else {
		ubJoyCode = 10;
	}
	while (ubJoyCode--) {
		if (pJoyLookup[ubJoyCode]) {
			if (g_sJoyManager.pStates[ubJoyCode] == JOY_NACTIVE) {
				joySetState(ubJoyCode, JOY_ACTIVE);
			}
		}
		else {
			joySetState(ubJoyCode, JOY_NACTIVE);
		}
	}
}

//----------------------------------------------------------- PARALLEL PORT JOYS

UBYTE joyEnableParallel(void) {
	if(s_isParallelEnabled) {
		return 1;
	}
	systemUse();
	// Open misc.resource for 3rd and 4th joy connected to parallel port
	static const char *szOwner = "ACE joy manager";
	MiscBase = (struct Library*)OpenResource((CONST_STRPTR)MISCNAME);
	if(!MiscBase) {
		logWrite("ERR: Couldn't open '%s'\n", MISCNAME);
		systemUnuse();
		return 0;
	}

	// Are data/status line currently used?
	const char *szCurrentOwner;
	szCurrentOwner = myAllocMiscResource(MR_PARALLELPORT, szOwner);
	if(szCurrentOwner) {
		logWrite("ERR: Parallel data lines access blocked by: '%s'\n", szCurrentOwner);
		systemUnuse();
		return 0;
	}
	szCurrentOwner = myAllocMiscResource(MR_PARALLELBITS, szOwner);
	if(szCurrentOwner) {
		logWrite("ERR: Parallel status lines access blocked by: '%s'\n", szCurrentOwner);
		FreeMiscResource(MR_PARALLELPORT);
		systemUnuse();
		return 0;
	}

	// Save old DDR & value regs
	s_ubOldDataDdr = g_pCia[CIA_A]->ddrb;
	s_ubOldStatusDdr = g_pCia[CIA_B]->ddra;
	s_ubOldDataVal = g_pCia[CIA_A]->prb;
	s_ubOldStatusVal = g_pCia[CIA_B]->pra;

	// Set data direction register to input.
	g_pCia[CIA_B]->ddra &= 0xFF ^ (BV(0) | BV(2)); // 0: BUSY, 2: SEL
	g_pCia[CIA_A]->ddrb = 0; // Data

	// Activate pull-ups for BUSY / SEL / data pins
	g_pCia[CIA_B]->pra |= BV(0) | BV(2); // Status lines values
	g_pCia[CIA_A]->prb = 0xFF; // Data lines values

	s_isParallelEnabled = 1;
	systemUnuse();
	return 1;
}

void joyDisableParallel(void) {
	if(!s_isParallelEnabled) {
		return;
	}

	// Restore old status/data DDR/val regs
	g_pCia[CIA_A]->prb = s_ubOldDataVal;
	g_pCia[CIA_B]->pra = s_ubOldStatusVal;
	g_pCia[CIA_A]->ddrb = s_ubOldDataDdr;
	g_pCia[CIA_B]->ddra = s_ubOldStatusDdr;

	// Close misc.resource
	systemUse();
	FreeMiscResource(MR_PARALLELPORT);
	FreeMiscResource(MR_PARALLELBITS);
	systemUnuse();
	s_isParallelEnabled = 0;
}

UBYTE joyIsParallelEnabled(void) {
	return s_isParallelEnabled;
}
