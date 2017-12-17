#ifndef GUARD_ACE_MANAGER_MOUSE_H
#define GUARD_ACE_MANAGER_MOUSE_H

#include <ace/types.h>
#include <ace/macros.h>

/* Types */
#define MOUSE_LMB 0
#define MOUSE_RMB 1
#define MOUSE_MMB 2

#define MOUSE_NACTIVE 0
#define MOUSE_USED 1
#define MOUSE_ACTIVE 2

// #define MOUSE_PORT_0 0 // Unused mouse port for alignment
#define MOUSE_PORT_1 1
#define MOUSE_PORT_2 2

typedef struct _tMouse {
	UWORD uwX;
	UWORD uwY;
	UBYTE pButtonStates[3];
#ifdef AMIGA
	UBYTE ubPrevHwX;
	UBYTE ubPrevHwY;
#endif
} tMouse;

typedef struct _tMouseManager {
	UBYTE ubPortFlags;
	tMouse pMouses[3]; // zero is unused, faster than subtracting from port code
	UWORD uwLoX;
	UWORD uwLoY;
	UWORD uwHiX;
	UWORD uwHiY;
#ifdef AMIGA
	UWORD uwPrevPotGo;
#endif // AMIGA
} tMouseManager;

/* Globals */
extern tMouseManager g_sMouseManager;

/* Functions */

void mouseCreate(
	IN UBYTE ubPortFlags
);

void mouseDestroy(void);

void mouseProcess(void);

/**
 * Set on-screen constraints for cursor.
 * @param uwLoX Minimum cursor X position.
 * @param uwLoX Minimum cursor Y position.
 * @param uwHiY Maximum cursor X position.
 * @param uwHiY Maximum cursor Y position.
 */
void mouseSetBounds(
	IN UWORD uwLoX,
	IN UWORD uwLoY,
	IN UWORD uwHiX,
	IN UWORD uwHiY
);

static inline UWORD mouseGetX(
	IN UBYTE ubMousePort
) {
	return g_sMouseManager.pMouses[ubMousePort].uwX;
}

static inline UWORD mouseGetY(
	IN UBYTE ubMousePort
) {
	return g_sMouseManager.pMouses[ubMousePort].uwY;
}

static inline void mouseSetButton(
	IN UBYTE ubMousePort,
	IN UBYTE ubMouseCode,
	IN UBYTE ubMouseState
) {
	g_sMouseManager.pMouses[ubMousePort].pButtonStates[ubMouseCode] = ubMouseState;
}

static inline UBYTE mouseCheck(
	IN UBYTE ubMousePort,
	IN UBYTE ubMouseCode
) {
	UBYTE ubBtn = g_sMouseManager.pMouses[ubMousePort].pButtonStates[ubMouseCode];
	return ubBtn != MOUSE_NACTIVE;
}

static inline UBYTE mouseUse(
	IN UBYTE ubMousePort,
	IN UBYTE ubMouseCode
) {
	tMouse *pMouse = &g_sMouseManager.pMouses[ubMousePort];
	if(pMouse->pButtonStates[ubMouseCode] == MOUSE_ACTIVE) {
		pMouse->pButtonStates[ubMouseCode] = MOUSE_USED;
		return 1;
	}
	return 0;
}

static inline UBYTE mouseInRect(
	IN UBYTE ubMousePort,
	IN UWORD uwX,
	IN UWORD uwY,
	IN UWORD uwWidth,
	IN UWORD uwHeight
) {
	UWORD uwMouseX = g_sMouseManager.pMouses[ubMousePort].uwX;
	UWORD uwMouseY = g_sMouseManager.pMouses[ubMousePort].uwY;
	return (
		(uwX <= uwMouseX) && (uwMouseX < uwX + uwWidth) &&
		(uwY <= uwMouseY) && (uwMouseY < uwY + uwHeight)
	);
}

/**
 *  Sets mouse position to given absolute position.
 */
static inline void mouseSetPosition(
	IN UBYTE ubMousePort,
	IN UWORD uwNewX,
	IN UWORD uwNewY
) {
	g_sMouseManager.pMouses[ubMousePort].uwX = CLAMP(
		uwNewX, g_sMouseManager.uwLoX, g_sMouseManager.uwHiX
	);
	g_sMouseManager.pMouses[ubMousePort].uwY = CLAMP(
		uwNewY, g_sMouseManager.uwLoY, g_sMouseManager.uwHiY
	);
}

/**
 * Moves mouse pointer from current position by relative offsets.
 */
static inline void mouseMoveBy(
	IN UBYTE ubMousePort,
	IN WORD wDx,
	IN WORD wDy
) {
	g_sMouseManager.pMouses[ubMousePort].uwX = CLAMP(
		g_sMouseManager.pMouses[ubMousePort].uwX + wDx,
		g_sMouseManager.uwLoX, g_sMouseManager.uwHiX
	);
	g_sMouseManager.pMouses[ubMousePort].uwY = CLAMP(
		g_sMouseManager.pMouses[ubMousePort].uwY + wDy,
		g_sMouseManager.uwLoY, g_sMouseManager.uwHiY
	);
}

#endif
