/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/assume.h>

#if defined(ACE_DEBUG)

void _assume(ULONG ulExprValue, const char *szErrorMsg, const char *szFile, ULONG ulLine) {
	if(!ulExprValue) {
		if(szErrorMsg) {
			logWrite("ERR: Assumption failed at %s:%lu - %s\n", szFile, ulLine, szErrorMsg);
		}
		else {
			logWrite("ERR: Assumption failed at %s:%lu\n", szFile, ulLine);
		}
#if defined(BARTMAN_GCC)
		__builtin_trap();
#else
		systemKill("Assumption failed - see logs for details");
#endif
	}
}

#endif
