#ifndef GUARD_ACE_GENERIC_MAIN_H
#define GUARD_ACE_GENERIC_MAIN_H

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
