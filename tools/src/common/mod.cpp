/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mod.h"
#include <fstream>
#include <exception>
#include <fmt/format.h>
#include "endian.h"

#define SAMPLE_NAME_SIZE 22

tMod::tMod(const std::string &szFileName)
{
	std::ifstream FileIn;
	FileIn.open(szFileName, std::ios::binary);

	FileIn.seekg(0, std::ios::end);
	std::uint32_t ulFileSize = FileIn.tellg(); // For pattern count calc

	FileIn.seekg(0, std::ios::beg);

	char szSongNameRaw[20];
	FileIn.read(szSongNameRaw, sizeof(szSongNameRaw));

	m_szSongName = szSongNameRaw;

	// Read sample info
	std::uint32_t ulTotalSampleSize = 0;
	for(std::uint8_t i = 0; i < 31; ++i) {
		char szSampleNameRaw[SAMPLE_NAME_SIZE];
		std::uint16_t uwSampleLen, uwSampleRepeatOffs, uwSampleRepeatLength;
		std::uint8_t ubSampleFineTune, ubSampleLinearVolume;

		FileIn.read(szSampleNameRaw, sizeof(szSampleNameRaw));
		FileIn.read(
			reinterpret_cast<char*>(&uwSampleLen), sizeof(uwSampleLen)
		); // In words, big endian
		FileIn.read(
			reinterpret_cast<char*>(&ubSampleFineTune), sizeof(ubSampleFineTune)
		);
		FileIn.read(
			reinterpret_cast<char*>(&ubSampleLinearVolume),
			sizeof(ubSampleLinearVolume)
		);
		FileIn.read(
			reinterpret_cast<char*>(&uwSampleRepeatOffs), sizeof(uwSampleRepeatOffs)
		); // In words, big endian
		FileIn.read(
			reinterpret_cast<char*>(&uwSampleRepeatLength), sizeof(uwSampleRepeatLength)
		); // In words, big endian

		// Convert stuff
		uwSampleLen = nEndian::fromBig16(uwSampleLen);
		uwSampleRepeatLength = nEndian::fromBig16(uwSampleRepeatLength);
		uwSampleRepeatOffs = nEndian::fromBig16(uwSampleRepeatOffs);

		// Data read successfully, fill sample info
		tSample Sample;
		for (std::uint8_t ubCharIndex = 0; ubCharIndex < SAMPLE_NAME_SIZE; ++ubCharIndex){
			// sample name to uppercase, to avoid duplicates
			// will be reworked to: comparison between samples on temporarily uppercased copies
			szSampleNameRaw[ubCharIndex] = std::toupper(szSampleNameRaw[ubCharIndex]);
		}
		Sample.m_szName = szSampleNameRaw;
		Sample.m_ubFineTune = ubSampleFineTune;
		Sample.m_ubVolume = ubSampleLinearVolume;
		Sample.m_uwRepeatLength = uwSampleRepeatLength;
		Sample.m_uwRepeatOffs = uwSampleRepeatOffs;
		Sample.m_vData.resize(uwSampleLen);
		m_vSamples.push_back(std::move(Sample));

		ulTotalSampleSize += uwSampleLen * 2;
	}

	FileIn.read(reinterpret_cast<char*>(&m_ubArrangementLength), sizeof(m_ubArrangementLength));
	FileIn.read(reinterpret_cast<char*>(&m_ubSongEndPos), sizeof(m_ubSongEndPos));

	// Arrangement - always 128-byte long
	m_vArrangement.resize(128);
	FileIn.read(reinterpret_cast<char*>(m_vArrangement.data()), 128);

	char pTag[5] = {'\0'};
	FileIn.read(pTag, 4);
	m_szFileFormatTag = pTag;

	if(
		m_szFileFormatTag != "M.K." && m_szFileFormatTag != "FLT4" &&
		m_szFileFormatTag != "M!K!" && m_szFileFormatTag != "4CHN"
	) {
		fmt::print("ERR: unsupported file format tag: {}", m_szFileFormatTag);
		throw std::exception();
	}

	// Determine pattern count
	std::uint32_t ulCurrPos = FileIn.tellg();
	std::uint32_t ulPatternDataSize = (ulFileSize - ulCurrPos - ulTotalSampleSize);
	if((ulPatternDataSize / 1024) * 1024 != ulPatternDataSize) {
		fmt::print("ERR: unexpected size of pattern data!");
		throw std::exception();
	}
	std::uint8_t ubPatternCount = (ulFileSize - ulCurrPos - ulTotalSampleSize) / 1024;

	// Read pattern data
	m_vPatterns.resize(ubPatternCount);
	for(std::uint8_t ubPattern = 0; ubPattern < ubPatternCount; ++ubPattern) {
		for(std::uint8_t ubRow = 0; ubRow < 64; ++ubRow) {
			std::array<tNote, 4> NotesInRow;
			for(std::uint8_t i = 0; i < 4; ++i) {
				std::uint32_t ulRawNote; // [instrumentHi:4] [period:12] [instrumentLo:4] [cmdNo:4] [cmdArg:8]
				FileIn.read(reinterpret_cast<char*>(&ulRawNote), sizeof(ulRawNote));
				ulRawNote = nEndian::fromBig32(ulRawNote);
				tNote Note = {
					.ubInstrument = uint8_t(((ulRawNote & 0xF0'00'00'00) >> 24) | ((ulRawNote & 0xF0'00) >> 12)),
					.uwPeriod =     uint16_t((ulRawNote & 0x0F'FF'00'00) >> 16),
					.ubCmd =         uint8_t((ulRawNote & 0x00'00'0F'00) >> 8),
					.ubCmdArg =       uint8_t(ulRawNote & 0x00'00'00'FF),
				};
				NotesInRow[i] = std::move(Note);
			}
			m_vPatterns[ubPattern].push_back(NotesInRow);
		}
	}

	// Read sample data
	for(auto &Sample: m_vSamples) {
		// Read raw sample data
		FileIn.read(
			reinterpret_cast<char*>(Sample.m_vData.data()),
			Sample.m_vData.size() * sizeof(Sample.m_vData[0])
		);
	}
}

void tMod::toMod(const std::string &szFileName, bool isSkipSampleData)
{
	std::ofstream FileOut;
	FileOut.open(szFileName, std::ios::binary);

	// Song name - without garbage after null terminator
	char szSongNameRaw[20] = {'\0'};
	strcpy(szSongNameRaw, m_szSongName.c_str());
	FileOut.write(szSongNameRaw, sizeof(szSongNameRaw));

	// Samples
	for(const auto &Sample: m_vSamples) {
		std::uint16_t uwSampleLen = nEndian::toBig16(Sample.m_vData.size());
		std::uint16_t uwSampleRepeatOffs = nEndian::toBig16(Sample.m_uwRepeatOffs);
		std::uint16_t uwSampleRepeatLength = nEndian::toBig16(Sample.m_uwRepeatLength);

		// Ensure that garbage after null terminator doesn't get copied
		char szSampleNameRaw[22] = {'\0'};
		strcpy(szSampleNameRaw, Sample.m_szName.c_str());

		FileOut.write(szSampleNameRaw, sizeof(szSampleNameRaw));
		FileOut.write(
			reinterpret_cast<char*>(&uwSampleLen), sizeof(uwSampleLen)
		); // In words, big endian
		FileOut.write(
			reinterpret_cast<const char*>(&Sample.m_ubFineTune), sizeof(Sample.m_ubFineTune)
		);
		FileOut.write(
			reinterpret_cast<const char*>(&Sample.m_ubVolume), sizeof(Sample.m_ubVolume)
		);
		FileOut.write(
			reinterpret_cast<char*>(&uwSampleRepeatOffs), sizeof(uwSampleRepeatOffs)
		); // In words, big endian
		FileOut.write(
			reinterpret_cast<char*>(&uwSampleRepeatLength), sizeof(uwSampleRepeatLength)
		); // In words, big endian
	}

	// Pattern count, song end jump pos
	FileOut.write(reinterpret_cast<char*>(&m_ubArrangementLength), sizeof(m_ubArrangementLength));
	FileOut.write(reinterpret_cast<char*>(&m_ubSongEndPos), sizeof(m_ubSongEndPos));

	// Pattern table
	FileOut.write(reinterpret_cast<char*>(m_vArrangement.data()), m_vArrangement.size());

	// File format tag
	FileOut.write(m_szFileFormatTag.data(), 4);

	// Pattern data
	for(const auto &Pattern: m_vPatterns) {
		for(std::uint8_t ubRow = 0; ubRow < 64; ++ubRow) {
			for(std::uint8_t ubChan = 0; ubChan < 4; ++ubChan) {
				const auto Note = &Pattern[ubRow][ubChan];
				// [instrumentHi:4] [period:12] [instrumentLo:4] [cmdNo:4] [cmdArg:8]
				std::uint32_t ulNoteRaw = nEndian::toBig32(
					((Note->ubInstrument & 0xF0  ) << 24) |
					((Note->uwPeriod     & 0x0FFF) << 16) |
					((Note->ubInstrument & 0x0F  ) << 12) |
					((Note->ubCmd & 0xF) << 8) | (Note->ubCmdArg & 0xFF)
				);
				FileOut.write(reinterpret_cast<char*>(&ulNoteRaw), sizeof(ulNoteRaw));
				FileOut.flush();
			}
		}
	}

	if(!isSkipSampleData) {
		// Sample data
		for(const auto &Sample: m_vSamples) {
			FileOut.write(
				reinterpret_cast<const char*>(Sample.m_vData.data()),
				Sample.m_vData.size() * 2
			);
		}
	}
}

const std::vector<tSample> &tMod::getSamples(void) const
{
	return m_vSamples;
}

const std::string &tMod::getSongName(void) const
{
	return m_szSongName;
}

void tMod::reorderSamples(const std::vector<uint8_t> vNewOrder)
{
	// Generate reordered sample defs - do it on copy because of X->Y & Y->X
	std::vector<tSample> vSamplesNew(m_vSamples.size());
	for(std::uint8_t ubOldIdx = 0; ubOldIdx < vNewOrder.size(); ++ubOldIdx) {
		if(!m_vSamples[ubOldIdx].m_szName.empty()) {
			std::uint8_t ubNewIdx = vNewOrder[ubOldIdx];
			vSamplesNew[ubNewIdx] = m_vSamples[ubOldIdx];
		}
	}

	// Replace all at once
	m_vSamples = vSamplesNew;

	// Replace sample index in patterns
	for(auto &Pattern: m_vPatterns) {
		for(auto &Row: Pattern) {
			for(auto &Channel: Row) {
				if(Channel.ubInstrument) {
					// instruments are starting with 1, zero means "none"
					Channel.ubInstrument = vNewOrder[Channel.ubInstrument - 1] + 1;
				}
			}
		}
	}
}


void tMod::clearSampleData(void)
{
	for(auto &Sample: m_vSamples) {
		Sample.m_vData.clear();
	}
}

bool tSample::operator ==(const tSample &Other) const
{
	bool isSame = (
		m_szName == Other.m_szName &&
		m_ubFineTune == Other.m_ubFineTune &&
		m_ubVolume == Other.m_ubVolume &&
		m_uwRepeatLength == Other.m_uwRepeatLength &&
		m_uwRepeatOffs == Other.m_uwRepeatOffs &&
		m_vData == Other.m_vData
	);
	return isSame;
}
