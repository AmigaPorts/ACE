#include "main.h"

#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/managers/timer.h>
#include <ace/managers/window.h>
#include <ace/managers/blit.h>
#include <ace/managers/copper.h>
#include <ace/managers/game.h>

#include "input.h"
#include "menu/menu.h"

int main(void) {
	memCreate();
	logOpen();
	timerCreate();

	windowCreate();
	blitManagerCreate();
	copCreate();

	inputOpen();

	gameCreate();
	gamePushState(gsMenuCreate, gsMenuLoop, gsMenuDestroy);
	while (gameIsRunning()) {
		timerProcess();
		inputProcess();
		gameProcess();
	}
	gameDestroy();

	inputClose();

	copDestroy();
	blitManagerDestroy();
	windowDestroy();

	timerDestroy();
	logClose();
	memDestroy();
	return EXIT_SUCCESS;
}
