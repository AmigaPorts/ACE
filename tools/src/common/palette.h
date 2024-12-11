/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_PALETTE_H_
#define _ACE_TOOLS_COMMON_PALETTE_H_

#include <vector>
#include <string>
#include "../common/rgb.h"

class tPalette {
public:

	static tPalette fromFile(const std::string &szPath);

	static tPalette fromPlt(const std::string &szPath);

	static tPalette fromGpl(const std::string &szPath);

	static tPalette fromPromotionPal(const std::string &szPath);

	static tPalette fromAct(const std::string &szPath);

	bool toPlt(const std::string &szPath, bool isForceOcs);

	bool toGpl(const std::string &szPath);

	bool toPromotionPal(const std::string &szPath);

	bool toAct(const std::string &szPath);

	bool isValid(void) const;

	std::uint8_t getBpp(void) const;

	bool convertToEhb(void);

	std::vector<tRgb> m_vColors;

	tPalette(void) {}

	tPalette(const std::vector<tRgb> &vColors):
		m_vColors(vColors)
	{}

	std::int16_t getColorIdx(const tRgb &Ref) const;
};

#endif // _ACE_TOOLS_COMMON_PALETTE_H_
