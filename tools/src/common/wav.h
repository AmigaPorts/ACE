/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_WAV_H_
#define _ACE_TOOLS_COMMON_WAV_H_

#include <string>
#include <vector>
#include <cstdint>

class tWav {
public:
	tWav(const std::string &szPath);

	const std::vector<uint8_t> &getData(void) const;
	std::uint32_t getSampleRate(void) const;
	std::uint8_t getBitsPerSample(void) const;

private:
	struct tSubchunk {
		std::string m_szId = std::string(4, '\0');
		std::uint32_t m_ulSize;
		std::string m_szContents;
	};

	const tSubchunk *findSubchunk(const std::string &szId) const;

	std::vector<tSubchunk> m_vSubchunks;
	std::vector<uint8_t> m_vData;
	std::uint32_t m_ulSampleRate;
	std::uint8_t m_ubBitsPerSample;
};

#endif // _ACE_TOOLS_COMMON_WAV_H_
