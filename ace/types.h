#ifndef GUARD_ACE_TYPES_H
#define GUARD_ACE_TYPES_H

#include <exec/types.h>

/* Docs */
#define IN    /* Input parameter. Passed pointer contents is const */
#define OUT   /* Output parameter. Passed pointer contants will be changed. */
#define INOUT /* Input/output parameter. */

#ifndef __VBCC__
#define __amigainterrupt /* Amiga interrupt handler */
#define __saveds         /**/
#define __reg(x)         /* Allows putting fn parameters in specific regs */
#define __chip           /* Variable in CHIP memory region */
#define __fast           /* Variable in FAST memory region */
#endif // __VBCC__

// Fast types
// TODO: AGA: perhaps 32-bit?
typedef UWORD FUBYTE;
typedef UWORD FUWORD;
typedef ULONG FULONG;
typedef WORD FBYTE;
typedef WORD FWORD;
typedef LONG FLONG;

/**
 * Coord type with fast sorting option
 */
typedef union _tUwCoordYX {
	ULONG ulYX;
	struct {
		UWORD uwY;
		UWORD uwX;
	} sUwCoord;
} tUwCoordYX;

typedef union _tUbCoordYX {
	UWORD uwYX;
	struct {
		UBYTE ubY;
		UBYTE ubX;
	} sUbCoord;
} tUbCoordYX;

typedef struct _tBCoordYX {
	BYTE bY;
	BYTE bX;
} tBCoordYX;

/**
 * Rectangle type
 */
typedef struct {
	UWORD uwY;
	UWORD uwX;
	UWORD uwWidth;
	UWORD uwHeight;
} tUwRect;

#endif
