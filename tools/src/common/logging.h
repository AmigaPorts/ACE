/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_LOGGING_H_
#define _ACE_TOOLS_COMMON_LOGGING_H_

#include <fmt/format.h>

namespace nLog {

template<typename... t_tArgs>
void error(const std::string &szFmt, const t_tArgs &... Args) {
	fmt::print("ERR: ");
	fmt::print(szFmt, Args...);
	fmt::print("\n");
}

} // namespace nLog

#endif // _ACE_TOOLS_COMMON_LOGGING_H_
