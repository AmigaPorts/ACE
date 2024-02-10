/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_ENDIAN_H_
#define _ACE_TOOLS_COMMON_ENDIAN_H_

#include <cstdint>
#include <bit>

namespace nEndian {
	constexpr bool isBig(void)
	{
		return std::endian::native == std::endian::big;
	}

	constexpr std::uint16_t toBig16(std::uint16_t uwIn)
	{
		if(isBig()) {
			return uwIn;
		}
		return (
			((uwIn & 0xFF'00) >> 8) |
			((uwIn & 0x00'FF) << 8)
		);
	}

	constexpr std::uint16_t fromBig16(std::uint16_t uwIn)
	{
		return toBig16(uwIn);
	}

	constexpr std::uint32_t toBig32(std::uint32_t ulIn)
	{
		if(isBig()) {
			return ulIn;
		}
		return (
			((ulIn & 0xFF'00'00'00) >> 24) |
			((ulIn & 0x00'FF'00'00) >>  8) |
			((ulIn & 0x00'00'FF'00) <<  8) |
			((ulIn & 0x00'00'00'FF) << 24)
		);
	}

	constexpr std::uint32_t fromBig32(std::uint32_t uwIn)
	{
		return toBig32(uwIn);
	}
}

#endif // _ACE_TOOLS_COMMON_ENDIAN_H_
