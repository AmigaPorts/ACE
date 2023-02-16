/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/timer.h>
#include <ace/managers/log.h>
#include <ace/managers/system.h>

/* Globals */
tTimerManager g_sTimerManager = {0};

/* Functions */

void timerCreate(void) {
	g_sTimerManager.uwFrameCounter = 0;
}

void timerDestroy(void) {
	// systemSetInt(INTB_VERTB, 0, 0);
}

ULONG timerGet(void) {
	return g_sTimerManager.uwFrameCounter;
}

void timerOnInterrupt(void) {
	++g_sTimerManager.uwFrameCounter;
}

ULONG timerGetPrec(void) {
#ifdef AMIGA
	// There are 4 cases how measurments may take place:
	// a) uwFr1, pRay, uwFr2 on frame A
	// b) uwFr1, pRay on frame A; uwFr2 on frame B
	// c) uwFr1 on frame A; pRay, uwFr2 on frame B
	// d) uwFr2, pRay, uwFr2 on frame B
	// So if pRay took place at low Y pos, it must be on frame B so use uwFr2,
	// Otherwise, pRay took place on A, so use uwFr1
	UWORD uwFr1 = g_sTimerManager.uwFrameCounter;
	tRayPos sRay = getRayPos();
	UWORD uwFr2 = g_sTimerManager.uwFrameCounter;
	if(sRay.bfPosY < 100) {
		return (uwFr2 * 160 * 313 + sRay.bfPosY * 160 + sRay.bfPosX);
	}
	else {
		return (uwFr1 * 160 * 313 + sRay.bfPosY * 160 + sRay.bfPosX);
	}
#else
	return 0;
#endif
}

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

void timerProcess(void) {
	ULONG ulCurrentTime;

	ulCurrentTime = timerGet();
	if(!g_sTimerManager.ubPaused) {
		if(ulCurrentTime > g_sTimerManager.ulLastTime) {
			g_sTimerManager.ulGameTicks += ulCurrentTime - g_sTimerManager.ulLastTime;
		}
		else {
			g_sTimerManager.ulGameTicks += (0xFFFF - g_sTimerManager.ulLastTime) + ulCurrentTime + 1;
		}
	}
	g_sTimerManager.ulLastTime = ulCurrentTime;
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
	// timerGetPrec(): One tick equals: PAL - 0.40us, NTSC - 0.45us
	ULONG ulStart = timerGetPrec();
	UWORD uwTickCnt = uwUsCnt*2/5;
	while(timerGetPrec() - ulStart < uwTickCnt) continue;
}
