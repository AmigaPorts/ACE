/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "rgb.h"
#include <fmt/format.h>
#include <stdexcept>
#include "parse.h"

tRgb::tRgb(const std::string &szCode)
{
	if(szCode[0] == '#') {
		if(szCode.length() == 3 + 1) {
			nParse::hexToRange<std::uint8_t>(szCode.substr(1, 1), 0, 15, this->ubR);
			nParse::hexToRange<std::uint8_t>(szCode.substr(2, 1), 0, 15, this->ubG);
			nParse::hexToRange<std::uint8_t>(szCode.substr(3, 1), 0, 15, this->ubB);
			this->ubR *= 17;
			this->ubG *= 17;
			this->ubB *= 17;
		}
		else if(szCode.length() == 6 + 1) {
			nParse::hexToRange<std::uint8_t>(szCode.substr(1, 2), 0, 255, this->ubR);
			nParse::hexToRange<std::uint8_t>(szCode.substr(3,2), 0, 255, this->ubG);
			nParse::hexToRange<std::uint8_t>(szCode.substr(5, 2), 0, 255, this->ubB);
		}
		else {
			throw std::runtime_error(fmt::format("Unknown RGB hex format: {}", szCode));
		}
	}
	else {
		throw std::runtime_error(fmt::format("Unknown RGB color format: {}", szCode));
	}
}

tRgb tRgb::to12Bit(void) const
{
	auto R4 = ((this->ubR + 16) / 17);
	auto G4 = ((this->ubG + 16) / 17);
	auto B4 = ((this->ubB + 16) / 17);
	auto Out = tRgb((R4 << 4) | R4, (G4 << 4) | G4, (B4 << 4) | B4);
	return Out;
}

tRgb tRgb::toEhb(void) const
{
	auto R4 = ((this->ubR + 16) / 17) >> 1;
	auto G4 = ((this->ubG + 16) / 17) >> 1;
	auto B4 = ((this->ubB + 16) / 17) >> 1;
	auto Out = tRgb((R4 << 4) | R4, (G4 << 4) | G4, (B4 << 4) | B4);
	return Out;
}

std::string tRgb::toString(void) const
{
	return fmt::format("#{:02X}{:02X}{:02X}", ubR, ubG, ubB);
}
