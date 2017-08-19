#ifndef GUARD_ACE_TYPES_H
#define GUARD_ACE_TYPES_H

#include <exec/types.h>

/* Docs */
#define IN
#define OUT
#define INOUT

// TODO: Convert to ifdef for elder compilers
#define inline

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
