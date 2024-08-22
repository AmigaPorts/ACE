/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_RGB_H_
#define _ACE_TOOLS_COMMON_RGB_H_

#include <cstdint>
#include <string>

struct tRgb {
	std::uint8_t ubR, ubG, ubB;

	tRgb(std::uint8_t ubNewR, std::uint8_t ubNewG, std::uint8_t ubNewB):
		ubR(ubNewR), ubG(ubNewG), ubB(ubNewB) { }

	tRgb(std::uint8_t ubGrayscale):
		ubR(ubGrayscale), ubG(ubGrayscale), ubB(ubGrayscale) { }

	tRgb():
		ubR(0), ubG(0), ubB(0) { }

	tRgb(const std::string &szCode);

	std::string toString(void) const;

	tRgb to12Bit(void) const;

	tRgb toEhb(void) const;

	bool operator == (const tRgb &Rhs) const {
		return ubB == Rhs.ubB && ubG == Rhs.ubG && ubR == Rhs.ubR;
	}

	bool operator != (const tRgb &Rhs) const {
		return !(*this == Rhs);
	}
};

#endif // _ACE_TOOLS_COMMON_RGB_H_
