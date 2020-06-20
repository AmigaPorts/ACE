/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _SHOWCASE_MENU_MENU_H_
#define _SHOWCASE_MENU_MENU_H_

#include <ace/types.h>

//---------------------------------------------------------------------- DEFINES

#define MENU_MAIN 0
#define MENU_TESTS 1
#define MENU_EXAMPLES 2

#define MENU_TEST_BLITRECT 10

#define MENU_EXAMPLES_PONG 20

//------------------------------------------------------------------------ TYPES

//---------------------------------------------------------------------- GLOBALS

//-------------------------------------------------------------------- FUNCTIONS

void gsMenuCreate(void);
void gsMenuLoop(void);
void gsMenuDestroy(void);

void menuDrawBg(void);

void menuShowMain(void);
void menuSelectMain(void);

void menuShowTests(void);
void menuSelectTests(void);

void menuShowExamples(void);
void menuSelectExamples(void);

//---------------------------------------------------------------------- INLINES

//----------------------------------------------------------------------- MACROS

#endif // _SHOWCASE_MENU_MENU_H_
