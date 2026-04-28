/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_ENDIAN_H_
#define _ACE_UTILS_ENDIAN_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Endian conversion functions.
 *  Allows convenient converting between Little and Big Endian.
 *  @todo Distinguish platform's endian and avoid conversion if endian
 *        is matching.
 */

#include <ace/types.h>

/**
 *  @brief Converts _native_ 16-bit from/to Little (Intel) Endian.
 *
 *  @param uwIn 16-bit value to be converted
 *  @return Supplied value, byte-swapped if neccessary.
 *
 *  @see endianLittle32()
 */
static inline UWORD endianLittle16(UWORD uwIn) {
#ifdef AMIGA
	return (uwIn << 8) | (uwIn >> 8);
#else
	return uwIn;
#endif // AMIGA
}

/**
 *  @brief Converts big-endian wire format to/from native 16-bit.
 *
 *  Use after reading a big-endian `UWORD` from a file, or before writing one.
 *  On Amiga (big-endian native) this is a no-op.
 *
 *  @param uwIn 16-bit value in wire (big-endian) layout as stored by `fileRead()`.
 *  @return Value in native endian.
 *
 *  @see endianLittle16()
 */
static inline UWORD endianBig16(UWORD uwIn) {
#ifdef AMIGA
	return uwIn;
#else
	return (uwIn << 8) | (uwIn >> 8);
#endif // AMIGA
}

/**
 *  @brief Converts _native_ 32-bit from/to Little (Intel) Endian.
 *
 *  @param ulIn 32-bit value to be converted
 *  @return Supplied value, byte-swapped if neccessary.
 *
 *  @see endianLittle16()
 */
static inline ULONG endianLittle32(ULONG ulIn) {
#ifdef AMIGA
	return (ulIn << 24) | ((ulIn&0xFF00) << 8) | ((ulIn & 0xFF0000) >> 8) | (ulIn >> 24);
#else
	return ulIn;
#endif // AMIGA
}

/**
 *  @brief Converts big-endian wire format to/from native 32-bit.
 *
 *  Use after reading a big-endian `ULONG` from a file, or before writing one.
 *  On Amiga (big-endian native) this is a no-op.
 *
 *  @param ulIn 32-bit value in wire (big-endian) layout as stored by `fileRead()`.
 *  @return Value in native endian.
 *
 *  @see endianLittle32()
 */
static inline ULONG endianBig32(ULONG ulIn) {
#ifdef AMIGA
	return ulIn;
#else
	return (ulIn << 24) | ((ulIn & 0xFF00) << 8) | ((ulIn & 0xFF0000) >> 8) | (ulIn >> 24);
#endif // AMIGA
}

#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_ENDIAN_H_
