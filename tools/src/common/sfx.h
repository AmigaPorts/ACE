/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_SFX_H_
#define _ACE_TOOLS_COMMON_SFX_H_

#include "wav.h"
#include <string>
#include <span>

class tSfx {
public:
	tSfx(void);

	tSfx(const tWav &Wav, bool isStrict);

	bool toSfx(const std::string &szPath, bool isCompress) const;

	bool isEmpty(void) const;

	std::uint32_t getLength(void) const;

	void normalize(void);

	void divideAmplitude(std::uint8_t ubDivisor);

	bool isFittingMaxAmplitude(std::int8_t bMaxAmplitude) const;

	bool hasEmptyFirstWord(void) const;

	void enforceEmptyFirstWord(void);

	void padContents(std::uint8_t ubAlignment);

	tSfx splitAfter(std::uint32_t ulSamples);

	static std::vector<uint8_t> compressLosslessDpcm(std::span<const int8_t> Uncompressed);
	static std::vector<int8_t> decompressLosslessDpcm(const std::vector<uint8_t> &vCompressed, std::uint32_t ulDecompressedSize);

private:
	std::uint32_t m_ulFreq;
	std::vector<int8_t> m_vData;
};

#endif // _ACE_TOOLS_COMMON_SFX_H_
