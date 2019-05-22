/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_MOUSE_H_
#define _ACE_MANAGERS_MOUSE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>
#include <ace/macros.h>

// Mouse button identifiers
#define MOUSE_LMB 0
#define MOUSE_RMB 1
#define MOUSE_MMB 2

// Button state defines
#define MOUSE_NACTIVE 0
#define MOUSE_USED 1
#define MOUSE_ACTIVE 2

// Mouse ports
// #define MOUSE_PORT_0 0 // Unused mouse port for alignment
#define MOUSE_PORT_1 1
#define MOUSE_PORT_2 2

/* Types */
typedef struct _tMouse {
	UWORD uwX;
	UWORD uwY;
	UBYTE pButtonStates[3];
	tUwAbsRect sBounds; ///< Min/max mouse position.
#ifdef AMIGA
	UBYTE ubPrevHwX;
	UBYTE ubPrevHwY;
#endif
} tMouse;

typedef struct _tMouseManager {
	UBYTE ubPortFlags;
	tMouse pMice[3]; ///< Zero is pad, faster than subtracting from port code.
#ifdef AMIGA
	UWORD uwPrevPotGo; ///< Previous control port config.
#endif // AMIGA
} tMouseManager;

/* Globals */
extern tMouseManager g_sMouseManager;

/* Functions */

/**
 * Initializes mouse management for given ports.
 * @param ubPortFlags: Ports in which mouse should be processed.
 *        OR combination of MOUSE_PORT_* defines.
 * @see mouseDestroy()
 * @see mouseProcess()
 */
void mouseCreate(UBYTE ubPortFlags);

/**
 * Cleans up after mouse maanger.
 * @see mouseCreate()
 */
void mouseDestroy(void);

/**
 * Processes mouse manager, updating mice's position and button states.
 * Should be called once per frame.
 */
void mouseProcess(void);

/**
 * Set on-screen constraints for cursor.
 * @param ubMousePort: Mouse port to be constrained.
 * @param uwLoX: Minimum cursor X position.
 * @param uwLoX: Minimum cursor Y position.
 * @param uwHiY: Maximum cursor X position.
 * @param uwHiY: Maximum cursor Y position.
 */
static inline void mouseSetBounds(
	UBYTE ubMousePort, UWORD uwLoX, UWORD uwLoY, UWORD uwHiX, UWORD uwHiY
) {
	g_sMouseManager.pMice[ubMousePort].sBounds.uwX1 = uwLoX;
	g_sMouseManager.pMice[ubMousePort].sBounds.uwY1 = uwLoY;
	g_sMouseManager.pMice[ubMousePort].sBounds.uwX2 = uwHiX;
	g_sMouseManager.pMice[ubMousePort].sBounds.uwY2 = uwHiY;
}


/**
 * Returns given mouse's current X position.
 * @param ubMousePort: Mouse to be polled. Use one of MOUSE_PORT_* values.
 * @return Mouse's current X position relative to top-left screen pos.
 */
static inline UWORD mouseGetX(UBYTE ubMousePort) {
	return g_sMouseManager.pMice[ubMousePort].uwX;
}

/**
 * Returns given mouse's current Y position.
 * @param ubMousePort: Mouse to be polled. Use one of MOUSE_PORT_* values.
 * @return Mouse's current X position relative to top-left screen pos.
 */
static inline UWORD mouseGetY(UBYTE ubMousePort) {
	return g_sMouseManager.pMice[ubMousePort].uwY;
}

/**
 * Sets given mouse's button to desired state.
 * @param ubMousePort: Mouse to be set.
 * @param ubMouseCode: Mouse button, which state should be changed
 *        (MOUSE_LMB, MOUSE_RMB or MOUSE_MMB).
 * @param ubMouseState: New button state
 *        (MOUSE_NACTIVE, MOUSE_USED, MOUSE_ACTIVE).
 */
static inline void mouseSetButton(
	UBYTE ubMousePort, UBYTE ubMouseCode, UBYTE ubMouseState
) {
	g_sMouseManager.pMice[ubMousePort].pButtonStates[ubMouseCode] = ubMouseState;
}

/**
 * Returns given mouse's button state.
 * @param ubMousePort: Mouse to be polled.
 * @param ubMouseCode: Button to be polled (MOUSE_LMB, MOUSE_RMB or MOUSE_MMB).
 * @return 1 if button is pressed, otherwise 0.
 */
