/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_TOOLS_PALETTE_CONVERTER_H_
#define _ACE_TOOLS_TOOLS_PALETTE_CONVERTER_H_

#include <vector>
#include <string>
#include "../common/rgb.h"

class tPalette {
public:

	static tPalette fromPlt(const std::string &szPath);

	static tPalette fromGpl(const std::string &szPath);

	static tPalette fromPromotionPal(const std::string &szPath);

	static void toPlt(tPalette vPalette, const std::string &szPath);

	std::vector<tRgb> m_vColors;

	tPalette(void) {}

	tPalette(const std::vector<tRgb> &vColors):
		m_vColors(vColors)
	{}

	int16_t getColorIdx(const tRgb &Ref) const;
};

#endif // _ACE_TOOLS_TOOLS_PALETTE_CONVERTER_H_
