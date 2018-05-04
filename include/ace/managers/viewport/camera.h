/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_VIEWPORT_CAMERA_H_
#define _ACE_MANAGERS_VIEWPORT_CAMERA_H_

/**
 * 2D Camera manager
 * Keeps track of previous and current XY
 * Datasource only - scrolling etc. should be done as separate managers
 * All coords are generally used as top-left position of camera unless specified otherwise
 */

#include <ace/types.h>
#include <ace/utils/extview.h>

typedef struct {
	tVpManager sCommon;
	tUwCoordYX uPos;      /// Current camera pos
	tUwCoordYX uLastPos;  /// Previous camera pos
	tUwCoordYX uMaxPos;   /// Max camera pos: world W&H - camera W&H
} tCameraManager;

tCameraManager *cameraCreate(
	tVPort *pVPort, UWORD uwPosX, UWORD uwPosY, UWORD uwMaxX, UWORD uwMaxY
);

void cameraDestroy(tCameraManager *pManager);
void cameraProcess(tCameraManager *pManager);

void cameraReset(
	tCameraManager *pManager,
	UWORD uwPosX, UWORD uwPosY, UWORD uwMaxX, UWORD uwMaxY
);

void cameraSetCoord(tCameraManager *pManager, UWORD uwX, UWORD uwY);

void cameraMoveBy(tCameraManager *pManager, WORD wDx, WORD wDy);

void cameraCenterAt(tCameraManager *pManager, UWORD uwAvgX, UWORD uwAvgY);

UBYTE cameraIsMoved(tCameraManager *pManager);

UWORD cameraGetXDiff(tCameraManager *pManager);

UWORD cameraGetYDiff(tCameraManager *pManager);

WORD cameraGetDeltaX(tCameraManager *pManager);

WORD cameraGetDeltaY(tCameraManager *pManager);

#endif // _ACE_MANAGERS_VIEWPORT_CAMERA_H_
