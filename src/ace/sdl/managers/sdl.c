/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <SDL.h>

#include <ace/managers/sdl_private.h>
#include <ace/managers/system.h>
#include <ace/managers/blit.h>

SDL_Window* s_pWindow = 0;
SDL_Surface* s_pScreenSurface = 0;
tView *s_pCurrentView;
tBitMap *s_pSurfaceBitmap;

//------------------------------------------------------------------ PRIVATE FNS

static void sdlUpdateSurfaceContents(void) {
	blitRect(s_pSurfaceBitmap, 0, 0, 320, 256, 0);

	if(s_pCurrentView && s_pCurrentView->ubVpCount > 0) {
		tVPort *pVp = s_pCurrentView->pFirstVPort;
		// Update palette from first VPort
		while(pVp) {
			// Copy contents of each vport to SDL surface
			tVpManager *pVpManager = vPortGetManager(pVp, VPM_SCROLL);
			pVpManager->cbDrawToSurface(pVpManager);
			pVp = pVp->pNext;
		}
	}

	// TODO: planar2chunky s_pSurfaceBitmap to surface instead
	Uint32 ulColorWhite = SDL_MapRGB(s_pScreenSurface->format, 0, 0, 0);
	SDL_FillRect(s_pScreenSurface, NULL, ulColorWhite);

	SDL_UpdateWindowSurface(s_pWindow);
}

//------------------------------------------------------------------- PUBLIC FNS

void sdlManagerCreate(void) {
	s_pCurrentView = 0;
	s_pSurfaceBitmap = 0;

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
	if(!s_pWindow) {
		logWrite("SDL Error: %s\n", SDL_GetError());
		systemKill("SDL_CreateWindow error");
		return;
	}

	s_pScreenSurface = SDL_GetWindowSurface(s_pWindow);
	s_pSurfaceBitmap = bitmapCreate(320, 256, 8, BMF_CLEAR);
}

void sdlManagerProcess(void) {
	sdlUpdateSurfaceContents();

	// Hack to get window to stay up
	SDL_Event e;

	while(SDL_PollEvent(&e)) {
		if(e.type == SDL_QUIT) {
			systemKill("SDL_QUIT");
		}
	}

}

void sdlManagerDestroy(void) {
	if(s_pSurfaceBitmap) {
		bitmapDestroy(s_pSurfaceBitmap);
	}
	if(s_pWindow) {
		SDL_DestroyWindow(s_pWindow);
	}
	SDL_Quit();
}

void sdlMessageBox(const char *szTitle, const char *szMsg) {
	SDL_ShowSimpleMessageBox(0, szTitle, szMsg, s_pWindow);
}

void sdlSetCurrentView(tView *pView) {
	s_pCurrentView = pView;
	sdlUpdateSurfaceContents();
}

tView *sdlGetCurrentView(void) {
	return s_pCurrentView;
}

tBitMap *sdlGetSurfaceBitmap(void) {
	return s_pSurfaceBitmap;
}
