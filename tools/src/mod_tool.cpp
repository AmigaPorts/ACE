/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "common/mod.h"
#include <cstdlib>
#include <vector>
#include <memory>
#include <algorithm>
#include <fstream>
#include <fmt/format.h>
#include "common/sfx.h"

int main(int lArgCount, const char *pArgs[])
{
	if(lArgCount <= 1) {
		fmt::print("ERR: no parameters specified\n");
		return EXIT_FAILURE;
	}

	std::vector<std::shared_ptr<tMod>> vModsIn;
	std::vector<std::string> vOutNames;
	std::string szSamplePackPath;
	bool isStripSamples = true;

	for(auto ArgIndex = 0; ArgIndex < lArgCount; ++ArgIndex) {
		if(pArgs[ArgIndex] == std::string("-i")) {
			vModsIn.push_back(std::make_shared<tMod>(pArgs[++ArgIndex]));
		}
		else if(pArgs[ArgIndex] == std::string("-o")) {
			vOutNames.push_back(pArgs[++ArgIndex]);
		}
		else if(pArgs[ArgIndex] == std::string("-sp")) {
			szSamplePackPath = pArgs[++ArgIndex];
		}
	}

	if(vModsIn.size() != vOutNames.size()) {
		fmt::print(
			"ERR: Output path count ({}) doesn't match input file count ({})\n",
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
					fmt::print(
						"ERR: Mod '{}' has duplicate sample name: '{}'\n",
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
					fmt::print(
						"ERR: sample {} '{}' have different contents in mod '{}' than in others\n",
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
		std::uint8_t ubSampleCount = std::uint8_t(vMergedSamples.size());
		FileSamplePack.write(reinterpret_cast<char*>(&ubSampleCount), sizeof(ubSampleCount));
		for(const auto &Sample: vMergedSamples) {
			std::uint16_t uwSampleWordLength = std::uint16_t(Sample.m_vData.size());
			FileSamplePack.write(reinterpret_cast<char*>(&uwSampleWordLength), sizeof(uwSampleWordLength));
			if(true) {
				std::uint32_t ulUncompressedSize = std::uint32_t(Sample.m_vData.size() * sizeof(Sample.m_vData[0]));
				auto vCompressed = tSfx::compressLosslessDpcm(
					std::span(reinterpret_cast<const int8_t*>(Sample.m_vData.data()),
					ulUncompressedSize
				));
				fmt::print(
					FMT_STRING("Compressed: {}/{} ({:.2f}%)\n"),
					vCompressed.size(), ulUncompressedSize,
					(float(vCompressed.size()) / ulUncompressedSize * 100)
				);

				std::uint32_t ulCompressedSize = std::uint32_t(vCompressed.size());
				FileSamplePack.write(reinterpret_cast<char*>(&ulCompressedSize), sizeof(ulCompressedSize));
				FileSamplePack.write(
					reinterpret_cast<const char*>(vCompressed.data()),
					vCompressed.size() * sizeof(vCompressed[0])
				);
			}
			else {
				std::uint32_t ulCompressedSize = 0;
				FileSamplePack.write(reinterpret_cast<char*>(&ulCompressedSize), sizeof(ulCompressedSize));
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
