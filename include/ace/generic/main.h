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

#if defined(__GNUC__)
#include <stdint.h>

#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif

uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

__attribute__((noreturn))
void __stack_chk_fail(void) {
	logWrite("ERR: STACK SMASHED\n");
	while(1) {}
}
#endif

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
