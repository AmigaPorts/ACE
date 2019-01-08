/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_RGB_H_
#define _ACE_TOOLS_COMMON_RGB_H_

#include <stdint.h>

struct tRgb {
	uint8_t ubR, ubG, ubB;

	tRgb(uint8_t ubNewR, uint8_t ubNewG, uint8_t ubNewB):
		ubR(ubNewR), ubG(ubNewG), ubB(ubNewB) { }

	tRgb(uint8_t ubRgb):
		ubR(ubRgb), ubG(ubRgb), ubB(ubRgb) { }

	bool operator == (const tRgb &Rhs) const {
		return ubB == Rhs.ubB && ubG == Rhs.ubG && ubR == Rhs.ubR;
	}

	bool operator != (const tRgb &Rhs) const {
		return !(*this == Rhs);
	}
};

#endif // _ACE_TOOLS_COMMON_RGB_H_
