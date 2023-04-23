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
			int8_t bSample = UnsignedSample - 128;
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
		for(uint32_t i = 0; i < DataSize; i += 2) {
			uint16_t uwRawSample = vWavData[i + 0] | (vWavData[i + 1] << 8);
			auto *pAsSigned = reinterpret_cast<int16_t *>(&uwRawSample);
			m_vData.push_back(*pAsSigned / 256);
		}
	}

	// Ptplayer requires first sample word to be zero
	if(m_vData[0] != 0) {
		m_vData.push_back(0);
		std::rotate(m_vData.rbegin(), m_vData.rbegin() + 1, m_vData.rend());
	}
	if(m_vData[1] != 0) {
		m_vData.push_back(0);
		std::rotate(m_vData.rbegin(), m_vData.rbegin() + 1, m_vData.rend());
	}

	// Needs even number of bytes - Amiga reads it as words
	if(m_vData.size() & 1) {
		m_vData.push_back(0);
	}
}

bool tSfx::toSfx(const std::string &szPath) const {
	std::ofstream FileOut(szPath, std::ios::binary);

	const uint8_t ubVersion = 1;
	const uint16_t uwWordLength = nEndian::toBig16(uint16_t(m_vData.size() / 2));
	const uint16_t uwSampleReateHz = nEndian::toBig16(m_ulFreq);

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

void tSfx::normalize(void)
{
	// Get the biggest amplitude - negative or positive
	int8_t bMaxAmplitude = 0;
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

void tSfx::divideAmplitude(uint8_t ubDivisor)
{
	for(auto &Sample: m_vData) {
		Sample /= ubDivisor;
	}
}

bool tSfx::isFittingMaxAmplitude(int8_t bMaxAmplitude) const
{
	for(auto &Sample: m_vData) {
		if(Sample > bMaxAmplitude) {
			return false;
		}
	}
	return true;
}
