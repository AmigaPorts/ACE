/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/log.h>
#include <SDL_log.h>

#if defined(ACE_DEBUG)

void logConsole(const char *szMsg) {
	printf(szMsg);
}

#endif // defined(ACE_DEBUG)
