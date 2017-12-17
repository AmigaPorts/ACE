#include <ace/managers/mouse.h>
#include <ace/managers/log.h>
#include <ace/utils/custom.h>
#include <ace/generic/screen.h>

/* Globals */
tMouseManager g_sMouseManager;

static UBYTE s_ubPrevX, s_ubPrevY;

/* Functions */
void mouseCreate() {
#ifdef AMIGA
	g_sMouseManager.uwMinX = 0;
	g_sMouseManager.uwMinY = 0;
	g_sMouseManager.uwMaxX = SCREEN_PAL_WIDTH;
	g_sMouseManager.uwMaxY = SCREEN_PAL_HEIGHT;
	g_sMouseManager.uwX = (g_sMouseManager.uwMaxX - g_sMouseManager.uwMinX) >> 1;
	g_sMouseManager.uwY = (g_sMouseManager.uwMaxY - g_sMouseManager.uwMinY) >> 1;
	s_ubPrevX = 0;
	s_ubPrevY = 0;

	// Enable RMB & MMB on port 1
	g_sMouseManager.uwPrevPotGo = custom.potinp;
	UWORD uwPotMask = (1 << 11) | (1 << 10) | (1 << 9) | (1 << 8);
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

void mouseSetBounds(UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight) {
	g_sMouseManager.uwMinX = uwX;
	g_sMouseManager.uwMinY = uwY;
	g_sMouseManager.uwMaxX = uwWidth;
	g_sMouseManager.uwMaxY = uwHeight;
}

void mouseProcess(void) {
	// Even if whole Amiga process will be moved to vbl interrupt, other platforms
	// will prob'ly use this fn anyway
#ifdef AMIGA
	UWORD uwMousePos = custom.joy0dat;
	UBYTE ubPosX = uwMousePos & 0xFF;
	UBYTE ubPosY = uwMousePos >> 8;

	BYTE bDx = ubPosX - s_ubPrevX;
	BYTE bDy = ubPosY - s_ubPrevY;
	mouseMoveBy(bDx, bDy);

	s_ubPrevX = ubPosX;
	s_ubPrevY = ubPosY;
	volatile UWORD * const pCiaAPra = (UWORD*)((UBYTE*)0xBFE001);

	// Left button state
	if(*pCiaAPra & (1 << 6))
		mouseSetButton(MOUSE_LMB, MOUSE_NACTIVE);
	else if(!mouseCheck(MOUSE_LMB))
		mouseSetButton(MOUSE_LMB, MOUSE_ACTIVE);

	// Right button state
	if(custom.potinp & (1 << 10))
		mouseSetButton(MOUSE_RMB, MOUSE_NACTIVE);
	else if(!mouseCheck(MOUSE_RMB))
		mouseSetButton(MOUSE_RMB, MOUSE_ACTIVE);

	// Middle button state
	if(custom.potinp & (1 << 8))
		mouseSetButton(MOUSE_MMB, MOUSE_NACTIVE);
	else if(!mouseCheck(MOUSE_MMB))
		mouseSetButton(MOUSE_MMB, MOUSE_ACTIVE);

#endif // AMIGA
}
