#ifndef GUARD_ACE_UTIL_ENDIAN_H
#define GUARD_ACE_UTIL_ENDIAN_H

/**
 *  Endian conversion functions.
 *  Allows convenient converting between Little and Big Endian.
 *  @todo Distinguish platform's endian and avoid conversion if endian
 *        is matching.
 */

#include <ace/types.h>

// TODO:

/**
 *  @brief Converts _native_ 16-bit to Little (Intel) Endian.
 *
 *  @param uwIn 16-bit value to be converted
 *  @return Supplied value, byte-swapped if neccessary.
 *
 *  @see endianIntel32()
 */
static inline UWORD endianIntel16(UWORD uwIn) {
#ifdef AMIGA
	return (uwIn << 8) | (uwIn >> 8);
#else
	return uwIn;
#endif // AMIGA
}

/**
 *  @brief Converts _native_ 32-bit to Little (Intel) Endian.
 *
 *  @param ulIn 32-bit value to be converted
 *  @return Supplied value, byte-swapped if neccessary.
 *
 *  @see endianIntel16()
 */
static inline ULONG endianIntel32(ULONG ulIn) {
#ifdef AMIGA
	return (ulIn << 24) | ((ulIn&0xFF00) << 8) | ((ulIn & 0xFF0000) >> 8) | (ulIn >> 24);
#else
	return ulIn;
#endif // AMIGA
}

#endif
