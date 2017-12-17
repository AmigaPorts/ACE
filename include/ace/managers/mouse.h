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

typedef struct {
	UBYTE pButtonStates[3];
	UWORD uwX;
	UWORD uwY;
#ifdef AMIGA
	UWORD uwMinX;
	UWORD uwMinY;
	UWORD uwMaxX;
	UWORD uwMaxY;
	UWORD uwPrevPotGo;
	__chip UWORD pBlankCursor[6];
#endif // AMIGA
} tMouseManager;

/* Globals */
extern tMouseManager g_sMouseManager;

/* Functions */

void mouseCreate(void);

void mouseDestroy(void);

void mouseProcess(void);

void mouseSetBounds(
	IN UWORD uwX,
	IN UWORD uwY,
	IN UWORD uwWidth,
	IN UWORD uwHeight
);

static inline UWORD mouseGetX(void) {
	return g_sMouseManager.uwX;
}

static inline UWORD mouseGetY(void) {
	return g_sMouseManager.uwY;
}

static inline void mouseSetButton(UBYTE ubMouseCode, UBYTE ubMouseState) {
	g_sMouseManager.pButtonStates[ubMouseCode] = ubMouseState;
}

static inline UBYTE mouseCheck(UBYTE ubMouseCode) {
	return g_sMouseManager.pButtonStates[ubMouseCode] != MOUSE_NACTIVE;
}

static inline UBYTE mouseUse(UBYTE ubMouseCode) {
	if (g_sMouseManager.pButtonStates[ubMouseCode] == MOUSE_ACTIVE) {
		g_sMouseManager.pButtonStates[ubMouseCode] = MOUSE_USED;
		return 1;
	}
	return 0;
}

static inline UBYTE mouseInRect(UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight) {
	return (
		(uwX <= g_sMouseManager.uwX) && (g_sMouseManager.uwX < uwX + uwWidth) &&
		(uwY <= g_sMouseManager.uwY) && (g_sMouseManager.uwY < uwY + uwHeight)
	);
}

/**
 *  Sets mouse position to given absolute position.
 */
static inline void mouseSetPosition(UWORD uwNewX, UWORD uwNewY) {
	g_sMouseManager.uwX = CLAMP(
		uwNewX, g_sMouseManager.uwMinX, g_sMouseManager.uwMaxX
	);
	g_sMouseManager.uwY = CLAMP(
		uwNewY, g_sMouseManager.uwMinY, g_sMouseManager.uwMaxY
	);
}

/**
 * Moves mouse pointer from current position by relative offsets.
 */
static inline void mouseMoveBy(WORD wDx, WORD wDy) {
	g_sMouseManager.uwX = CLAMP(
		g_sMouseManager.uwX + wDx, g_sMouseManager.uwMinX, g_sMouseManager.uwMaxX
	);
	g_sMouseManager.uwY = CLAMP(
		g_sMouseManager.uwY + wDy, g_sMouseManager.uwMinY, g_sMouseManager.uwMaxY
	);
}

#endif
