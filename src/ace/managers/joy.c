#include <ace/managers/joy.h>
#include <ace/managers/log.h>
#include <ace/utils/custom.h>
#include <resources/misc.h> // OS-friendly parallel joys: misc.resource
#include <clib/misc_protos.h> // OS-friendly parallel joys: misc.resource

/* Globals */
tJoyManager g_sJoyManager;

#if defined(CONFIG_SYSTEM_OS_FRIENDLY)
struct Library *MiscBase = 0;
static UBYTE s_ubOldDataDdr;
static UBYTE s_ubOldStatusDdr;
static UBYTE s_ubOldDataVal;
static UBYTE s_ubOldStatusVal;
#endif

/* Functions */
void joyOpen(void) {
#if defined(CONFIG_SYSTEM_OS_FRIENDLY)
	// Open misc.resource for 3rd and 4th joy connected to parallel port
	static const char *szOwner = "ACE joy manager";
	MiscBase = (struct Library*)OpenResource(MISCNAME);
	if(!MiscBase) {
		logWrite("ERR: Couldn't open %s\n", MISCNAME);
		return;
	}

	// Are data/status line currently used?
	char *szCurrentOwner;
	szCurrentOwner = AllocMiscResource(MR_PARALLELPORT, szOwner);
	if(szCurrentOwner) {
		logWrite("ERR: Parallel data lines access blocked by: %s\n", szCurrentOwner);
		return;
	}
	szCurrentOwner = AllocMiscResource(MR_PARALLELBITS, szOwner);
	if(szCurrentOwner) {
		logWrite("ERR: Parallel status lines access blocked by: %s\n", szCurrentOwner);
		// Free what was already allocated
		FreeMiscResource(MR_PARALLELPORT);
	}

	// Save old DDR & value regs
	s_ubOldDataDdr = g_pCiaA->ddrb;
	s_ubOldStatusDdr = g_pCiaB->ddra;
	s_ubOldDataVal = g_pCiaA->prb;
	s_ubOldStatusVal = g_pCiaB->pra;

	// Set data direction register to input. Status lines are as follows:
	// bit 0: BUSY
	// bit 2: SEL
	g_pCiaB->ddra |= BV(0) | BV(2); // Status lines DDR
	g_pCiaA->ddrb = 0xFF; // Data lines DDR

	g_pCiaB->pra &= 0xFF^(BV(0) | BV(2)); // Status lines values
	g_pCiaA->prb = 0; // Data lines values
#endif
}

void joyClose(void) {
#if defined(CONFIG_SYSTEM_OS_FRIENDLY)
	// Restore old status/data DDR/val regs
	g_pCiaA->prb = s_ubOldDataVal;
	g_pCiaB->pra = s_ubOldStatusVal;
	g_pCiaA->ddrb= s_ubOldDataDdr;
	g_pCiaB->ddra = s_ubOldStatusDdr;

	// Close misc.resource
	FreeMiscResource(MR_PARALLELPORT);
	FreeMiscResource(MR_PARALLELBITS);
#endif
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

void joyProcess(void) {
	UBYTE ubCiaAPra = g_pCiaA->pra;
	UWORD uwJoyDataPort1 = g_pCustom->joy0dat;
	UWORD uwJoyDataPort2 = g_pCustom->joy1dat;
	UBYTE ubParData = g_pCiaA->prb;
	UBYTE ubParStatus = g_pCiaB->pra;

	UWORD pJoyLookup[] = {
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

		!BTST(ubParStatus, 2), // Joy 3 fire
		!BTST(ubParData, 0),   // Joy 3 up
		!BTST(ubParData, 1),   // Joy 3 down
		!BTST(ubParData, 2),   // Joy 3 left
		!BTST(ubParData, 3),   // Joy 3 right

		!BTST(ubParStatus , 0), // Joy 4 fire
		!BTST(ubParData , 4),   // Joy 4 up
		!BTST(ubParData , 5),   // Joy 4 down
		!BTST(ubParData , 6),   // Joy 4 left
		!BTST(ubParData , 7)    // Joy 4 right
	};

	UBYTE ubJoyCode = 20;
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
