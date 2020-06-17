/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_GAME_H_
#define _ACE_MANAGERS_GAME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h> // Amiga typedefs

/* Functions */

void gameExit(void);

UBYTE gameIsRunning(void);

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_GAME_H_
