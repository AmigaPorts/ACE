/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_GENERIC_MAIN_H_
#define _ACE_GENERIC_MAIN_H_

/**
 * @file main.h
 * @brief Implements generic way of starting ACE project, allowing to avoid
 * excess boilerplate.
 *
 * This module is based around Arduino's setup/loop idiom: the idea is to have
 * unified way of setting up stuff that almost every project will use,
 * and moving management of everything else to dedicated callbacks.
 *
 * You can always skip including this file if you prefer to have custom main()
 * and program flow.
 */

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
#include <ace/managers/game.h>

#ifndef GENERIC_MAIN_LOG_PATH
#define GENERIC_MAIN_LOG_PATH 0
#endif

//--------------------------------------------------------- USER FUNCTIONS BEGIN
// Those functions must be defined by the user in some other file (e.g. main.c)!

/**
 * @brief User initialization code.
 *
 * This is called once ACE is mostly set up, before calling genericProcess.
 * Use it to initialize additional ACE modules or set up your game.
 *
 * @see genericProcess()
 * @see genericDestroy()
 */
void genericCreate(void);

/**
 * @brief Main loop code.
 *
 * This is called while GENERIC_MAIN_LOOP_CONDITION is true.
 * Use it to process things every frame, e.g. current game state,
 * or additional ACE modules.
 *
 * @see genericCreate()
 * @see genericDestroy()
 */
void genericProcess(void);

/**
 * @brief User deinitialization code.
 *
 * This is called once GENERIC_MAIN_LOOP_CONDITION returns false.
 * After executing this function, app will deinitialize and return to OS.
 * Use it to clean up after things you've set up in genericCreate.
 *
 * @see genericCreate()
 * @see genericProcess()
 */
void genericDestroy(void);

//----------------------------------------------------------- USER FUNCTIONS END

// You can define this macro before including this file to change app's
// loop condition
#ifndef GENERIC_MAIN_LOOP_CONDITION
#define GENERIC_MAIN_LOOP_CONDITION gameIsRunning()
#endif

//-------------------------------------------------------- STACK SMASH DETECTION
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
	while(1) continue;
}
#endif

//-------------------------------------------------------- GENERIC MAIN FUNCTION

int main(void) {
	systemCreate();
	logOpen(GENERIC_MAIN_LOG_PATH);
	memCreate();
#if !defined(GENERIC_MAIN_NO_TIMER)
	timerCreate();
#endif

	blitManagerCreate();
	copCreate();

	// Call user callbacks:
	genericCreate();
	while (GENERIC_MAIN_LOOP_CONDITION) {
#if !defined(GENERIC_MAIN_NO_TIMER)
		timerProcess();
#endif
		genericProcess();
	}
	genericDestroy();

	copDestroy();
	blitManagerDestroy();

#if !defined(GENERIC_MAIN_NO_TIMER)
	timerDestroy();
#endif
	memDestroy();
	logClose();
	systemDestroy();

	return EXIT_SUCCESS;
}

#ifdef __cplusplus
}
#endif

#endif
