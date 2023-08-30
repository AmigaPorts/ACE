/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/system.h>

void systemCreate(void) {

}

void systemDestroy(void) {

}

void systemKill(UNUSED_ARG const char *szMsg) {

}

void systemUse(void) {

}

void systemUnuse(void) {

}

UBYTE systemIsUsed(void) {
	return 1;
}

UBYTE systemIsPal(void) {
	return 1;
}

void systemGetBlitterFromOs(void) {

}

void systemReleaseBlitterToOs(void) {

}

UBYTE systemBlitterIsUsed(void) {
	return 1;
}

void systemDump(void) {

}

void systemIdleBegin(void) {

}

void systemIdleEnd(void) {

}

void systemCheckStack(void) {

}
