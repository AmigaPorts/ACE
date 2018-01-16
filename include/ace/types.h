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

#if  defined(__VBCC__)
#define INTERRUPT __amigainterrupt __saveds
#define REGARG(arg, reg) __reg(reg) arg
#define CHIP __chip
#define INTERRUPT_END do {} while(0)
#elif defined(__GNUC__)
#define INTERRUPT
#define REGARG(arg, reg) arg asm(reg)
#define CHIP __attribute__((chip))
#define INTERRUPT_END asm("cmp d0,d0")
#elif defined(__CODE_CHECKER__)
// My realtime source checker has problems with GCC asm() expanded from REGARG()
// being in fn arg list, so I just use blank defines for it
#define INTERRUPT
#define REGARG(arg, x) arg
#define CHIP
#define INTERRUPT_END do {} while(0)
#else
#error "Compiler not supported!"
#endif

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

#endif // GUARD_ACE_TYPES_H
