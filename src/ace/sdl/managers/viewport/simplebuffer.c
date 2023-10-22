/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/viewport/simplebuffer_private.h>
#include <ace/managers/sdl_private.h>
#include <ace/managers/blit.h>

tSimpleBufferManager *simpleBufferCreate(void *pTags,	...) {
	va_list vaTags;
	tSimpleBufferManager *pManager;
	UWORD uwBoundWidth, uwBoundHeight;
	UBYTE ubBitmapFlags;
	tBitMap *pFront = 0, *pBack = 0;
	UBYTE isCameraCreated = 0;

	logBlockBegin("simpleBufferCreate(pTags: %p, ...)", pTags);
	va_start(vaTags, pTags);

	// Init manager
	pManager = memAllocFastClear(sizeof(tSimpleBufferManager));
	pManager->sCommon.process = (tVpManagerFn)simpleBufferProcess;
	pManager->sCommon.destroy = (tVpManagerFn)simpleBufferDestroy;
#if defined(ACE_SDL)
	pManager->sCommon.cbDrawToSurface = (tVpManagerFn)simpleBufferDrawToSurface;
#endif
	pManager->sCommon.ubId = VPM_SCROLL;
	logWrite("Addr: %p\n", pManager);

	tVPort *pVPort = (tVPort*)tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_VPORT, 0);
	if(!pVPort) {
		logWrite("ERR: No parent viewport (TAG_SIMPLEBUFFER_VPORT) specified!\n");
		goto fail;
	}
	pManager->sCommon.pVPort = pVPort;
	logWrite("Parent VPort: %p\n", pVPort);

	// Buffer bitmap
	uwBoundWidth = tagGet(
		pTags, vaTags, TAG_SIMPLEBUFFER_BOUND_WIDTH, pVPort->uwWidth
	);
	uwBoundHeight = tagGet(
		pTags, vaTags, TAG_SIMPLEBUFFER_BOUND_HEIGHT, pVPort->uwHeight
	);
	ubBitmapFlags = tagGet(
		pTags, vaTags, TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR
	);
	logWrite("Bounds: %ux%u\n", uwBoundWidth, uwBoundHeight);
	pFront = bitmapCreate(
		uwBoundWidth, uwBoundHeight, pVPort->ubBPP, ubBitmapFlags
	);
	if(!pFront) {
		logWrite("ERR: Can't alloc buffer bitmap!\n");
		goto fail;
	}

	UBYTE isDblBfr = tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_IS_DBLBUF, 0);
	if(isDblBfr) {
		pBack = bitmapCreate(
			uwBoundWidth, uwBoundHeight, pVPort->ubBPP, ubBitmapFlags
		);
		if(!pBack) {
			logWrite("ERR: Can't alloc buffer bitmap!\n");
			goto fail;
		}
	}

	// Find camera manager, create if not exists
	pManager->pCamera = (tCameraManager*)vPortGetManager(pVPort, VPM_CAMERA);
	if(!pManager->pCamera) {
		isCameraCreated = 1;
		pManager->pCamera = cameraCreate(
			pVPort, 0, 0, uwBoundWidth, uwBoundHeight, isDblBfr
		);
	}

	UBYTE isScrollX = tagGet(pTags, vaTags, TAG_SIMPLEBUFFER_USE_X_SCROLLING, 1);
	simpleBufferSetFront(pManager, pFront);
	simpleBufferSetBack(pManager, pBack ? pBack : pFront);

	// Add manager to VPort
	vPortAddManager(pVPort, (tVpManager*)pManager);
	logBlockEnd("simpleBufferCreate()");
	va_end(vaTags);
	return pManager;

fail:
	if(pBack && pBack != pFront) {
		bitmapDestroy(pBack);
	}
	if(pFront) {
		bitmapDestroy(pFront);
	}
	if(pManager) {
		if(pManager->pCamera && isCameraCreated) {
			cameraDestroy(pManager->pCamera);
		}
		memFree(pManager, sizeof(tSimpleBufferManager));
	}
	logBlockEnd("simpleBufferCreate()");
	va_end(vaTags);
	return 0;
}

void simpleBufferDestroy(tSimpleBufferManager *pManager) {
	logBlockBegin("simpleBufferDestroy()");
	if(pManager->pBack != pManager->pFront) {
		bitmapDestroy(pManager->pBack);
	}
	bitmapDestroy(pManager->pFront);
	memFree(pManager, sizeof(tSimpleBufferManager));
	logBlockEnd("simpleBufferDestroy()");
}

void simpleBufferProcess(tSimpleBufferManager *pManager) {
	tBitMap *pTmp = pManager->pBack;
	pManager->pBack = pManager->pFront;
	pManager->pFront = pTmp;
}

UBYTE simpleBufferGetRawCopperlistInstructionCount(UBYTE ubBpp) {
	return 0;
}

void simpleBufferDrawToSurface(tSimpleBufferManager *pManager) {
	tBitMap *pDest = sdlGetSurfaceBitmap();
	tVPort *pVPort = pManager->sCommon.pVPort;

	// TODO: replace with blitCopy() when it's fully implemented
	blitCopyAligned(
	// blitCopy(
		pManager->pFront,
		pManager->pCamera->uPos.uwX, pManager->pCamera->uPos.uwY,
		pDest, pVPort->uwOffsX, pVPort->uwOffsY, pVPort->uwWidth, pVPort->uwHeight
		// MINTERM_COOKIE
	);
}