static inline UBYTE mouseCheck(UBYTE ubMousePort, UBYTE ubMouseCode) {
	UBYTE ubBtn = g_sMouseManager.pMice[ubMousePort].pButtonStates[ubMouseCode];
	return ubBtn != MOUSE_NACTIVE;
}

/**
 * Returns whether given button was recently pressed.
 * If button was polled as ACTIVE, function returns 1 and sets button as USED.
 * @param ubMousePort: Mouse to be polled.
 * @param ubMouseCode: Button to be polled (MOUSE_LMB, MOUSE_RMB or MOUSE_MMB).
 * @return 1 if button was recently pressed, otherwise 0.
 */
static inline UBYTE mouseUse(UBYTE ubMousePort, UBYTE ubMouseCode) {
	tMouse *pMouse = &g_sMouseManager.pMice[ubMousePort];
	if(pMouse->pButtonStates[ubMouseCode] == MOUSE_ACTIVE) {
		pMouse->pButtonStates[ubMouseCode] = MOUSE_USED;
		return 1;
	}
	return 0;
}

/**
 * Checks if given mouse has position contained within given rectangle.
 * @param ubMousePort: Mouse to be polled.
 * @param sRect: Rectangle to be checked.
 * @return 1 if mouse position is within given rectangle, otherwise 0.
 */
static inline UBYTE mouseInRect(UBYTE ubMousePort, tUwRect sRect) {
	UWORD uwMouseX = g_sMouseManager.pMice[ubMousePort].uwX;
	UWORD uwMouseY = g_sMouseManager.pMice[ubMousePort].uwY;
	return (
		(sRect.uwX <= uwMouseX) && (uwMouseX < sRect.uwX + sRect.uwWidth) &&
		(sRect.uwY <= uwMouseY) && (uwMouseY < sRect.uwY + sRect.uwHeight)
	);
}

/**
 *  Sets mouse position to given absolute position.
 * This function takes into account bounds specified by mouseSetBounds().
 * @param ubMousePort: Mouse of which position should be changed.
 * @param uwNewX: new X position.
 * @param uwNewY: new Y position.
 * @see mouseSetBounds()
 * @see mouseMoveBy()
 */
static inline void mouseSetPosition(
	UBYTE ubMousePort, UWORD uwNewX, UWORD uwNewY
) {
	tMouse *pMouse = &g_sMouseManager.pMice[ubMousePort];
	pMouse->uwX = CLAMP(uwNewX, pMouse->sBounds.uwX1, pMouse->sBounds.uwX2);
	pMouse->uwY = CLAMP(uwNewY, pMouse->sBounds.uwY1, pMouse->sBounds.uwY2);
}

/**
 * Moves mouse pointer from current position by relative offsets.
 * This function takes into account bounds specified by mouseSetBounds().
 * @param ubMousePort: Mouse of which position should be changed.
 * @param wDx: Positive value moves mouse right, negative moves left.
 * @param wDy: Positive value moves mouse down, negative moves left.
 * @see mouseSetBounds()
 * @see mouseSetPosition()
 */
static inline void mouseMoveBy(UBYTE ubMousePort, WORD wDx, WORD wDy) {
	tMouse *pMouse = &g_sMouseManager.pMice[ubMousePort];
	pMouse->uwX = CLAMP(
		pMouse->uwX + wDx, pMouse->sBounds.uwX1, pMouse->sBounds.uwX2
	);
	pMouse->uwY = CLAMP(
		pMouse->uwY + wDy, pMouse->sBounds.uwY1, pMouse->sBounds.uwY2
	);
}

/**
 * Resets mouse position to center of legal coordinate range.
 * @param ubMousePort: Mouse of which position should be reset.
 */
static inline void mouseResetPos(UBYTE ubMousePort) {
	const tUwAbsRect *pBounds = &g_sMouseManager.pMice[ubMousePort].sBounds;
	g_sMouseManager.pMice[ubMousePort].uwX = (pBounds->uwX2 - pBounds->uwX1) >> 1;
	g_sMouseManager.pMice[ubMousePort].uwY = (pBounds->uwY2 - pBounds->uwY1) >> 1;
}

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_MOUSE_H_
