/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_SDL_PRIVATE_H_
#define _ACE_MANAGERS_SDL_PRIVATE_H_

#include <ace/utils/extview.h>
#include <ace/utils/bitmap.h>
#include <SDL_keycode.h>
#include <SDL_GameController.h>

typedef void (*tSdlKeyHandler)(UBYTE isPressed, SDL_KeyCode eKeyCode);
typedef void (*tSdlVblankHandler)(void);
typedef void (*tSdlJoyButtonHandler)(SDL_JoystickID lId, SDL_GameControllerButton eButton, UBYTE isPressed);
typedef void (*tSdlJoyAddRemoveHandler)(SDL_JoystickID lId, UBYTE isAdded);
typedef void (*tSdlSpriteHandler)(void);

void sdlManagerCreate(void);

void sdlManagerProcess(void);

void sdlManagerDestroy(void);

void sdlMessageBox(const char *szTitle, const char *szMsg);

void sdlSetCurrentView(tView *pView);

tView *sdlGetCurrentView();

tBitMap *sdlGetSurfaceBitmap(void);

ULONG sdlGetMillisSinceVblank(void);

//-------------------------------------------------------------------- CALLBACKS

void sdlRegisterKeyHandler(tSdlKeyHandler cbKeyHandler);

void sdlRegisterVblankHandler(tSdlVblankHandler cbVblankHandler);

void sdlRegisterJoyButtonHandler(tSdlJoyButtonHandler cbJoyButtonHandler);

void sdlRegisterJoyAddRemoveHandler(tSdlJoyAddRemoveHandler cbJoyAddRemoveHandler);

void sdlRegisterSpriteHandler(tSdlSpriteHandler cbSpriteHandler);

#endif // _ACE_MANAGERS_SDL_PRIVATE_H_
