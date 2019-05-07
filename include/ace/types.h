/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TYPES_H_
#define _ACE_TYPES_H_

// Full OS takeover
#define CONFIG_SYSTEM_OS_TAKEOVER
// OS-friendly (old) mode
// #define CONFIG_SYSTEM_OS_FRIENDLY // TODO: implement

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

#if defined(__CODE_CHECKER__)
// My realtime source checker has problems with GCC asm() expanded from REGARG()
// being in fn arg list, so I just use blank defines for it
#define INTERRUPT
#define INTERRUPT_END do {} while(0)
#define HWINTERRUPT
#define UNUSED_ARG __attribute__((unused))
#define REGARG(arg, x) arg
#define CHIP
#define FAR
#define FN_HOTSPOT
#define FN_COLDSPOT
#elif defined(__VBCC__)
#if defined(CONFIG_SYSTEM_OS_FRIENDLY)
#define INTERRUPT __amigainterrupt __saveds
#define INTERRUPT_END do {} while(0)
#elif defined(CONFIG_SYSTEM_OS_TAKEOVER)
#define INTERRUPT
#define INTERRUPT_END do {} while(0)
#endif

#define HWINTERRUPT __interrupt __saveds
#define UNUSED_ARG
#define REGARG(arg, reg) __reg(reg) arg
#define CHIP __chip
#define FAR
#define INTERRUPT_END do {} while(0)
#define FN_HOTSPOT
#define FN_COLDSPOT
#elif defined(__GNUC__)
#if defined(CONFIG_SYSTEM_OS_FRIENDLY)
// Interrupt macros for OS interrupts (handlers)
#define INTERRUPT
#define INTERRUPT_END asm("cmp d0,d0")
#elif defined(CONFIG_SYSTEM_OS_TAKEOVER)
// Interrupt macros for ACE interrupts
#define INTERRUPT
#define INTERRUPT_END do {} while(0)
#endif

#define HWINTERRUPT __attribute__((interrupt))
#define UNUSED_ARG __attribute__((unused))
#define REGARG(arg, reg) arg asm(reg)
#define CHIP __attribute__((chip))
#define FAR __far
#define FN_HOTSPOT __attribute__((hot))
#define FN_COLDSPOT __attribute__((cold))
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

#endif // _ACE_TYPES_H_
