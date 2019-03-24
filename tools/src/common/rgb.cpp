/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "rgb.h"
#include <stdexcept>

tRgb::tRgb(const std::string &szCode)
{
	if(szCode[0] == '#') {
		if(szCode.length() == 3 + 1) {
			this->ubR = std::stoul(szCode.substr(1, 1), nullptr, 16);
			this->ubG = std::stoul(szCode.substr(2, 1), nullptr, 16);
			this->ubB = std::stoul(szCode.substr(3, 1), nullptr, 16);
		}
		else if(szCode.length() == 6 + 1) {
			this->ubR = std::stoul(szCode.substr(1, 2), nullptr, 16);
			this->ubG = std::stoul(szCode.substr(3, 2), nullptr, 16);
			this->ubB = std::stoul(szCode.substr(5, 2), nullptr, 16);
		}
		else {
			throw std::runtime_error("Can't convert color code");
		}
	}
}
