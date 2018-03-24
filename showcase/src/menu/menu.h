#ifndef GUARD_SHOWCASE_MENU_MENU_H
#define GUARD_SHOWCASE_MENU_MENU_H

#include <ace/types.h>

/* ****************************************************************** DEFINES */

#define MENU_MAIN 0
#define MENU_TESTS 1
#define MENU_EXAMPLES 2

#define MENU_TEST_BLITRECT 10

#define MENU_EXAMPLES_PONG 20

/* ******************************************************************** TYPES */

/* ****************************************************************** GLOBALS */

/* **************************************************************** FUNCTIONS */

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

/* ****************************************************************** INLINES */

/* ******************************************************************* MACROS */

#endif
