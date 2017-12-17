#include <ace/managers/mouse.h>
#include <ace/managers/log.h>
#include <ace/utils/custom.h>
#include <ace/generic/screen.h>

/* Globals */
tMouseManager g_sMouseManager;

/* Functions */
void mouseCreate(UBYTE ubPortFlags) {
#ifdef AMIGA
	mouseSetBounds(0, 0, SCREEN_PAL_WIDTH, SCREEN_PAL_HEIGHT);

	g_sMouseManager.ubPortFlags = ubPortFlags;
	g_sMouseManager.uwPrevPotGo = custom.potinp;
	UWORD uwPotMask = 0;

	// Enable RMB & MMB
	if(ubPortFlags & MOUSE_PORT_1) {
		uwPotMask |= (1 << 11) | (1 << 10) | (1 << 9) | (1 << 8);
		memset(&g_sMouseManager.pMouses[MOUSE_PORT_1], 0, sizeof(tMouse));
		g_sMouseManager.pMouses[MOUSE_PORT_1].uwX = (g_sMouseManager.uwHiX - g_sMouseManager.uwLoX) >> 1;
		g_sMouseManager.pMouses[MOUSE_PORT_1].uwY = (g_sMouseManager.uwHiY - g_sMouseManager.uwHiY) >> 1;
	}
	if(ubPortFlags & MOUSE_PORT_2) {
		uwPotMask |= (1 << 15) | (1 << 14) | (1 << 13) | (1 << 12);
		memset(&g_sMouseManager.pMouses[MOUSE_PORT_2], 0, sizeof(tMouse));
		g_sMouseManager.pMouses[MOUSE_PORT_2].uwX = (g_sMouseManager.uwHiX - g_sMouseManager.uwLoX) >> 1;
		g_sMouseManager.pMouses[MOUSE_PORT_2].uwY = (g_sMouseManager.uwHiY - g_sMouseManager.uwLoY) >> 1;
	}
	custom.potgo = (custom.potinp & (0xFFFF ^ uwPotMask)) | uwPotMask;

	// Amiga Hardware Reference Manual suggests that pos should be polled every
	// vblank, so there could be some interrupt init.
#endif // AMIGA
}

void mouseDestroy(void) {
#ifdef AMIGA
	// Should mouse manager be interrupt driven, interrupt handler deletion will
	// be here.
	custom.potgo = g_sMouseManager.uwPrevPotGo;
#endif // AMIGA
}

void mouseSetBounds(UWORD uwLoX, UWORD uwLoY, UWORD uwHiX, UWORD uwHiY) {
	g_sMouseManager.uwLoX = uwLoX;
	g_sMouseManager.uwLoY = uwLoY;
	g_sMouseManager.uwHiX = uwHiX;
	g_sMouseManager.uwHiY = uwHiY;
}

void mouseProcess(void) {
	// Even if whole Amiga process will be moved to vbl interrupt, other platforms
	// will prob'ly use this fn anyway
#ifdef AMIGA
	volatile UWORD * const pCiaAPra = (UWORD*)((UBYTE*)0xBFE001);

	if(g_sMouseManager.ubPortFlags & MOUSE_PORT_1) {
		// Movement
		UWORD uwMousePos = custom.joy0dat;
		UBYTE ubPosX = uwMousePos & 0xFF;
		UBYTE ubPosY = uwMousePos >> 8;

		BYTE bDx = ubPosX - g_sMouseManager.pMouses[MOUSE_PORT_1].ubPrevHwX;
		BYTE bDy = ubPosY - g_sMouseManager.pMouses[MOUSE_PORT_1].ubPrevHwY;
		mouseMoveBy(MOUSE_PORT_1, bDx, bDy);

		g_sMouseManager.pMouses[MOUSE_PORT_1].ubPrevHwX = ubPosX;
		g_sMouseManager.pMouses[MOUSE_PORT_1].ubPrevHwY = ubPosY;

		// Left button state
		if(*pCiaAPra & (1 << 6))
			mouseSetButton(MOUSE_PORT_1, MOUSE_LMB, MOUSE_NACTIVE);
		else if(!mouseCheck(MOUSE_PORT_1, MOUSE_LMB))
			mouseSetButton(MOUSE_PORT_1, MOUSE_LMB, MOUSE_ACTIVE);

		// Right button state
		if(custom.potinp & (1 << 10))
			mouseSetButton(MOUSE_PORT_1, MOUSE_RMB, MOUSE_NACTIVE);
		else if(!mouseCheck(MOUSE_PORT_1, MOUSE_RMB))
			mouseSetButton(MOUSE_PORT_1, MOUSE_RMB, MOUSE_ACTIVE);

		// Middle button state
		if(custom.potinp & (1 << 8))
			mouseSetButton(MOUSE_PORT_1, MOUSE_MMB, MOUSE_NACTIVE);
		else if(!mouseCheck(MOUSE_PORT_1, MOUSE_MMB))
			mouseSetButton(MOUSE_PORT_1, MOUSE_MMB, MOUSE_ACTIVE);
	}
	if(g_sMouseManager.ubPortFlags & MOUSE_PORT_2) {
		// Movement
		UWORD uwMousePos = custom.joy1dat;
		UBYTE ubPosX = uwMousePos & 0xFF;
		UBYTE ubPosY = uwMousePos >> 8;

		BYTE bDx = ubPosX - g_sMouseManager.pMouses[MOUSE_PORT_2].ubPrevHwX;
		BYTE bDy = ubPosY - g_sMouseManager.pMouses[MOUSE_PORT_2].ubPrevHwY;
		mouseMoveBy(MOUSE_PORT_2, bDx, bDy);

		g_sMouseManager.pMouses[MOUSE_PORT_2].ubPrevHwX = ubPosX;
		g_sMouseManager.pMouses[MOUSE_PORT_2].ubPrevHwY = ubPosY;

		// Left button state
		if(*pCiaAPra & (1 << 7))
			mouseSetButton(MOUSE_PORT_2, MOUSE_LMB, MOUSE_NACTIVE);
		else if(!mouseCheck(MOUSE_PORT_2, MOUSE_LMB))
			mouseSetButton(MOUSE_PORT_2, MOUSE_LMB, MOUSE_ACTIVE);

		// Right button state
		if(custom.potinp & (1 << 14))
			mouseSetButton(MOUSE_PORT_2, MOUSE_RMB, MOUSE_NACTIVE);
		else if(!mouseCheck(MOUSE_PORT_2, MOUSE_RMB))
			mouseSetButton(MOUSE_PORT_2, MOUSE_RMB, MOUSE_ACTIVE);

		// Middle button state
		if(custom.potinp & (1 << 12))
			mouseSetButton(MOUSE_PORT_2, MOUSE_MMB, MOUSE_NACTIVE);
		else if(!mouseCheck(MOUSE_PORT_2, MOUSE_MMB))
			mouseSetButton(MOUSE_PORT_2, MOUSE_MMB, MOUSE_ACTIVE);
	}

#endif // AMIGA
}
