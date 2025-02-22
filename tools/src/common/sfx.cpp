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

bool tSfx::toSfx(const std::string &szPath, bool isCompress) const {
	std::ofstream FileOut(szPath, std::ios::binary);

	const std::uint8_t ubVersion = 2;
	const std::uint16_t uwWordLength = nEndian::toBig16(uint16_t(m_vData.size() / 2));
	const std::uint16_t uwSampleReateHz = nEndian::toBig16(m_ulFreq);

	FileOut.write(reinterpret_cast<const char*>(&ubVersion), sizeof(ubVersion));
	FileOut.write(reinterpret_cast<const char*>(&uwWordLength), sizeof(uwWordLength));
	FileOut.write(reinterpret_cast<const char*>(&uwSampleReateHz), sizeof(uwSampleReateHz));

	if(isCompress) {
		auto vCompressed = tSfx::compressLosslessDpcm(
			std::span(m_vData.data(), m_vData.size()
		));

		std::uint32_t ulCompressedLength = std::uint16_t(vCompressed.size());
		std::uint32_t ulCompressedLengthBe = nEndian::toBig32(ulCompressedLength);
		FileOut.write(reinterpret_cast<const char*>(&ulCompressedLengthBe), sizeof(ulCompressedLengthBe));
		FileOut.write(reinterpret_cast<const char*>(vCompressed.data()), vCompressed.size());
		fmt::print(
			FMT_STRING("Compressed: {}/{} ({:.2f}%)\n"),
			vCompressed.size(), m_vData.size(), (float(vCompressed.size()) / m_vData.size() * 100)
		);

		{
			constexpr auto UnpackNibble = [](uint8_t x) {
				struct {int8_t y: 4;} s;
				return s.y = x;
			};

			// Verify that compression actually works
			std::vector<std::int8_t> vDecompressed;
			vDecompressed.resize(m_vData.size());
			std::copy(vCompressed.begin(), vCompressed.end(), &vDecompressed[m_vData.size() - ulCompressedLength]);
			std::uint32_t ulReadPos = std::uint32_t(m_vData.size()) - ulCompressedLength;
			std::uint16_t uwCtl;
			std::uint32_t ulWritePos = 0;
			std::int8_t bLastSample = 0;
			while(ulReadPos < m_vData.size()) {
				uwCtl = (uint8_t(vDecompressed[ulReadPos++]) << 8) | uint8_t(vDecompressed[ulReadPos++]);
				for(auto i = std::min<std::uint32_t>(16, std::uint32_t(m_vData.size()) - ulReadPos); i--;) {
					if(uwCtl & 1) {
						if(ulWritePos > ulReadPos) {
							nLog::error("write pos {} > read pos {}", ulWritePos, ulReadPos);
							return false;
						}
						std::uint8_t ubNibbles = vDecompressed[ulReadPos++];
						bLastSample += UnpackNibble(ubNibbles & 0xF);
						ubNibbles >>= 4;
						vDecompressed[ulWritePos++] = bLastSample;
						bLastSample += UnpackNibble(ubNibbles & 0xF);
						vDecompressed[ulWritePos++] = bLastSample;
					}
					else {
						if(ulWritePos > ulReadPos) {
							nLog::error("write pos {} > read pos {}", ulWritePos, ulReadPos);
							return false;
						}
						bLastSample = vDecompressed[ulReadPos++];
						vDecompressed[ulWritePos++] = bLastSample;
					}
					uwCtl >>= 1;
				}
			}

			for(auto i = 0; i < m_vData.size(); ++i) {
				if(vDecompressed[i] != m_vData[i]) {
					nLog::error("mismatch on byte {}", i);
					return false;
				}
			}
		}
	}
	else {
		std::uint32_t ulCompressedLength = 0;
		FileOut.write(reinterpret_cast<const char*>(&ulCompressedLength), sizeof(ulCompressedLength));
		FileOut.write(reinterpret_cast<const char*>(m_vData.data()), m_vData.size());
	}

	return true;
}

bool tSfx::isEmpty(void) const
{
	return m_vData.empty();
}

std::uint32_t tSfx::getLength(void) const
{
	return std::uint32_t(m_vData.size());
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

std::vector<uint8_t> tSfx::compressLosslessDpcm(std::span<const int8_t> Uncompressed)
{
	static constexpr auto PackNibble = [](std::int8_t bNibble) -> std::uint8_t {
		return bNibble & 0b1111;
	};

	std::int8_t bPrevSample = 0;
	std::uint16_t uwCtl = 0;
	std::vector<uint8_t> vCompressed;
	std::vector<uint8_t> vChunk;
	vChunk.reserve(16);
	for(auto i = 0; i < Uncompressed.size(); ++i) {
		auto Delta = Uncompressed[i] - bPrevSample;
		bool isPacked = false;
		if(i + 1 < Uncompressed.size()) {
			auto DeltaNext = Uncompressed[i + 1] - Uncompressed[i];
			if(std::abs(Delta) <= 7 && std::abs(DeltaNext) <= 7) {
				uwCtl |= 1 << 15;
				std::uint8_t ubPacked = (
					(PackNibble(DeltaNext) << 4) | PackNibble(Delta)
				);
				vChunk.push_back(ubPacked);
				isPacked = true;
				bPrevSample = Uncompressed[i + 1];
				++i;
			}
		}
		if(!isPacked) {
			vChunk.push_back(Uncompressed[i]);
			bPrevSample = Uncompressed[i];
		}
		if(vChunk.size() == 16) {
			vCompressed.push_back(uwCtl >> 8);
			vCompressed.push_back(uwCtl & 0xFF);
			vCompressed.insert(vCompressed.end(), vChunk.begin(), vChunk.end());
			vChunk.clear();
			uwCtl = 0;
		}
		else {
			uwCtl >>= 1;
		}
	}

	// trailing stuff, incomplete chunk - fill with zeros
	uwCtl >>= 15 - vChunk.size();
	vCompressed.push_back(uwCtl >> 8);
	vCompressed.push_back(uwCtl & 0xFF);
	vCompressed.insert(vCompressed.end(), vChunk.begin(), vChunk.end());

	return vCompressed;
}
