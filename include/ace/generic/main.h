#ifndef GUARD_ACE_GENERIC_MAIN_H
#define GUARD_ACE_GENERIC_MAIN_H

#include <ace/types.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/managers/timer.h>
#include <ace/managers/window.h>
#include <ace/managers/blit.h>
#include <ace/managers/copper.h>
#include <ace/managers/game.h>

void genericCreate(void);
void genericProcess(void);
void genericDestroy(void);

int main(void) {
	memCreate();
	logOpen();
	timerCreate();

	windowCreate();
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
	windowDestroy();

	timerDestroy();
	logClose();
	memDestroy();

	return EXIT_SUCCESS;
}

#endif