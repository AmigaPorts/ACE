/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_PARSE_H_
#define _ACE_TOOLS_COMMON_PARSE_H_

#include <string>
#include "logging.h"

namespace nParse {

static inline bool toInt32(
	const std::string &szVal, const std::string &szValName, int32_t &lOut
)
{
	try {
		lOut = std::stol(szVal);
	}
	catch(std::exception ex) {
		nLog::error("Couldn't parse {}: '{}'\n", szValName, szVal);
		return false;
	}
	return true;
}

} // namespace nParse


#endif // _ACE_TOOLS_COMMON_PARSE_H_
