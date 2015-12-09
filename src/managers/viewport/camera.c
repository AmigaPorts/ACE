#include "camera.h"

tCameraManager *cameraCreate(tVPort *pVPort, UWORD uwPosX, UWORD uwPosY, UWORD uwMaxX, UWORD uwMaxY) {
	logBlockBegin("cameraCreate(pVPort: %p, uwPosX: %u, uwPosY: %u, uwMaxX: %u, uwMaxY: %u)", pVPort, uwPosX, uwPosY, uwMaxX, uwMaxY);
	tCameraManager *pManager;
	
	pManager = memAllocFast(sizeof(tCameraManager));
	logWrite("Addr: %p\n", pManager);
	pManager->sCommon.pNext = 0;
	pManager->sCommon.process = (tVpManagerFn)cameraProcess;
	pManager->sCommon.destroy = (tVpManagerFn)cameraDestroy;
	pManager->sCommon.pVPort = pVPort;
	pManager->sCommon.ubId = VPM_CAMERA;
	
	cameraReset(pManager, uwPosX, uwPosY, uwMaxX, uwMaxY);
	
	vPortAddManager(pVPort, (tVpManager*)pManager);
	logBlockEnd("cameraCreate()");
	return pManager;
}

void cameraDestroy(tCameraManager *pManager) {
	logWrite("cameraManagerDestroy...");
	memFree(pManager, sizeof(tCameraManager));
	logWrite("OK! \n");
}

inline void cameraProcess(tCameraManager *pManager) {
	pManager->uLastPos.ulYX = pManager->uPos.ulYX;
}

void cameraReset(tCameraManager *pManager, UWORD uwStartX, UWORD uwStartY, UWORD uwWidth, UWORD uwHeight) {
	logBlockBegin("cameraReset(pManager: %p, uwStartX: %u, uwStartY: %u, uwWidth: %u, uwHeight: %u)", pManager, uwStartX, uwStartY, uwWidth, uwHeight);
	
	pManager->uPos.sUwCoord.uwX = uwStartX;
	pManager->uPos.sUwCoord.uwY = uwStartY;
	pManager->uLastPos.sUwCoord.uwX = uwStartX;
	pManager->uLastPos.sUwCoord.uwY = uwStartY;
	
	// Max camera coords based on viewport size
	pManager->uMaxPos.sUwCoord.uwX = uwWidth - pManager->sCommon.pVPort->uwWidth;
	pManager->uMaxPos.sUwCoord.uwY = uwHeight - pManager->sCommon.pVPort->uwHeight;	
	logWrite("Camera max coord: %u,%u\n", pManager->uMaxPos.sUwCoord.uwX, pManager->uMaxPos.sUwCoord.uwY);
	
	logBlockEnd("cameraReset()");
}

inline void cameraSetCoord(tCameraManager *pManager, UWORD uwX, UWORD uwY) {
	pManager->uPos.sUwCoord.uwX = uwX;
	pManager->uPos.sUwCoord.uwY = uwY;
	// logWrite("New camera pos: %u,%u\n", uwX, uwY);
}

void cameraMove(tCameraManager *pManager, WORD wX, WORD wY) {
	WORD wTmp;
	
	// musz¹ byæ tutaj bo potem nie rozkminimy czy wynik jest ujemny,
	// chyba ¿e tCoord bêdzie trzymaæ WORD zamiast UWORD
	wTmp = pManager->uPos.sUwCoord.uwX + wX;
	if (wTmp > 0) { // Margines z lewej
		pManager->uPos.sUwCoord.uwX = wTmp;
		if (pManager->uPos.sUwCoord.uwX > pManager->uMaxPos.sUwCoord.uwX)
			pManager->uPos.sUwCoord.uwX = pManager->uMaxPos.sUwCoord.uwX;
	}
	else
		pManager->uPos.sUwCoord.uwX = 0;
	
	wTmp = pManager->uPos.sUwCoord.uwY + wY;
	if (wTmp > 0) { // Margines z góry
		pManager->uPos.sUwCoord.uwY = wTmp;
		if (pManager->uPos.sUwCoord.uwY > pManager->uMaxPos.sUwCoord.uwY)
			pManager->uPos.sUwCoord.uwY = pManager->uMaxPos.sUwCoord.uwY;
	}
	else
		pManager->uPos.sUwCoord.uwY = 0;
}

void cameraCenterAt(tCameraManager *pManager, UWORD uwAvgX, UWORD uwAvgY) {
	tVPort *pVPort;
	
	pVPort = pManager->sCommon.pVPort;
	
	// Limit lewy górny
	if(uwAvgX < pVPort->uwWidth>>1)
		uwAvgX = 0;
	else
		uwAvgX -= pVPort->uwWidth>>1;
	
	if(uwAvgY < pVPort->uwHeight>>1)
		uwAvgY = 0;
	else
		uwAvgY -= pVPort->uwHeight>>1;
	
	// Limit prawy dolny
	if(uwAvgX > pManager->uMaxPos.sUwCoord.uwX)
		uwAvgX = pManager->uMaxPos.sUwCoord.uwX;
	if(uwAvgY > pManager->uMaxPos.sUwCoord.uwY)
		uwAvgY = pManager->uMaxPos.sUwCoord.uwY;
	
	cameraSetCoord(pManager, uwAvgX, uwAvgY);
}

inline UBYTE cameraIsMoved(tCameraManager *pManager) {
	return pManager->uPos.ulYX != pManager->uLastPos.ulYX;
}

inline UWORD cameraGetXDiff(tCameraManager *pManager) {
	return abs(cameraGetDeltaX(pManager));
}

inline UWORD cameraGetYDiff(tCameraManager *pManager) {
	return abs(cameraGetDeltaX(pManager));
}

inline WORD cameraGetDeltaX(tCameraManager *pManager) {
	return (pManager->uPos.sUwCoord.uwX - pManager->uLastPos.sUwCoord.uwX);
}

inline WORD cameraGetDeltaY(tCameraManager *pManager) {
	return (pManager->uPos.sUwCoord.uwY - pManager->uLastPos.sUwCoord.uwY);
}