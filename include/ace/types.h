/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TYPES_H_
#define _ACE_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ACE_DEBUG_ALL) && !defined(ACE_DEBUG)
#define ACE_DEBUG
#endif

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

// Potential collision with stdio
#if !defined(ULONG_MAX)
#define ULONG_MAX 0xFFFFFFFFu
#endif
#define UWORD_MAX 0xFFFFu
#define UBYTE_MAX 0xFFu

#if defined(__CODE_CHECKER__) || defined(__INTELLISENSE__)
// My realtime source checker has problems with GCC asm() expanded from REGARG()
// being in fn arg list, so I just use blank defines for it
#define INTERRUPT
#define INTERRUPT_END do {} while(0)
#define HWINTERRUPT
#define UNUSED_ARG __attribute__((unused))
#define REGARG(arg, reg) arg
#define CHIP
#define FAR
#define FN_HOTSPOT
#define FN_COLDSPOT
#define BITFIELD_STRUCT struct __attribute__((packed))
#elif defined(BARTMAN_GCC)
#define INTERRUPT
#define INTERRUPT_END do {} while(0)
#define HWINTERRUPT __attribute__((interrupt))
#define UNUSED_ARG __attribute__((unused))
#define REGARG(arg, reg) arg
#define CHIP __attribute__((section(".MEMF_CHIP")))
#define FAR
#define FN_HOTSPOT __attribute__((hot))
#define FN_COLDSPOT __attribute__((cold))
#define BITFIELD_STRUCT struct
#elif defined(__GNUC__) // Bebbo
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
#define BITFIELD_STRUCT struct
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
typedef union tUwCoordYX {
	ULONG ulYX;
	struct {
		UWORD uwY;
		UWORD uwX;
	};

#if defined(__cplusplus)
	[[nodiscard]] static constexpr auto zero() { return tUwCoordYX {.uwY = 0, .uwX = 0}; }
	[[nodiscard]] constexpr auto withX(UWORD uwX) { auto copy = *this; copy.uwX = uwX; return copy; }
	[[nodiscard]] constexpr auto withY(UWORD uwY) { auto copy = *this; copy.uwY = uwY; return copy; }
#endif
} tUwCoordYX;

typedef union tUbCoordYX {
	UWORD uwYX;
	struct {
		UBYTE ubY;
		UBYTE ubX;
	};


#if defined(__cplusplus)
	[[nodiscard]] static constexpr auto zero() { return tUbCoordYX {.ubY = 0, .ubX = 0}; }
	[[nodiscard]] constexpr auto withX(UBYTE ubX) { auto copy = *this; copy.ubX = ubX; return copy; }
	[[nodiscard]] constexpr auto withY(UBYTE ubY) { auto copy = *this; copy.ubY = ubY; return copy; }
#endif
} tUbCoordYX;

typedef struct tBCoordYX {
	BYTE bY;
	BYTE bX;

#if defined(__cplusplus)
	[[nodiscard]] static constexpr auto zero() { return tBCoordYX {.bY = 0, .bX = 0}; }
	[[nodiscard]] constexpr auto withX(BYTE bX) { auto copy = *this; copy.bX = bX; return copy; }
	[[nodiscard]] constexpr auto withY(BYTE bY) { auto copy = *this; copy.bY = bY; return copy; }
#endif
} tBCoordYX;

typedef struct tWCoordYX {
	WORD wY;
	WORD wX;

#if defined(__cplusplus)
	[[nodiscard]] static constexpr auto zero() { return tWCoordYX {.wY = 0, .wX = 0}; }
	[[nodiscard]] constexpr auto withX(WORD wX) { auto copy = *this; copy.wX = wX; return copy; }
	[[nodiscard]] constexpr auto withY(WORD wY) { auto copy = *this; copy.wY = wY; return copy; }
#endif
} tWCoordYX;

/**
 * Rectangle type
 */
typedef struct tUwRect {
	UWORD uwY;
	UWORD uwX;
	UWORD uwWidth;
	UWORD uwHeight;
} tUwRect;

typedef struct tUwAbsRect {
	UWORD uwY1;
	UWORD uwX1;
	UWORD uwY2;
	UWORD uwX2;
} tUwAbsRect;

#ifdef __cplusplus
}
#endif

#endif // _ACE_TYPES_H_
