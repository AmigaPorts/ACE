/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/joy.h>

static UBYTE s_isJoyParallelEnabled;

void joyOpen(void) {
	s_isJoyParallelEnabled = 0;
}

UBYTE joyEnableParallel(void) {
	s_isJoyParallelEnabled = 1;
}

void joyDisableParallel(void) {
	s_isJoyParallelEnabled = 0;
}

UBYTE joyIsParallelEnabled(void) {
	return s_isJoyParallelEnabled;
}

void joySetState(UBYTE ubJoyCode, UBYTE ubJoyState) {

}

UBYTE joyCheck(UBYTE ubJoyCode) {
	return 0;
}

UBYTE joyUse(UBYTE ubJoyCode) {
	return 0;
}

void joyProcess(void) {

}

void joyClose(void) {
	s_isJoyParallelEnabled = 0;
}
