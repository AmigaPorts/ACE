/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "wav.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include "logging.h"

enum class tAudioFormat: std::uint16_t {
	PCM = 1
};

bool tryRead(std::ifstream &Stream, char *Buffer, std::uint32_t ulSize)
{
	// ifstream::read doesn't return meaningful return value
	// so here's a helper fn for it
	Stream.read(Buffer, ulSize);
	return Stream.good();
}

tWav::tWav(const std::string &szPath)
{
	// Read header - chunk
	std::string szRiffId(4, '\0');
	std::ifstream StreamFile(szPath.c_str(), std::ios::binary);
	if(!StreamFile.is_open()) {
		nLog::error("Couldn't open: '{}'", szPath);
		return;
	}
	tryRead(StreamFile, szRiffId.data(), szRiffId.length());
	if(szRiffId != "RIFF") {
		nLog::error("Couldn't find RIFF header, got: '{}'", szRiffId);
		return;
	}

	std::uint32_t ulChunkSize;
	tryRead(StreamFile, reinterpret_cast<char*>(&ulChunkSize), sizeof(ulChunkSize));

	std::string szFormat(4, '\0');
	tryRead(StreamFile, szFormat.data(), szFormat.length());

	if(szFormat != "WAVE") {
		nLog::error("Unsupported format: '{}', expected 'WAVE'", szFormat);
		return;
	}

	// Read subchunks
	bool isEnd = false;
	std::string szSubchunkId(4, '\0');
	while(tryRead(StreamFile, szSubchunkId.data(), szSubchunkId.length())) {
		tSubchunk Subchunk;
		Subchunk.m_szId = szSubchunkId;
		tryRead(StreamFile, reinterpret_cast<char*>(&Subchunk.m_ulSize), sizeof(Subchunk.m_ulSize));
		Subchunk.m_szContents.resize(Subchunk.m_ulSize);
		tryRead(StreamFile, Subchunk.m_szContents.data(),Subchunk.m_szContents.length());
		m_vSubchunks.push_back(std::move(Subchunk));
	}

	// Find and read "fmt" subchunk
	auto *pSubchunkFmt = findSubchunk("fmt ");
	if(pSubchunkFmt == nullptr) {
		nLog::error("Couldn't find WAV 'fmt' subchunk");
		return;
	}
	std::stringstream StreamSubchunk(pSubchunkFmt->m_szContents);
	tAudioFormat eAudioFormat;
	std::uint16_t uwNumChannels, uwBlockAlign, uwBitsPerSample;
	std::uint32_t ulSampleRate, ulByteRate;
	StreamSubchunk.read(reinterpret_cast<char*>(&eAudioFormat), sizeof(eAudioFormat));
	StreamSubchunk.read(reinterpret_cast<char*>(&uwNumChannels), sizeof(uwNumChannels));
	StreamSubchunk.read(reinterpret_cast<char*>(&ulSampleRate), sizeof(ulSampleRate));
	StreamSubchunk.read(reinterpret_cast<char*>(&ulByteRate), sizeof(ulByteRate));
	StreamSubchunk.read(reinterpret_cast<char*>(&uwBlockAlign), sizeof(uwBlockAlign));
	StreamSubchunk.read(reinterpret_cast<char*>(&uwBitsPerSample), sizeof(uwBitsPerSample));

	if(eAudioFormat != tAudioFormat::PCM) {
		nLog::error("Unrecognized WAV audio format: {}", static_cast<int>(eAudioFormat));
		return;
	}
	if(uwNumChannels != 1) {
		nLog::error("Unsupported WAV channel count: {}", uwNumChannels);
		return;
	}
	if(uwBitsPerSample != 8 && uwBitsPerSample != 16) {
		nLog::error("Unsupported WAV bps: {}", uwBitsPerSample);
		return;
	}

	// Read "data" subchunk
	auto *pSubchunkData = findSubchunk("data");
	if(pSubchunkData == nullptr) {
		nLog::error("Couldn't find WAV 'data' subchunk");
		return;
	}

	m_ubBitsPerSample = uwBitsPerSample;
	m_ulSampleRate = ulSampleRate;
	m_vData.resize(pSubchunkData->m_szContents.length());
	memcpy(m_vData.data(), pSubchunkData->m_szContents.data(), pSubchunkData->m_szContents.length());
}

const tWav::tSubchunk *tWav::findSubchunk(const std::string &szId) const {
	auto Subchunk = std::find_if(
		m_vSubchunks.cbegin(), m_vSubchunks.cend(), [szId](const tSubchunk &Sc) {
			return Sc.m_szId == szId;
		}
	);
	if(Subchunk == m_vSubchunks.cend()) {
		return nullptr;
	}
	return &*Subchunk;
}

const std::vector<uint8_t> &tWav::getData(void) const
{
	return m_vData;
}

std::uint32_t tWav::getSampleRate(void) const
{
	return m_ulSampleRate;
}

std::uint8_t tWav::getBitsPerSample(void) const
{
	return m_ubBitsPerSample;
}
