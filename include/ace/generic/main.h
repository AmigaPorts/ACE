/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_GENERIC_MAIN_H_
#define _ACE_GENERIC_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <ace/types.h>
#include <ace/managers/system.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/managers/timer.h>
#include <ace/managers/blit.h>
#include <ace/managers/copper.h>
#include <ace/managers/state.h>

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

tStateManager *g_pGameStateManager;

int main(void) {
	systemCreate();
	memCreate();
	logOpen();
	timerCreate();

	blitManagerCreate();
	copCreate();

	g_pGameStateManager = stateManagerCreate();

	genericCreate();
	while (g_pGameStateManager->pState) {
		timerProcess();
		genericProcess();
	}
	genericDestroy();

	stateManagerDestroy(g_pGameStateManager);

	copDestroy();
	blitManagerDestroy();

	timerDestroy();
	logClose();
	memDestroy();
	systemDestroy();

	return EXIT_SUCCESS;
}

#ifdef __cplusplus
}
#endif

#endif
