/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GUARD_ACE_MANAGER_VIEWPORT_CAMERA_H
#define GUARD_ACE_MANAGER_VIEWPORT_CAMERA_H

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
	IN tVPort *pVPort,
	IN UWORD uwPosX,
	IN UWORD uwPosY,
	IN UWORD uwMaxX,
	IN UWORD uwMaxY
);

void cameraDestroy(
	IN tCameraManager *pManager
);
void cameraProcess(
	IN tCameraManager *pManager
);

void cameraReset(
	IN tCameraManager *pManager,
	IN UWORD uwPosX,
	IN UWORD uwPosY,
	IN UWORD uwMaxX,
	IN UWORD uwMaxY
);

void cameraSetCoord(
	IN tCameraManager *pManager,
	IN UWORD uwX,
	IN UWORD uwY
);

void cameraMoveBy(
	IN tCameraManager *pManager,
	IN WORD wDx,
	IN WORD wDy
);

void cameraCenterAt(
	IN tCameraManager *pManager,
	IN UWORD uwAvgX,
	IN UWORD uwAvgY
);

UBYTE cameraIsMoved(
	IN tCameraManager *pManager
);

UWORD cameraGetXDiff(
	IN tCameraManager *pManager
);

UWORD cameraGetYDiff(
	IN tCameraManager *pManager
);

WORD cameraGetDeltaX(
	IN tCameraManager *pManager
);

WORD cameraGetDeltaY(
	IN tCameraManager *pManager
);

#endif
