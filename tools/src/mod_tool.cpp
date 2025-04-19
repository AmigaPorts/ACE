/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstdlib>
#include <vector>
#include <memory>
#include <algorithm>
#include <fstream>
#include <string_view>
#include <fmt/format.h>
#include "common/mod.h"
#include "common/sfx.h"
#include "common/logging.h"
#include "common/endian.h"

using namespace std::string_view_literals;

int main(int lArgCount, const char *pArgs[])
{
	if(lArgCount <= 1) {
		nLog::error("no parameters specified\n");
		return EXIT_FAILURE;
	}

	std::vector<std::shared_ptr<tMod>> vModsIn;
	std::vector<std::string> vOutNames;
	std::string szSamplePackPath;
	bool isStripSamples = true;
	bool isCompressed = false;

	for(auto ArgIndex = 0; ArgIndex < lArgCount; ++ArgIndex) {
		if(pArgs[ArgIndex] == "-i"sv) {
			vModsIn.push_back(std::make_shared<tMod>(pArgs[++ArgIndex]));
		}
		else if(pArgs[ArgIndex] == "-o"sv) {
			vOutNames.push_back(pArgs[++ArgIndex]);
		}
		else if(pArgs[ArgIndex] == "-sp"sv) {
			szSamplePackPath = pArgs[++ArgIndex];
		}
		else if(pArgs[ArgIndex] == "-c"sv) {
			isCompressed = true;
		}
	}

	if(vModsIn.size() != vOutNames.size()) {
		nLog::error(
			"Output path count ({}) doesn't match input file count ({})\n",
			vOutNames.size(), vModsIn.size()
		);
		return EXIT_FAILURE;
	}

	for(const auto &pMod: vModsIn) {
		const auto &ModSamples = pMod->getSamples();
		const auto SampleCount = ModSamples.size();
		for(auto SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex) {
			for(auto OtherIndex = SampleIndex + 1; OtherIndex < SampleCount; ++OtherIndex) {
				if(
					!ModSamples[SampleIndex].m_szName.empty() &&
					ModSamples[SampleIndex].m_szName == ModSamples[OtherIndex].m_szName
				) {
					nLog::error(
						"Mod '{}' has duplicate sample name: '{}'\n",
						pMod->getSongName(), ModSamples[SampleIndex].m_szName
					);
					return EXIT_FAILURE;
				}
			}
		}
	}

	// Get the collection of unique samples
	std::vector<tSample> vMergedSamples;
	for(const auto &pMod: vModsIn) {
		const auto &ModSamples = pMod->getSamples();
		for(const auto &ModSample: ModSamples) {
			if(ModSample.m_vData.empty()) {
				continue;
			}

			// Check if sample is on the list
			auto FoundMergedSample = std::find_if(
				vMergedSamples.begin(), vMergedSamples.end(),
				[&ModSample](const tSample &RefSample) -> bool {
					bool isSameName = (RefSample.m_szName == ModSample.m_szName);
					return isSameName;
				}
			);
			if(FoundMergedSample == vMergedSamples.end()) {
				// Sample not on the list - add it
				fmt::print(FMT_STRING("Adding sample '{}' at index {}\n"), ModSample.m_szName, vMergedSamples.size());
				vMergedSamples.push_back(ModSample);
			}
			else {
				// Verify that Samples are exactly the same
				if(ModSample != *FoundMergedSample) {
					nLog::error(
						"sample {} '{}' have different contents in mod '{}' than in others\n",
						(&ModSample - ModSamples.data()), ModSample.m_szName, pMod->getSongName()
					);
					return EXIT_FAILURE;
				}
			}
		}
	}

	// Change the sample indices in modules so that they can be used with the samplepack
	for(const auto &pMod: vModsIn) {
		// Scan the samples to get the reorder table
		std::vector<uint8_t> vReorder; // [idxOld] = idxNew
		for(const auto ModSample: pMod->getSamples()) {
			std::uint8_t ubIdxNew = 0;
			if(!ModSample.m_szName.empty()) {
				// Find the sample index in new samplepack
				auto SampleNew = std::find_if(
					vMergedSamples.begin(), vMergedSamples.end(),
					[&ModSample](const tSample &MergedSample) -> bool {
						bool isSame = MergedSample.m_szName == ModSample.m_szName;
						return isSame;
					}
				);
				ubIdxNew = uint8_t(std::distance(vMergedSamples.begin(), SampleNew));
			}
			vReorder.push_back(ubIdxNew);
		}

		// Reorder samples, replace sample indices in notes
		pMod->reorderSamples(vReorder);

		// Trim sample data - Don't do it or sample defs will have data length 0
		// pMod->clearSampleData();
	}

	if(!szSamplePackPath.empty()) {
		fmt::print("Writing sample pack to {}...\n", szSamplePackPath);
		std::ofstream FileSamplePack;
		FileSamplePack.open(szSamplePackPath, std::ios::binary);
		std::uint8_t ubVersion = 2;
		std::uint8_t ubSampleCount = std::uint8_t(vMergedSamples.size());
		FileSamplePack.write(reinterpret_cast<char*>(&ubVersion), sizeof(ubVersion));
		FileSamplePack.write(reinterpret_cast<char*>(&ubSampleCount), sizeof(ubSampleCount));

		for(const auto &Sample: vMergedSamples) {
			std::uint16_t uwSampleWordLengthBe = nEndian::toBig16(std::uint16_t(Sample.m_vData.size()));
			FileSamplePack.write(reinterpret_cast<char*>(&uwSampleWordLengthBe), sizeof(uwSampleWordLengthBe));
			if(isCompressed) {
				std::uint32_t ulUncompressedSize = std::uint32_t(Sample.m_vData.size() * sizeof(Sample.m_vData[0]));
				auto UncompressedBytes = std::span(reinterpret_cast<const int8_t*>(Sample.m_vData.data()), ulUncompressedSize);
				auto vCompressed = tSfx::compressLosslessDpcm(UncompressedBytes);
				fmt::print(
					FMT_STRING("Compressed: {}/{} ({:.2f}%)\n"),
					vCompressed.size(), ulUncompressedSize,
					(float(vCompressed.size()) / ulUncompressedSize * 100)
				);

				auto vDecompressed = tSfx::decompressLosslessDpcm(
					vCompressed, std::uint32_t(Sample.m_vData.size() * sizeof(std::uint16_t))
				);
				for(auto i = 0; i < UncompressedBytes.size(); ++i) {
					if(vDecompressed[i] != UncompressedBytes[i]) {
						nLog::error("mismatch on byte {}", i);
						return EXIT_FAILURE;
					}
				}

				std::uint32_t ulCompressedSizeBe = nEndian::toBig32(std::uint32_t(vCompressed.size()));
				FileSamplePack.write(reinterpret_cast<char*>(&ulCompressedSizeBe), sizeof(ulCompressedSizeBe));
				FileSamplePack.write(
					reinterpret_cast<const char*>(vCompressed.data()),
					vCompressed.size() * sizeof(vCompressed[0])
				);
			}
			else {
				std::uint32_t ulCompressedSizeBe = 0;
				FileSamplePack.write(reinterpret_cast<char*>(&ulCompressedSizeBe), sizeof(ulCompressedSizeBe));
				FileSamplePack.write(
					reinterpret_cast<const char*>(Sample.m_vData.data()),
					Sample.m_vData.size() * sizeof(Sample.m_vData[0])
				);
			}
		}
	}

	std::uint32_t ulIdx = 0;
	for(const auto &pMod: vModsIn) {
		fmt::print("Writing {} to {}...\n", pMod->getSongName(), vOutNames[ulIdx]);
		pMod->toMod(vOutNames[ulIdx], isStripSamples);
		++ulIdx;
	}

	fmt::print("All done!\n");
	return EXIT_SUCCESS;
}
