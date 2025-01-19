/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <fstream>
#include <filesystem>
#include <vector>
#include "common/logging.h"
#include "common/fs.h"
#include "common/endian.h"

struct tPakCompressEntry {
	std::string ShortPath;
	std::string Path;
	std::uint32_t ulSize;
	std::uint32_t ulChecksum;
};

static std::uint32_t adler32Buffer(const std::uint8_t *pData, std::uint32_t ulDataSize) {
	constexpr std::uint32_t modulo = 65521;
	std::uint32_t a = 1, b = 0;
	for(std::uint32_t i = 0; i < ulDataSize; ++i) {
		a += pData[i];
		if(a >= modulo) {
			a -= modulo;
		}
		b += a;
		if(b >= modulo) {
			b -= modulo;
		}
	}
	return (b << 16) | a;
}

static void printUsage(const std::string &szAppName) {
	using fmt::print;
	print("Usage:\n\t{} inDir outPak\n\n", szAppName);
	print("inDir\t- path to input directory.\n");
	print("outPak\t- path to output pak file.\n");
}

int main(int lArgCount, const char *pArgs[])
{
	const std::uint8_t ubMandatoryArgCnt = 2;
	// Mandatory args
	if(lArgCount - 1 < ubMandatoryArgCnt) {
		nLog::error("Too few arguments, expected {}", ubMandatoryArgCnt);
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	std::string InPath = pArgs[1];
	std::string OutPath = pArgs[2];

	if(!nFs::isDir(InPath)) {
		nLog::error("Path {} isn't a folder", InPath);
		return EXIT_FAILURE;
	}

	std::ofstream FilePak;
	FilePak.open(OutPath, std::ios::binary);
	std::vector<tPakCompressEntry> vEntries;


	auto AbsoluteBasePath = std::filesystem::absolute(InPath);
	bool isCollided = false;
  for (std::filesystem::recursive_directory_iterator i(InPath), end; i != end; ++i) {
    if (!is_directory(i->path())) {
			tPakCompressEntry Entry;
			Entry.ShortPath = std::filesystem::relative(i->path(), AbsoluteBasePath).generic_string();
			Entry.Path = i->path().generic_string();
			Entry.ulSize = std::uint32_t(std::filesystem::file_size(Entry.Path));
			Entry.ulChecksum = adler32Buffer(
				reinterpret_cast<const std::uint8_t*>(Entry.ShortPath.c_str()),
				std::uint32_t(Entry.ShortPath.size())
			);
			for(auto OtherIndex = 0; OtherIndex < vEntries.size(); ++OtherIndex) {
				if(Entry.ulChecksum == vEntries[OtherIndex].ulChecksum) {
					nLog::error("Entry {} checksum collision with entry {}", vEntries.size(), OtherIndex);
					isCollided = true;
				}
			}
			vEntries.push_back(Entry);
		}
	}
	fmt::print("Discovered {} files\n", vEntries.size());
	if(isCollided) {
		nLog::error("Aborting due to checksum collisions! Report an issue and/or change your file names a bit.");
		return EXIT_FAILURE;
	}

	std::uint16_t uwFileCount = std::uint16_t(vEntries.size());
	std::uint16_t uwFileCountBe = nEndian::toBig16(uwFileCount);
	FilePak.write(reinterpret_cast<char*>(&uwFileCountBe), sizeof(uwFileCountBe));
	std::uint32_t ulNextFileOffs = sizeof(uwFileCount) + (uwFileCount * 3 * sizeof(std::uint32_t));
	std::uint16_t i = 0;
	for(const auto &Entry: vEntries) {
		fmt::print(
			"Adding file {:4d}: '{}', offset: {}, size: {}, checksum: {:08X}...\n",
			i++, Entry.ShortPath, ulNextFileOffs, Entry.ulSize,
			Entry.ulChecksum
		);
		std::uint32_t ulChecksumBe = nEndian::toBig32(Entry.ulChecksum);
		std::uint32_t ulOffsBe = nEndian::toBig32(ulNextFileOffs);
		std::uint32_t ulSizeBe = nEndian::toBig32(Entry.ulSize);

		FilePak.write(reinterpret_cast<char*>(&ulChecksumBe), sizeof(ulChecksumBe));
		FilePak.write(reinterpret_cast<char*>(&ulOffsBe), sizeof(ulOffsBe));
		FilePak.write(reinterpret_cast<char*>(&ulSizeBe), sizeof(ulSizeBe));

		ulNextFileOffs += Entry.ulSize;
	}

	std::vector<char> FileContents;
	for(const auto &Entry: vEntries) {
		std::ifstream FileIn;
		FileIn.open(Entry.Path, std::ios::binary);
		FileContents.resize(Entry.ulSize);
		FileIn.read(FileContents.data(), Entry.ulSize);
		FilePak.write(FileContents.data(), Entry.ulSize);
	}

	fmt::print("All done!\n");
	return EXIT_SUCCESS;
}
