/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_VIEWPORT_CAMERA_H_
#define _ACE_MANAGERS_VIEWPORT_CAMERA_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 2D Camera manager
 * Keeps track of previous and current XY
 * Datasource only - scrolling etc. should be done as separate managers
 * All coords are generally used as top-left position of camera unless specified otherwise
 */

#include <ace/types.h>
#include <ace/utils/extview.h>

typedef struct _tCameraManager {
	tVpManager sCommon;
	tUwCoordYX uPos;        ///< Current camera pos
	tUwCoordYX uLastPos[2]; ///< Previous camera pos
	tUwCoordYX uMaxPos;     ///< Max camera pos: world W&H - camera W&H
	UBYTE ubBfr;            ///< Currently used buffer for double buffering
	UBYTE isDblBfr;
#ifdef ACE_USE_AGA_FEATURES
	UBYTE ubFineX;          ///< 35ns remainder: 0-3 lores (1/4 px), 0-1 hires (1/2 px)
	UBYTE ubLastFineX[2];   ///< Previous fine X per copper buffer
#endif
} tCameraManager;

tCameraManager *cameraCreate(
	tVPort *pVPort, UWORD uwPosX, UWORD uwPosY, UWORD uwMaxX, UWORD uwMaxY,
	UBYTE isDblBfr
);

void cameraDestroy(tCameraManager *pManager);
void cameraProcess(tCameraManager *pManager);

void cameraReset(
	tCameraManager *pManager,
	UWORD uwPosX, UWORD uwPosY, UWORD uwMaxX, UWORD uwMaxY, UBYTE isDblBfr
);

void cameraSetCoord(tCameraManager *pManager, UWORD uwX, UWORD uwY);

void cameraMoveBy(tCameraManager *pManager, WORD wDx, WORD wDy);

#ifdef ACE_USE_AGA_FEATURES
/**
 * @brief Sets the AGA fine (35ns) X remainder. Clamped to the current resolution
 * (0-3 lores, 0-1 hires). Forced to 0 when the camera is at max X.
 */
void cameraSetFineX(tCameraManager *pManager, UBYTE ubFineX);

/**
 * @brief Moves the camera by a 35ns X delta plus integer Y.
 * Fine X carries into @c uPos.uwX using 4 units/pixel (lores) or 2 (hires).
 */
void cameraMoveByFine(tCameraManager *pManager, WORD wDxFine, WORD wDy);
#endif

static inline UBYTE cameraGetFineX(const tCameraManager *pManager) {
#ifdef ACE_USE_AGA_FEATURES
	return pManager->ubFineX;
#else
	(void)pManager;
	return 0;
#endif
}

void cameraCenterAt(tCameraManager *pManager, UWORD uwAvgX, UWORD uwAvgY);

UBYTE cameraIsMoved(const tCameraManager *pManager);

UWORD cameraGetXDiff(const tCameraManager *pManager);

UWORD cameraGetYDiff(const tCameraManager *pManager);

WORD cameraGetDeltaX(const tCameraManager *pManager);

WORD cameraGetDeltaY(const tCameraManager *pManager);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_VIEWPORT_CAMERA_H_
