#include <ace/managers/mouse.h>
#include <ace/macros.h>
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
	// Amiga Hardware Reference Manual suggests that pos should be polled every
	// vblank, so there could be some interrupt init.
#endif // AMIGA
}

void mouseDestroy(void) {
#ifdef AMIGA
	// Should mouse manager be interrupt driven, interrupt handler deletion will
	// be here.
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

	BYTE wDx = ubPosX - s_ubPrevX;
	BYTE wDy = ubPosY - s_ubPrevY;
	if(ABS(wDx) <= 127) {
		g_sMouseManager.uwX = CLAMP(
			g_sMouseManager.uwX + wDx, g_sMouseManager.uwMinX, g_sMouseManager.uwMaxX
		);
	}
	else {
		g_sMouseManager.uwX = CLAMP(
			g_sMouseManager.uwX - wDx, g_sMouseManager.uwMinX, g_sMouseManager.uwMaxX
		);
	}

	if(ABS(wDy) <= 127) {
		g_sMouseManager.uwY = CLAMP(
			g_sMouseManager.uwY + wDy, g_sMouseManager.uwMinY, g_sMouseManager.uwMaxY
		);
	}
	else {
		g_sMouseManager.uwY = CLAMP(
			g_sMouseManager.uwY + wDy, g_sMouseManager.uwMinY, g_sMouseManager.uwMaxY
		);
	}

	s_ubPrevX = ubPosX;
	s_ubPrevY = ubPosY;

	// Button states

#endif // AMIGA
}

void mouseSetState(UBYTE ubMouseCode, UBYTE ubMouseState) {
	g_sMouseManager.pButtonStates[ubMouseCode] = ubMouseState;
}

UBYTE mouseCheck(UBYTE ubMouseCode) {
	return g_sMouseManager.pButtonStates[ubMouseCode] != MOUSE_NACTIVE;
}

UBYTE mouseUse(UBYTE ubMouseCode) {
	if (g_sMouseManager.pButtonStates[ubMouseCode] == MOUSE_ACTIVE) {
		g_sMouseManager.pButtonStates[ubMouseCode] = MOUSE_USED;
		return 1;
	}
	return 0;
}

UBYTE mouseIsIntersects(UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight) {
#ifdef AMIGA
	// return (uwX <= g_sWindowManager.pWindow->MouseX) && (g_sWindowManager.pWindow->MouseX < uwX + uwWidth) && (uwY <= g_sWindowManager.pWindow->MouseY) && (g_sWindowManager.pWindow->MouseY < uwY + uwHeight);
#else
	return 0;
#endif // AMIGA
}

UWORD mouseGetX(void) {
#ifdef AMIGA
	return g_sMouseManager.uwX;
#else
	return 0;
#endif // AMIGA
}

UWORD mouseGetY(void) {
#ifdef AMIGA
	return g_sMouseManager.uwY;
#else
	return 0;
#endif // AMIGA
}

void mouseSetPointer(UWORD *pCursor, WORD wHeight, WORD wWidth, WORD wOffsetX, WORD wOffsetY) {
	logBlockBegin("mouseSetPointer(pCursor: %p, wHeight: %d, wWidth, %d, wOffsetX: %d, wOffsetY: %d)", pCursor, wHeight, wWidth, wOffsetX, wOffsetY);
#ifdef AMIGA
#endif // AMIGA
	logBlockEnd("mouseSetPointer");
}

void mouseResetPointer(void) {
#ifdef AMIGA
#endif // AMIGA
}

void mouseSetPosition(UWORD uwNewX, UWORD uwNewY) {
	mouseMoveBy(uwNewX - mouseGetX(), uwNewY - mouseGetY());
}

void mouseMoveBy(WORD wDx, WORD wDy) {
#ifdef AMIGA
#endif // AMIGA
}

void mouseClick(UBYTE ubMouseCode) {
#ifdef AMIGA
#endif // AMIGA
}
