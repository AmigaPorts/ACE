/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/system.h>
#include <ace/managers/log.h>
#include <SDL.h>

SDL_Window* s_pWindow = 0;
SDL_Surface* s_pScreenSurface = 0;

void systemCreate(void) {
	//Initialize SDL
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		logWrite("SDL Error: %s\n", SDL_GetError());
		systemKill("SDL_Init error");
		return;
	}

	//Create window
	s_pWindow = SDL_CreateWindow(
		"ACE", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		640, 512, SDL_WINDOW_SHOWN
	);
	if(!s_pWindow)
	{
		logWrite("SDL Error: %s\n", SDL_GetError());
		systemKill("SDL_CreateWindow error");
		return;
	}

	//Get window surface
	s_pScreenSurface = SDL_GetWindowSurface(s_pWindow);

	//Fill the surface white
	Uint32 ulColorWhite = SDL_MapRGB(s_pScreenSurface->format, 0xFF, 0xFF, 0xFF);
	SDL_FillRect(s_pScreenSurface, NULL, ulColorWhite);

	//Update the surface
	SDL_UpdateWindowSurface(s_pWindow);
}

void systemProcessFinal(void) {
	//Hack to get window to stay up
	SDL_Event e;

	while(SDL_PollEvent(&e)) {
		if(e.type == SDL_QUIT) {
			systemKill("SDL_QUIT");
		}
	}

}

void systemDestroy(void) {
	SDL_DestroyWindow(s_pWindow);
	SDL_Quit();
}

void systemKill(const char *szMsg) {
	logWrite("SYSKILL: '%s'\n", szMsg);
	SDL_ShowSimpleMessageBox(0, "ACE SYSKILL", szMsg, s_pWindow);
	systemDestroy();
	exit(EXIT_FAILURE);
}

void systemUse(void) {

}

void systemUnuse(void) {

}

UBYTE systemIsUsed(void) {
	return 1;
}

UBYTE systemIsPal(void) {
	return 1;
}

void systemGetBlitterFromOs(void) {

}

void systemReleaseBlitterToOs(void) {

}

UBYTE systemBlitterIsUsed(void) {
	return 1;
}

void systemDump(void) {

}

void systemIdleBegin(void) {

}

void systemIdleEnd(void) {

}

void systemCheckStack(void) {

}
