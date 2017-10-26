#ifndef GUARD_ACE_TYPES_H
#define GUARD_ACE_TYPES_H

#ifdef AMIGA
#include <exec/types.h>
#else
#include <stdint.h>

typedef uint8_t  UBYTE;
typedef uint16_t UWORD;
typedef uint32_t ULONG;

typedef int8_t  BYTE;
typedef int16_t WORD;
typedef int32_t LONG;
#endif // AMIGA

#define GAME_DEBUG

/* Docs */
#define IN    /* Input parameter. Passed pointer contents is const */
#define OUT   /* Output parameter. Passed pointer contents will be changed. */
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

#define PRI_FUBYTE "hu"
#define PRI_FUWORD "hu"
#define PRI_FULONG "u"
#define PRI_FBYTE  "hd"
#define PRI_FWORD  "hd"
#define PRI_FLONG  "d"

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
typedef struct _tUwRect {
	UWORD uwY;
	UWORD uwX;
	UWORD uwWidth;
	UWORD uwHeight;
} tUwRect;

typedef struct _tUwAbsRect {
	UWORD uwY1;
	UWORD uwX1;
	UWORD uwY2;
	UWORD uwX2;
} tUwAbsRect;

#endif