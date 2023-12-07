/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_ASSUME_H_
#define _ACE_UTILS_ASSUME_H_

#include <ace/types.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ACE_DEBUG)

#define assume(cond) _assume(cond, 0, __FILE__, __LINE__)
#define assumeMsg(cond, szErrorMsg) _assume(cond, szErrorMsg, __FILE__, __LINE__)
#define assumeNotNull(ptr) assumeMsg(ptr != 0, "Null pointer: " #ptr);

void _assume(ULONG ulExprValue, const char *szErrorMsg, const char *szFile, ULONG ulLine);

#else

#if __GNUC__ >= 13
#define assume(cond) __attribute__((__assume__(cond)))
#else
// https://stackoverflow.com/questions/25667901/ - this isn't optimized away!
// #define assume(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
// https://stackoverflow.com/questions/30919802/ - no footprint on code
#define assume(cond) ((void) sizeof(cond))
#endif
#define assumeMsg(cond, szErrorMsg) assume(cond)
#define assumeNotNull(ptr) assume(ptr != 0)

#endif

#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_ASSUME_H_
