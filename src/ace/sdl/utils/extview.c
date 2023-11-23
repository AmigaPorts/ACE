/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/extview.h>
#include <ace/managers/sdl_private.h>
#include <ace/managers/system.h>
#include <SDL.h>

UBYTE viewIsLoaded(const tView *pView) {
	UBYTE isLoaded = (sdlGetCurrentView() == pView);
	return isLoaded;
}

void viewUpdateCLUT(tView *pView) {
	sdlSetCurrentView(pView);
}

void viewLoad(tView *pView) {
	sdlSetCurrentView(pView);
}

void vPortWaitForPos(const tVPort *pVPort, UWORD uwPosY, UBYTE isExact) {
	// Determine VPort end position
	UWORD uwEndPos = pVPort->uwOffsY + uwPosY;
	uwEndPos += pVPort->pView->ubPosY; // Addition from DiWStrt
	UWORD uwLinesPerFrame = (systemIsPal() ? 312 : 272);
#if defined(ACE_DEBUG)
	UWORD yPos = uwLinesPerFrame;
	if(uwEndPos >= yPos) {
		logWrite("ERR: vPortWaitForPos - too big wait pos: %04hx (%hu)\n", uwEndPos, uwEndPos);
		logWrite("\tVPort offs: %hu, pos: %hu\n", pVPort->uwOffsY, uwPosY);
	}
#endif

	ULONG ulNowMs = sdlGetMillisSinceVblank();
	UWORD uwCurrentPos = (ulNowMs * uwLinesPerFrame) / 20;

	WORD wDeltaPos = uwEndPos - uwCurrentPos;
	if(wDeltaPos < 0) {
		if(!isExact) {
			return;
		}

		wDeltaPos += uwLinesPerFrame;
	}

	ULONG ulWaitMs = (wDeltaPos * 20) / uwLinesPerFrame;
	SDL_Delay(ulWaitMs);
}
