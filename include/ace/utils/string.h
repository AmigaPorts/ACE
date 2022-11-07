/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_STRING_H_
#define _ACE_UTILS_STRING_H_

// Contains highly optimized string functions.
// Those should be preferred over standard library in game loop.
// The functions returning pointer to null terminator at the end can be easily
// used in concatenation chains.

#include <ace/types.h>

/**
 * @brief Writes the unsigned value into destination string.
 * Relies on prior verification that buffer != NULL and bufsize != 0
 *
 * @param ulVal Value to be stringified.
 * @param pDst Destination string. Must be non-null and be at least of size
 * fitting the target value and null terminator.
 * @return Pointer to the null-terminator at the end of stringified value.
 */
char *stringDecimalFromULong(ULONG ulVal, char *pDst);

/**
 * @brief Copies string to new location, converting it to upper case.
 * The uppercasing works on basic 26-letter ASCII alphabet.
 * This does nothing for non-uppercase chars.
 *
 * @param szSrc Source string.
 * @param szDst Destination string.
 */
void strToUpper(const char *szSrc, char *szDst);

/**
 * @brief Convers single character to uppercase. Works only on basic
 * 26-letter ASCII alphabet.
 * This does nothing for non-uppercase chars.
 *
 * @param c Character to convert to uppercase.
 * @return Character converted to uppercase.
 */
char charToUpper(char c);

/**
 * @brief Checks if string is empty.
 * The check stops on first null-terminator.
 *
 * @param szStr String to be checked.
 * @return 1 if string consists only of null-temrinator, otherwise 0.
 */
UBYTE stringIsEmpty(const char *szStr);

/**
 * @brief Copies string from given source to destination buffer.
 *
 * @param szSrc Source string.
 * @param szDest Buffer for destination string.
 * @return Pointer to null-terminator character at the end of szDest.
 */
char *stringCopy(const char *szSrc, char *szDest);

/**
 * @brief Copies string from given source to destination buffer, with specified
 * length limit.
 *
 * The resulting string in destination buffer is null-terminated.
 *
 * @param szSrc Source string.
 * @param szDest Buffer for destination string.
 * @param uwMaxLength Maximum length of string to be written in szDest,
 * including the null terminator. Must be non-zero
 * @return Pointer to null-terminator character at the end of szDest.
 */
char *stringCopyLimited(const char *szSrc, char *szDest, UWORD uwMaxLength);

#endif // _ACE_UTILS_STRING_H_
