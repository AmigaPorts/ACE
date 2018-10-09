/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/viewport/camera.h>
#include <ace/macros.h>

tCameraManager *cameraCreate(
	tVPort *pVPort, UWORD uwPosX, UWORD uwPosY, UWORD uwMaxX, UWORD uwMaxY,
	UBYTE isDblBfr
) {
	logBlockBegin(
		"cameraCreate(pVPort: %p, uwPosX: %u, uwPosY: %u, uwMaxX: %u, uwMaxY: %u, isDblBfr: %hhu)",
		pVPort, uwPosX, uwPosY, uwMaxX, uwMaxY, isDblBfr
	);
	tCameraManager *pManager;

	pManager = memAllocFastClear(sizeof(tCameraManager));
	logWrite("Addr: %p\n", pManager);
	pManager->sCommon.process = (tVpManagerFn)cameraProcess;
	pManager->sCommon.destroy = (tVpManagerFn)cameraDestroy;
	pManager->sCommon.pVPort = pVPort;
	pManager->sCommon.ubId = VPM_CAMERA;

	logWrite("Resetting camera bounds...\n");
	cameraReset(pManager, uwPosX, uwPosY, uwMaxX, uwMaxY, isDblBfr);

	logWrite("Attaching camera to VPort...\n");
	vPortAddManager(pVPort, (tVpManager*)pManager);
	logBlockEnd("cameraCreate()");
	return pManager;
}

void cameraDestroy(tCameraManager *pManager) {
	logWrite("cameraManagerDestroy...");
	memFree(pManager, sizeof(tCameraManager));
	logWrite("OK! \n");
}

void cameraProcess(tCameraManager *pManager) {
	pManager->uLastPos[pManager->ubBfr].ulYX = pManager->uPos.ulYX;
	if(pManager->isDblBfr) {
		pManager->ubBfr = !pManager->ubBfr;
	}
}

void cameraReset(
	tCameraManager *pManager,
	UWORD uwStartX, UWORD uwStartY, UWORD uwWidth, UWORD uwHeight, UBYTE isDblBfr
) {
	logBlockBegin(
		"cameraReset(pManager: %p, uwStartX: %u, uwStartY: %u, uwWidth: %u, uwHeight: %u, isDblBfr: %hhu)",
		pManager, uwStartX, uwStartY, uwWidth, uwHeight, isDblBfr
	);

	pManager->uPos.sUwCoord.uwX = uwStartX;
	pManager->uPos.sUwCoord.uwY = uwStartY;
	pManager->uLastPos[0].sUwCoord.uwX = uwStartX;
	pManager->uLastPos[0].sUwCoord.uwY = uwStartY;
	pManager->uLastPos[1].sUwCoord.uwX = uwStartX;
	pManager->uLastPos[1].sUwCoord.uwY = uwStartY;
	pManager->isDblBfr = isDblBfr;
	pManager->ubBfr = 0;

	// Max camera coords based on viewport size
	pManager->uMaxPos.sUwCoord.uwX = uwWidth - pManager->sCommon.pVPort->uwWidth;
	pManager->uMaxPos.sUwCoord.uwY = uwHeight - pManager->sCommon.pVPort->uwHeight;
	logWrite("Camera max coord: %u,%u\n", pManager->uMaxPos.sUwCoord.uwX, pManager->uMaxPos.sUwCoord.uwY);

	logBlockEnd("cameraReset()");
}

void cameraSetCoord(tCameraManager *pManager, UWORD uwX, UWORD uwY) {
	pManager->uPos.sUwCoord.uwX = uwX;
	pManager->uPos.sUwCoord.uwY = uwY;
	// logWrite("New camera pos: %u,%u\n", uwX, uwY);
}

void cameraMoveBy(tCameraManager *pManager, WORD wDx, WORD wDy) {
	pManager->uPos.sUwCoord.uwX = CLAMP(pManager->uPos.sUwCoord.uwX+wDx, 0, pManager->uMaxPos.sUwCoord.uwX);
	pManager->uPos.sUwCoord.uwY = CLAMP(pManager->uPos.sUwCoord.uwY+wDy, 0, pManager->uMaxPos.sUwCoord.uwY);
}

void cameraCenterAt(tCameraManager *pManager, UWORD uwAvgX, UWORD uwAvgY) {
	tVPort *pVPort;

	pVPort = pManager->sCommon.pVPort;
	pManager->uPos.sUwCoord.uwX = CLAMP(uwAvgX - (pVPort->uwWidth>>1), 0, pManager->uMaxPos.sUwCoord.uwX);
	pManager->uPos.sUwCoord.uwY = CLAMP(uwAvgY - (pVPort->uwHeight>>1), 0, pManager->uMaxPos.sUwCoord.uwY);
}

UBYTE cameraIsMoved(tCameraManager *pManager) {
	return pManager->uPos.ulYX != pManager->uLastPos[pManager->ubBfr].ulYX;
}

UWORD cameraGetXDiff(tCameraManager *pManager) {
	return ABS(cameraGetDeltaX(pManager));
}

UWORD cameraGetYDiff(tCameraManager *pManager) {
	return ABS(cameraGetDeltaX(pManager));
}

WORD cameraGetDeltaX(tCameraManager *pManager) {
	return (pManager->uPos.sUwCoord.uwX - pManager->uLastPos[pManager->ubBfr].sUwCoord.uwX);
}

WORD cameraGetDeltaY(tCameraManager *pManager) {
	return (pManager->uPos.sUwCoord.uwY - pManager->uLastPos[pManager->ubBfr].sUwCoord.uwY);
}
