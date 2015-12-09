#ifndef GUARD_ACE_MANAGER_MOUSE_H
#define GUARD_ACE_MANAGER_MOUSE_H

#include <clib/exec_protos.h> // Amiga typedefs
#include <clib/intuition_protos.h> // IDCMP_RAWKEY etc
#include <devices/input.h>
#include <clib/alib_protos.h>

#include "config.h"

#include "managers/window.h"

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

inline UWORD mouseGetX(void);

inline UWORD mouseGetY(void);

inline void mouseSetPointer(
	IN UWORD *pCursor,
	IN WORD wHeight,
	IN WORD wWidth,
	IN WORD wOffsetX,
	IN WORD wOffsetY
);

inline void mouseResetPointer(void);

void mouseMove(
	WORD wX,
	WORD wY
);

void mouseClick(
	UBYTE ubMouseCode
);

void mouseClose(void);

#endif