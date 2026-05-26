/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_MISC_RESOURCE_H_
#define _ACE_MANAGERS_MISC_RESOURCE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <ace/types.h>

// Types
typedef enum tMiscSubresource {
	MISC_SUBRESOURCE_PARALLEL,
	MISC_SUBRESOURCE_SERIAL,
	MISC_SUBRESOURCE_COUNT
} tMiscSubresource;

// Globals

// Functions

UBYTE miscResourceTryUse(tMiscSubresource eSubresource);

void miscResourceUnuse(tMiscSubresource eSubresource);

UBYTE miscResourceIsUsed(tMiscSubresource eSubresource);

#ifdef __cplusplus
}
#endif


#endif // _ACE_MANAGERS_MISC_RESOURCE_H_
