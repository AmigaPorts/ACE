/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "input.h"
#include <ace/managers/key.h>
#include <ace/managers/joy.h>

/* Globals */

/* Functions */
void inputOpen(void) {
	joyOpen();
	keyCreate();
}

void inputProcess(void) {
	joyProcess();
	keyProcess();
}

void inputClose(void) {
	keyDestroy();
	joyClose();
}
