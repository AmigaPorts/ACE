#ifndef GUARD_ACE_MANAGER_VIEWPORT_CAMERA_H
#define GUARD_ACE_MANAGER_VIEWPORT_CAMERA_H

/**
 * 2D Camera manager
 * Keeps track of previous and current XY
 * Datasource only - scrolling etc. should be done as separate managers
 * All coords are generally used as top-left position of camera unless specified otherwise
 */

#include "types.h"
#include "macros.h"
#include "config.h"
#include "utils/extview.h"

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

inline void cameraReset(
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

void cameraMove(
	IN tCameraManager *pManager,
	IN WORD wX,
	IN WORD wY
);

void cameraCenterAt(
	IN tCameraManager *pManager,
	IN UWORD uwAvgX,
	IN UWORD uwAvgY
);

inline UBYTE cameraIsMoved(
	IN tCameraManager *pManager
);

inline UWORD cameraGetXDiff(
	IN tCameraManager *pManager
);

inline UWORD cameraGetYDiff(
	IN tCameraManager *pManager
);

inline WORD cameraGetDeltaX(
	IN tCameraManager *pManager
);

inline WORD cameraGetDeltaY(
	IN tCameraManager *pManager
);


#endif