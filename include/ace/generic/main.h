/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_GENERIC_MAIN_H_
#define _ACE_GENERIC_MAIN_H_

#include <stdlib.h>
#include <ace/types.h>
#include <ace/managers/system.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/managers/timer.h>
#include <ace/managers/blit.h>
#include <ace/managers/copper.h>
#include <ace/managers/game.h>

void genericCreate(void);
void genericProcess(void);
void genericDestroy(void);

int main(void) {
	systemCreate();
	memCreate();
	logOpen();
	timerCreate();

	blitManagerCreate();
	copCreate();

	gameCreate();
	genericCreate();
	while (gameIsRunning()) {
		timerProcess();
		genericProcess();
	}
	genericDestroy();
	gameDestroy();

	copDestroy();
	blitManagerDestroy();

	timerDestroy();
	logClose();
	memDestroy();
	systemDestroy();

	return EXIT_SUCCESS;
}

#endif
