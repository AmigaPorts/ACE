/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_SFX_H_
#define _ACE_TOOLS_COMMON_SFX_H_

#include "wav.h"
#include <string>

class tSfx {
public:
	constexpr tSfx(void);

	tSfx(const tWav &Wav, bool isStrict);

	bool toSfx(const std::string &szPath) const;

	bool isEmpty(void) const;

	uint32_t getLength(void) const;

	void normalize(void);

	void divideAmplitude(uint8_t ubDivisor);

	bool isFittingMaxAmplitude(int8_t bMaxAmplitude) const;

	bool hasEmptyFirstWord(void) const;

	void enforceEmptyFirstWord(void);

	void padContents(uint8_t ubAlignment);

	tSfx splitAfter(uint32_t ulSamples);

private:
	uint32_t m_ulFreq;
	std::vector<int8_t> m_vData;
};

//------------------------------------------------------------------------- IMPL

constexpr tSfx::tSfx(void):
	m_ulFreq(0)
{
}

#endif // _ACE_TOOLS_COMMON_SFX_H_
