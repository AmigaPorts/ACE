/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <proto/exec.h> // Bartman's compiler needs this
#include <proto/misc.h> // Bartman's compiler needs this
#include <resources/misc.h> // OS-friendly parallel joys: misc.resource
#include <clib/misc_protos.h> // OS-friendly parallel joys: misc.resource
#include <ace/managers/misc_resource.h>
#include <ace/managers/log.h>
#include <ace/managers/system.h>

// Globals
struct Library *MiscBase = 0;

static UBYTE s_ubOldDataDdr, s_ubOldStatusDdr, s_ubOldDataVal, s_ubOldStatusVal;
static UBYTE s_ubMiscResourceUseMask = 0;
static const char *s_szOwner = "ACE";

#if defined(ACE_DEBUG)
static const char *s_pSubresourceNames[MISC_SUBRESOURCE_COUNT] = {
	[MISC_SUBRESOURCE_PARALLEL] = "Parallel",
	[MISC_SUBRESOURCE_SERIAL] = "Serial",
};
#endif

// Functions

// Wrapper function to omit casting
static inline const char *myAllocMiscResource(
	ULONG ulUnitNum, const char *szOwner
) {
	return (const char*)AllocMiscResource(ulUnitNum, (CONST_STRPTR)szOwner);
}


static UBYTE miscResourceTryOpen(void) {
	systemUse();
	logWrite("Opening misc.resource...\n");

	MiscBase = (struct Library*)OpenResource((CONST_STRPTR)MISCNAME);
	if(!MiscBase) {
		logWrite("ERR: Couldn't open misc.resource\n");
		systemUnuse();
		return 0;
	}

	systemUnuse();
	return 1;
}

static void miscResourceClose(void) {
	// misc.resource doesn't need closing
}

UBYTE miscResourceTryUse(tMiscSubresource eSubresource) {
	if(s_ubMiscResourceUseMask & BV(eSubresource)) {
		logWrite("ERR: misc.resource subresource %d already in use\n", eSubresource);
		return 0;
	}

	if(!s_ubMiscResourceUseMask && !miscResourceTryOpen()) {
		return 0;
	}

	// Are data/status line currently used?
	systemUse();
	const char *szCurrentOwner;
	switch(eSubresource) {
		case MISC_SUBRESOURCE_PARALLEL:
			szCurrentOwner = myAllocMiscResource(MR_PARALLELPORT, s_szOwner);
			if(szCurrentOwner) {
				logWrite("ERR: Parallel data lines access blocked by: '%s'\n", szCurrentOwner);
				systemUnuse();
				return 0;
			}
			szCurrentOwner = myAllocMiscResource(MR_PARALLELBITS, s_szOwner);
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
			break;
		case MISC_SUBRESOURCE_SERIAL:
			szCurrentOwner = myAllocMiscResource(MR_SERIALPORT, s_szOwner);
			if(szCurrentOwner) {
				logWrite("ERR: Serial data lines access blocked by: '%s'\n", szCurrentOwner);
				systemUnuse();
				return 0;
			}
			szCurrentOwner = myAllocMiscResource(MR_SERIALBITS, s_szOwner);
			if(szCurrentOwner) {
				logWrite("ERR: Serial status lines access blocked by: '%s'\n", szCurrentOwner);
				FreeMiscResource(MR_SERIALPORT);
				systemUnuse();
				return 0;
			}
			break;
		case MISC_SUBRESOURCE_COUNT:
			break;
	}
	systemUnuse();

	s_ubMiscResourceUseMask |= BV(eSubresource);
#if defined(ACE_DEBUG)
	logWrite("Opened subresource %s\n", s_pSubresourceNames[eSubresource]);
#endif
	return 1;
}

void miscResourceUnuse(tMiscSubresource eSubresource) {
#if defined(ACE_DEBUG)
	logWrite("Closing subresource %s\n", s_pSubresourceNames[eSubresource]);
#endif

	s_ubMiscResourceUseMask &= ~BV(eSubresource);

	systemUse();

	switch(eSubresource) {
		case MISC_SUBRESOURCE_PARALLEL:
			// Restore old status/data DDR/val regs
			g_pCia[CIA_A]->prb = s_ubOldDataVal;
			g_pCia[CIA_B]->pra = s_ubOldStatusVal;
			g_pCia[CIA_A]->ddrb = s_ubOldDataDdr;
			g_pCia[CIA_B]->ddra = s_ubOldStatusDdr;

			FreeMiscResource(MR_PARALLELBITS);
			FreeMiscResource(MR_PARALLELPORT);
			break;
		case MISC_SUBRESOURCE_SERIAL:
			FreeMiscResource(MR_SERIALBITS);
			FreeMiscResource(MR_SERIALPORT);
			break;
		case MISC_SUBRESOURCE_COUNT:
			break;
	}

	if(!s_ubMiscResourceUseMask) {
		miscResourceClose();
	}
	systemUnuse();
}

UBYTE miscResourceIsUsed(tMiscSubresource eSubresource) {
	return (s_ubMiscResourceUseMask & BV(eSubresource)) != 0;
}
