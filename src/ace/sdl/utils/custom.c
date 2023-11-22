/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/custom.h>
#include <ace/managers/sdl_private.h>

tRayPos getRayPos(void) {
	ULONG ulMillis = sdlGetMillisSinceVblank();

	tRayPos sPos = {
		.bfLaced = 0,
		.bfPosX = 0,
		.bfPosY = (ulMillis * (systemIsPal() ? 312 : 272)) / 20,
	};

	return sPos;
}
