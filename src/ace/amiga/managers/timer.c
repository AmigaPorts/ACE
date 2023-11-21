/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/timer.h>

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
