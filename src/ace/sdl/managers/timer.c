/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/timer.h>
#include <ace/managers/sdl_private.h>

tTimerManager g_sTimerManager = {0};

//------------------------------------------------------------------ PRIVATE FNS

void onVblank(void) {
	++g_sTimerManager.uwFrameCounter;
}

//------------------------------------------------------------------- PUBLIC FNS

void timerCreate(void) {
	g_sTimerManager.uwFrameCounter = 0;
	sdlRegisterVblankHandler(onVblank);
	// TODO: implement
}

void timerDestroy(void) {
	sdlRegisterVblankHandler(0);
	// TODO: implement
}

ULONG timerGet(void) {
	// TODO: implement
	return g_sTimerManager.uwFrameCounter;
}

ULONG timerGetPrec(void) {
	// TODO: implement
	return 0;
}

void timerProcess(void) {
	// TODO: implement
}
