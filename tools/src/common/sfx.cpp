/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "sfx.h"
#include <fstream>
#include <algorithm>
#include "logging.h"
#include "endian.h"

tSfx::tSfx(void):
	m_ulFreq(0)
{
}

tSfx::tSfx(const tWav &Wav, bool isStrict):
	tSfx()
{
	auto vWavData = Wav.getData();
	m_ulFreq = Wav.getSampleRate();
	auto BitsPerSample = Wav.getBitsPerSample();

	if(BitsPerSample == 8) {
		// Convert 8-bit unsigned to signed - https://wiki.multimedia.cx/index.php/PCM#Sign
		for(const auto &UnsignedSample: vWavData) {
			std::int8_t bSample = UnsignedSample - 128;
			m_vData.push_back(bSample);
		}
	}
	else if(BitsPerSample == 16) {
		nLog::warn("Got 16bps, expected 8bps. Resample your .wav in proper audio program!");
		if(isStrict) {
			nLog::error("Strict mode - aborting...");
			return;
		}
		nLog::warn("Resampling data - may result in poor results!");
		auto DataSize = vWavData.size();
		for(std::uint32_t i = 0; i < DataSize; i += 2) {
			std::uint16_t uwRawSample = vWavData[i + 0] | (vWavData[i + 1] << 8);
			auto *pAsSigned = reinterpret_cast<std::int16_t *>(&uwRawSample);
			m_vData.push_back(*pAsSigned / 256);
		}
	}

	// Needs even number of bytes - Amiga reads it as words
	if(m_vData.size() & 1) {
		m_vData.push_back(0);
	}
}

bool tSfx::toSfx(const std::string &szPath) const {
	std::ofstream FileOut(szPath, std::ios::binary);

	const std::uint8_t ubVersion = 1;
	const std::uint16_t uwWordLength = nEndian::toBig16(uint16_t(m_vData.size() / 2));
	const std::uint16_t uwSampleReateHz = nEndian::toBig16(m_ulFreq);

	FileOut.write(reinterpret_cast<const char*>(&ubVersion), sizeof(ubVersion));
	FileOut.write(reinterpret_cast<const char*>(&uwWordLength), sizeof(uwWordLength));
	FileOut.write(reinterpret_cast<const char*>(&uwSampleReateHz), sizeof(uwSampleReateHz));
	FileOut.write(reinterpret_cast<const char*>(m_vData.data()), m_vData.size());

	return true;
}

bool tSfx::isEmpty(void) const
{
	return m_vData.empty();
}

std::uint32_t tSfx::getLength(void) const
{
	return m_vData.size();
}

void tSfx::normalize(void)
{
	// Get the biggest amplitude - negative or positive
	std::int8_t bMaxAmplitude = 0;
	for(const auto &Sample: m_vData) {
		auto Amplitude = abs(Sample);
		if(Amplitude > bMaxAmplitude) {
			bMaxAmplitude = int8_t(Amplitude);
		}
	}

	// Scale samples
	for(auto &Sample: m_vData) {
		Sample = (Sample * std::numeric_limits<int8_t>::max()) / bMaxAmplitude;
	}
}

void tSfx::divideAmplitude(std::uint8_t ubDivisor)
{
	for(auto &Sample: m_vData) {
		Sample /= ubDivisor;
	}
}

bool tSfx::isFittingMaxAmplitude(std::int8_t bMaxAmplitude) const
{
	for(auto &Sample: m_vData) {
		if(Sample > bMaxAmplitude) {
			return false;
		}
	}
	return true;
}

bool tSfx::hasEmptyFirstWord(void) const {
	return m_vData[0] == 0 && m_vData[1] == 0;
}

void tSfx::enforceEmptyFirstWord(void) {
	while(!hasEmptyFirstWord()) {
		m_vData.push_back(0);
		std::rotate(m_vData.rbegin(), m_vData.rbegin() + 1, m_vData.rend());
	}
}

void tSfx::padContents(std::uint8_t ubAlignment) {
		std::uint8_t ubAddCount = m_vData.size() % ubAlignment;
		for(std::uint8_t i = ubAddCount; i--;) {
			m_vData.push_back(0);
		}
}

tSfx tSfx::splitAfter(std::uint32_t ulSamples) {
	tSfx Out;
	if(m_vData.size() <= ulSamples) {
		return Out;
	}

	for(std::uint32_t ulPos = ulSamples; ulPos < m_vData.size(); ++ulPos) {
		Out.m_vData.push_back(m_vData[ulPos]);
	}
	Out.m_ulFreq = m_ulFreq;
	m_vData.resize(ulSamples);
	return Out;
}
