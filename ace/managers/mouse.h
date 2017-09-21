#ifndef GUARD_ACE_MANAGER_MOUSE_H
#define GUARD_ACE_MANAGER_MOUSE_H

#include <clib/exec_protos.h> // Amiga typedefs
#include <clib/intuition_protos.h> // IDCMP_RAWKEY etc
#include <devices/input.h>
#include <clib/alib_protos.h>

#include <ace/config.h>

#include <ace/managers/window.h>

/* Types */
#define MOUSE_LMB IECODE_LBUTTON
#define MOUSE_RMB IECODE_RBUTTON
#define MOUSE_MMB IECODE_MBUTTON

#define MOUSE_NACTIVE 0
#define MOUSE_USED 1
#define MOUSE_ACTIVE 2

typedef struct {
	UBYTE pStates[3];
	__chip UWORD pBlankCursor[6];
	struct MsgPort *pInputMP;
	struct IOStdReq *pInputIO;
} tMouseManager;

/* Globals */
extern tMouseManager g_sMouseManager;

/* Functions */
void mouseOpen(void);

void mouseSetState(
	IN UBYTE ubMouseCode,
	IN UBYTE ubMouseState
);

UBYTE mouseCheck(
	IN UBYTE ubMouseCode
);

UBYTE mouseUse(
	IN UBYTE ubMouseCode
);

UBYTE mouseIsIntersects(
	IN UWORD uwX,
	IN UWORD uwY,
	IN UWORD uwWidth,
	IN UWORD uwHeight
);

UWORD mouseGetX(void);

UWORD mouseGetY(void);

void mouseSetPointer(
	IN UWORD *pCursor,
	IN WORD wHeight,
	IN WORD wWidth,
	IN WORD wOffsetX,
	IN WORD wOffsetY
);

void mouseResetPointer(void);

/**
 *  Sets mouse position to given absolute position.
 */
void mouseSetPosition(
	IN UWORD uwNewX,
	IN UWORD uwNewY
);

/**
 * Moves mouse pointer from current position by relative offsets.
 */
void mouseMove(
	IN WORD wX,
	IN WORD wY
);

void mouseClick(
	IN UBYTE ubMouseCode
);

void mouseClose(void);

#endif
