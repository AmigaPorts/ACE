/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <SDL.h>

#include <ace/managers/sdl_private.h>
#include <ace/managers/system.h>
#include <ace/managers/blit.h>
#include <ace/utils/chunky.h>

static SDL_Window *s_pWindow = 0;
static SDL_Renderer *s_pWindowRenderer;
static SDL_Surface *s_pOffscreenSurface = 0;
static tView *s_pCurrentView;
static tBitMap *s_pRenderBitmap;
static ULONG s_ulMillisOnVblank;
static SDL_TimerID s_VblankTimerId;

static tSdlKeyHandler s_cbKeyHandler;
static tSdlVblankHandler s_cbVblankHandler;
static tSdlJoyButtonHandler s_cbJoyButtonHandler;
static tSdlJoyAddRemoveHandler s_cbJoyAddRemoveHandler;
static tSdlSpriteHandler s_cbSpriteHandler;
//------------------------------------------------------------------ PRIVATE FNS

static void sdlUpdateSurfaceContents(void) {
	for(UBYTE i = 0; i < s_pRenderBitmap->Depth; ++i) {
		memset(&s_pRenderBitmap->Planes[i][0], 0, s_pRenderBitmap->BytesPerRow * s_pRenderBitmap->Rows);
	}

	if(s_pCurrentView && s_pCurrentView->ubVpCount > 0) {
		tVPort *pVp = s_pCurrentView->pFirstVPort;
		SDL_Color pColors[32];
		for(UBYTE i = 0; i < 32; ++i) {
			pColors[i] = (SDL_Color){
				.r = (pVp->pPalette[i] >> 8) * 17,
				.g = ((pVp->pPalette[i] >> 4) & 0xF) * 17,
				.b = ((pVp->pPalette[i] >> 0) & 0xF) * 17,
				.a = 255
			};
		}

		SDL_SetPaletteColors(s_pOffscreenSurface->format->palette, pColors, 0, 32);
		// Update palette from first VPort
		while(pVp) {
			// Copy contents of each vport to SDL surface
			tVpManager *pVpManager = vPortGetManager(pVp, VPM_SCROLL);
			if(pVpManager) {
				pVpManager->cbDrawToSurface(pVpManager);
			}
			pVp = pVp->pNext;
		}
	}

	if(s_cbSpriteHandler) {
		s_cbSpriteHandler();
	}

	// https://discourse.libsdl.org/t/mini-code-sample-for-sdl2-256-color-palette/27147/9
	UBYTE *pSurfacePixels = s_pOffscreenSurface->pixels;
	ULONG ulPos = 0;
	for(UWORD uwY = 0; uwY < 256; ++uwY) {
		for(UWORD uwX = 0; uwX < 320; uwX += 16) {
			chunkyFromPlanar16(s_pRenderBitmap, uwX, uwY, &pSurfacePixels[ulPos]);
			ulPos += 16;
		}
	}

	// TODO: update pTexture instead
	SDL_Texture* pTexture = SDL_CreateTextureFromSurface(s_pWindowRenderer, s_pOffscreenSurface);
	SDL_RenderCopy(s_pWindowRenderer, pTexture, NULL, NULL);
	SDL_RenderPresent(s_pWindowRenderer);
	SDL_DestroyTexture(pTexture);
}

static ULONG onFauxVblankTimer(ULONG ulInterval, void *pParam) {
	ULONG ulNewInterval = ulInterval;
	s_ulMillisOnVblank = SDL_GetTicks();
	if(s_cbVblankHandler) {
		s_cbVblankHandler();
	}
	return ulNewInterval;
}

//------------------------------------------------------------------- PUBLIC FNS

void sdlRegisterKeyHandler(tSdlKeyHandler cbKeyHandler) {
	s_cbKeyHandler = cbKeyHandler;
}

void sdlRegisterVblankHandler(tSdlVblankHandler cbVblankHandler) {
	s_cbVblankHandler = cbVblankHandler;
}

void sdlRegisterJoyButtonHandler(tSdlJoyButtonHandler cbJoyButtonHandler) {
	s_cbJoyButtonHandler = cbJoyButtonHandler;
}

void sdlRegisterJoyAddRemoveHandler(tSdlJoyAddRemoveHandler cbJoyAddRemoveHandler) {
	s_cbJoyAddRemoveHandler = cbJoyAddRemoveHandler;
}

void sdlRegisterSpriteHandler(tSdlSpriteHandler cbSpriteHandler) {
	s_cbSpriteHandler = cbSpriteHandler;
}

void sdlManagerCreate(void) {
	s_pCurrentView = 0;
	s_pRenderBitmap = 0;

	//Initialize SDL
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0) {
		logWrite("SDL Error: %s\n", SDL_GetError());
		systemKill("SDL_Init error");
		return;
	}

	//Create window
	s_pWindow = SDL_CreateWindow(
		"ACE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		640, 512, SDL_WINDOW_SHOWN
	);
	if(!s_pWindow) {
		logWrite("SDL Error: %s\n", SDL_GetError());
		systemKill("SDL_CreateWindow error");
		return;
	}
	s_pWindowRenderer = SDL_CreateRenderer(s_pWindow, -1, 0);
	s_pOffscreenSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 256, 8, 0, 0, 0, 0);
	s_pRenderBitmap = bitmapCreate(320, 256, 8, BMF_CLEAR);

	SDL_RendererInfo sRendererInfo;
	if (!SDL_GetRendererInfo(s_pWindowRenderer, &sRendererInfo)) {
		logWrite("Using renderer: %s\n", sRendererInfo.name);
	}

	s_VblankTimerId = SDL_AddTimer(20, onFauxVblankTimer, 0);
}

void sdlManagerProcess(void) {
	sdlUpdateSurfaceContents();

	// Hack to get window to stay up
	SDL_Event sEvt;
	while(SDL_PollEvent(&sEvt)) {
		switch(sEvt.type) {
			case SDL_QUIT:
				systemKill("SDL_QUIT");
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if(s_cbKeyHandler) {
					s_cbKeyHandler(sEvt.key.state == SDL_PRESSED, sEvt.key.keysym.sym);
				}
				break;
			// TODO: SDL_CONTROLLERAXISMOTION
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
				if(s_cbJoyButtonHandler) {
					s_cbJoyButtonHandler(sEvt.cbutton.which, sEvt.cbutton.button, sEvt.cbutton.state == SDL_PRESSED);
				}
				break;
			case SDL_CONTROLLERDEVICEADDED:
			case SDL_CONTROLLERDEVICEREMOVED:
				if(s_cbJoyAddRemoveHandler) {
					s_cbJoyAddRemoveHandler(sEvt.cdevice.which, sEvt.cdevice.type != SDL_CONTROLLERDEVICEREMOVED);
				}
				break;
		}
	}

}

void sdlManagerDestroy(void) {
	// TODO: ensure everything is destroyed
	SDL_RemoveTimer(s_VblankTimerId);
	if(s_pRenderBitmap) {
		// TODO: this should be done before mem manager destroys, which is earlier
		// than systemDestroy() calling sdlManagerDestroy().
		bitmapDestroy(s_pRenderBitmap);
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
	return s_pRenderBitmap;
}

ULONG sdlGetMillisSinceVblank(void) {
	ULONG ulElapsedMillis = SDL_GetTicks() - s_ulMillisOnVblank;
	return ulElapsedMillis;
}
