/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_MATH_H_
#define _ACE_TOOLS_COMMON_MATH_H_

#if defined(_MSC_VER) && _MSC_VER < 1930
auto ceilToFactor(std::uint64_t Value, std::int64_t Multiple) {
#else
auto ceilToFactor(auto Value, auto Multiple) {
#endif
	auto Rounded = ((Value + Multiple - 1) / Multiple) * Multiple;
	return Rounded;
}

#endif // _ACE_TOOLS_COMMON_MATH_H_
