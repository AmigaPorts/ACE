/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/timer.h>

ULONG timerGetDelta(ULONG ulStart, ULONG ulStop) {
	if(ulStop >= ulStart) {
		return ulStop-ulStart;
	}
	return (0xFFFFFFFF - ulStart) + ulStop;
}

UBYTE timerPeek(ULONG *pTimer, ULONG ulTimerDelay) {
	return (*pTimer + ulTimerDelay) <= g_sTimerManager.ulGameTicks;
}

UBYTE timerCheck(ULONG *pTimer, ULONG ulTimerDelay) {
	if (timerPeek(pTimer, ulTimerDelay)) {
		*pTimer = g_sTimerManager.ulGameTicks;
		return 1;
	}
	return 0;
}

void timerFormatPrec(char *szBfr, ULONG ulPrecTime) {
	ULONG ulResult, ulRest;
	if(ulPrecTime > 0xFFFFFFFF>>2) {
		sprintf(szBfr, ">7min");
		return;
	}
	// ulResult [us]
	ulResult = ulPrecTime*4;
	ulRest = ulResult % 10;
	ulResult = ulResult / 10;
	if(ulResult < 1000) {
		sprintf(szBfr, "%3lu.%01lu us", ulResult, ulRest);
		return;
	}
	// ulResult [ms]
	ulRest = ulResult % 1000;
	ulResult /= 1000;
	if(ulResult < 1000) {
		sprintf(szBfr, "%3lu.%03lu ms", ulResult, ulRest);
		return;
	}
	// ulResult [s]
	ulRest = ulResult % 1000;
	ulResult /= 1000;
	sprintf(szBfr, "%lu.%03lu s", ulResult, ulRest);
}

void timerWaitUs(UWORD uwUsCnt) {
	// timerGetPrec(): on Amiga, one tick equals: PAL - 0.40us, NTSC - 0.45us
	ULONG ulStart = timerGetPrec();
	UWORD uwTickCnt = uwUsCnt*2/5;
	while(timerGetPrec() - ulStart < uwTickCnt) continue;
}
