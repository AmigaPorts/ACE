/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_STRING_H_
#define _ACE_UTILS_STRING_H_

// Contains highly optimized string functions.
// Those should be preferred over standard library in game loop.

#include <ace/types.h>

char *stringDecimalFromULong(ULONG ulVal, char *pDst);

void strToUpper(const char *szSrc, char *szDst);

#endif // _ACE_UTILS_STRING_H_
