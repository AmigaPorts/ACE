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
		uwPotMask |= BV(11) | BV(10) | BV(9) | BV(8);
		memset(&g_sMouseManager.pMouses[MOUSE_PORT_1], 0, sizeof(tMouse));
		g_sMouseManager.pMouses[MOUSE_PORT_1].uwX = (g_sMouseManager.uwHiX - g_sMouseManager.uwLoX) >> 1;
		g_sMouseManager.pMouses[MOUSE_PORT_1].uwY = (g_sMouseManager.uwHiY - g_sMouseManager.uwHiY) >> 1;
	}
	if(ubPortFlags & MOUSE_PORT_2) {
		uwPotMask |= BV(15) | BV(14) | BV(13) | BV(12);
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

static void mouseProcessPort(
	UBYTE ubPort, UWORD uwPosReg,
	UBYTE ubStateLmb, UBYTE ubStateRmb, UBYTE ubStateMmb
) {
	// Deltas are signed bytes even though underflows and overflows may occur.
	// It is expected behavior since it is encouraged in Amiga HRM as means to
	// determine mouse movement direction which takes into account joyxdat
	// underflows and overflows.

	UBYTE ubPosX = uwPosReg & 0xFF;
	UBYTE ubPosY = uwPosReg >> 8;

	BYTE bDx = ubPosX - g_sMouseManager.pMouses[ubPort].ubPrevHwX;
	BYTE bDy = ubPosY - g_sMouseManager.pMouses[ubPort].ubPrevHwY;
	mouseMoveBy(ubPort, bDx, bDy);

	g_sMouseManager.pMouses[ubPort].ubPrevHwX = ubPosX;
	g_sMouseManager.pMouses[ubPort].ubPrevHwY = ubPosY;

	// Left button state
	if(ubStateLmb)
		mouseSetButton(ubPort, MOUSE_LMB, MOUSE_NACTIVE);
	else if(!mouseCheck(ubPort, MOUSE_LMB))
		mouseSetButton(ubPort, MOUSE_LMB, MOUSE_ACTIVE);

	// Right button state
	if(ubStateRmb)
		mouseSetButton(ubPort, MOUSE_RMB, MOUSE_NACTIVE);
	else if(!mouseCheck(ubPort, MOUSE_RMB))
		mouseSetButton(ubPort, MOUSE_RMB, MOUSE_ACTIVE);

	// Middle button state
	if(ubStateMmb)
		mouseSetButton(ubPort, MOUSE_MMB, MOUSE_NACTIVE);
	else if(!mouseCheck(ubPort, MOUSE_MMB))
		mouseSetButton(ubPort, MOUSE_MMB, MOUSE_ACTIVE);
}

void mouseProcess(void) {
	// Even if whole Amiga process will be moved to vbl interrupt, other platforms
	// will prob'ly use this fn anyway
#ifdef AMIGA

	if(g_sMouseManager.ubPortFlags & MOUSE_PORT_1) {
		mouseProcessPort(
			MOUSE_PORT_1, custom.joy0dat,
			g_pCiaA->pra & BV(6), custom.potinp & BV(10), custom.potinp & BV(8)
		);
	}
	if(g_sMouseManager.ubPortFlags & MOUSE_PORT_2) {
		mouseProcessPort(
			MOUSE_PORT_2, custom.joy1dat,
			g_pCiaA->pra & BV(7), custom.potinp & BV(14), custom.potinp & BV(12)
		);
	}

#endif // AMIGA
}
