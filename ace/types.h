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

/**
 * Coord type with fast sorting option
 */
typedef union {
	ULONG ulYX;
	struct {
		UWORD uwY;
		UWORD uwX;
	} sUwCoord;
} tUwCoordYX;

/**
 * Rectangle type
 */
typedef struct {
	UWORD uwY;
	UWORD uwX;
	UWORD uwWidth;
	UWORD uwHeight;
} tUwRect;

typedef struct {
	BYTE bY;
	BYTE bX;
} tBCoordYX;


#endif
