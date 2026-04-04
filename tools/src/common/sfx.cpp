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

		// Verify that compression actually works
		std::vector<std::int8_t> vDecompressed = tSfx::decompressLosslessDpcm(vCompressed, std::uint32_t(m_vData.size()));
		for(auto i = 0; i < m_vData.size(); ++i) {
			if(vDecompressed[i] != m_vData[i]) {
				nLog::error("mismatch on byte {}", i);
				return false;
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
	static constexpr auto Sgn = [](std::int8_t bDelta) { return (bDelta > 0) ? 1 : (bDelta < 0) ? -1 : 0;};
	std::int8_t bPrevSample = 0;
	std::uint32_t ulCtl = 0;
	std::vector<uint8_t> vCompressed;
	std::vector<uint8_t> vChunk;
	vChunk.reserve(16);
	std::uint8_t ubCtlSize = 0;
	for(auto i = 0; i < Uncompressed.size(); ++i) {
		auto Delta = Uncompressed[i] - bPrevSample;
		bool isPacked = false;
		if(Delta == 0) {
			// Skip sample
			isPacked = true;
			++ubCtlSize;
			// fmt::print("skip {}: {}\n", i, bPrevSample);
		}
		else if(i + 1 < Uncompressed.size() && ubCtlSize <= 14) {
			auto DeltaNext = Uncompressed[i + 1] - Uncompressed[i];
			if(std::abs(Delta) <= 16 && std::abs(DeltaNext) <= 16 && DeltaNext != 0) {
				if(Sgn(Delta) > 0) {
					// fmt::print("add delta A {}: {}\n", i, abs(Delta));
				}
				else {
					// fmt::print("sub delta A {}: {}\n", i, abs(Delta));
				}
				if(Sgn(DeltaNext) > 0) {
					// fmt::print("add delta B {}: {}\n", i, abs(DeltaNext));
				}
				else {
					// fmt::print("sub delta B {}: {}\n", i, abs(DeltaNext));
				}
				ulCtl |= ((Sgn(Delta) > 0) ? 0b01 : 0b10) << 30;
				ulCtl >>= 2;
				ulCtl |= ((Sgn(DeltaNext) > 0) ? 0b01 : 0b10) << 30;
				ubCtlSize += 2;
				std::uint8_t ubPacked = ((abs(DeltaNext) - 1) << 4) | (abs(Delta) - 1);
				vChunk.push_back(ubPacked);
				isPacked = true;
				bPrevSample = Uncompressed[i + 1];
				++i;
			}
		}
		if(!isPacked) {
			vChunk.push_back(Uncompressed[i]);
			bPrevSample = Uncompressed[i];
			ulCtl |= 0b11 << 30;
			++ubCtlSize;
			// fmt::print("full sample {}: {}\n", i, bPrevSample);
		}
		if(ubCtlSize == 16) {
			vCompressed.push_back((ulCtl >> 24) & 0xFF);
			vCompressed.push_back((ulCtl >> 16) & 0xFF);
			vCompressed.push_back((ulCtl >> 8) & 0xFF);
			vCompressed.push_back((ulCtl >> 0) & 0xFF);
			// fmt::print("write ctl: {:08X}\n", ulCtl);
			vCompressed.insert(vCompressed.end(), vChunk.begin(), vChunk.end());
			vChunk.clear();
			ubCtlSize = 0;
			ulCtl = 0;
		}
		else {
			ulCtl >>= 2;
		}
	}

	if(ubCtlSize) {
		// trailing stuff, incomplete chunk - fill with zeros
		// at least one shift by 2 was done already, so shift max by 30
		ulCtl >>= (30 - ubCtlSize * 2);
		vCompressed.push_back((ulCtl >> 24) & 0xFF);
		vCompressed.push_back((ulCtl >> 16) & 0xFF);
		vCompressed.push_back((ulCtl >> 8) & 0xFF);
		vCompressed.push_back((ulCtl >> 0) & 0xFF);
		vCompressed.insert(vCompressed.end(), vChunk.begin(), vChunk.end());
		// fmt::print("tail ctl: {:08X}, ctl size: {}, tail size: {}\n", ulCtl, ubCtlSize, vChunk.size());
	}

	return vCompressed;
}

std::vector<int8_t> tSfx::decompressLosslessDpcm(
	const std::vector<uint8_t> &vCompressed, std::uint32_t ulDecompressedSize
)
{
	std::vector<std::int8_t> vDecompressed;
	vDecompressed.resize(ulDecompressedSize);
	std::copy(vCompressed.begin(), vCompressed.end(), &vDecompressed[ulDecompressedSize - vCompressed.size()]);
	std::uint32_t ulReadPos = 0;
	std::uint32_t ulCtl;
	std::uint32_t ulWritePos = 0;
	std::int8_t bLastSample = 0;
	while(true) {
		ulCtl = (
			(uint8_t(vCompressed[ulReadPos++]) << 24) |
			(uint8_t(vCompressed[ulReadPos++]) << 16) |
			(uint8_t(vCompressed[ulReadPos++]) << 8) |
			uint8_t(vCompressed[ulReadPos++])
		);
		// fmt::print("read ctl: {:08X}\n", ulCtl);
		for(auto i = 0; i < 16; ++i) {
			std::uint8_t ubCtl = ulCtl & 0b11;
			if(ubCtl == 0) {
				// fmt::print("skip {}: {}\n", ulWritePos, bLastSample);
				vDecompressed[ulWritePos++] = bLastSample;
			}
			else if(ubCtl == 0b11) {
				bLastSample = vCompressed[ulReadPos++];
				// fmt::print("full sample {}: {}\n", ulWritePos, bLastSample);
				vDecompressed[ulWritePos++] = bLastSample;
			}
			else {
				std::uint8_t ubNibbles = vCompressed[ulReadPos++];
				if(ubCtl == 0b01) {
					bLastSample += (ubNibbles & 0xF) + 1;
					// fmt::print("add delta A {}: {}\n", ulWritePos, (ubNibbles & 0xF) + 1);
					vDecompressed[ulWritePos++] = bLastSample;
				}
				else {
					bLastSample -= (ubNibbles & 0xF) + 1;
					// fmt::print("sub delta A {}: {}\n", ulWritePos, (ubNibbles & 0xF) + 1);
					vDecompressed[ulWritePos++] = bLastSample;
				}
				ubNibbles >>= 4;
				ulCtl >>= 2;
				++i;
				ubCtl = ulCtl & 0b11;
				if(ubCtl == 0b01) {
					bLastSample += ubNibbles + 1;
					// fmt::print("add delta B {}: {}\n", ulWritePos, ubNibbles + 1);
					vDecompressed[ulWritePos++] = bLastSample;
				}
				else {
					bLastSample -= ubNibbles + 1;
					// fmt::print("sub delta B {}: {}\n", ulWritePos, ubNibbles + 1);
					vDecompressed[ulWritePos++] = bLastSample;
				}
			}

			ulCtl >>= 2;
			if(ulWritePos >= std::uint32_t(vDecompressed.size())) {
				return vDecompressed;
			}
		}
	}
}
