/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/log.h>

#if defined(ACE_DEBUG)

#ifdef ACE_DEBUG_UAE

#if defined(BARTMAN_GCC)
long (*bartmanLog)(long mode, const char *string) = (long (*)(long, const char *))0xf0ff60;
void logConsole(const char *szMsg) {
	if (*((UWORD *)bartmanLog) == 0x4eb9 || *((UWORD *)bartmanLog) == 0xa00e) {
		bartmanLog(86, szMsg);
	}
}
#else
void logConsole(const char *szMsg) {
	volatile ULONG * const s_pUaeFmt = (ULONG *)0xBFFF04;
	*s_pUaeFmt = (ULONG)((UBYTE*)szMsg);
}
#endif

#else
#define logConsole(x)
#endif

#endif // defined(ACE_DEBUG)
