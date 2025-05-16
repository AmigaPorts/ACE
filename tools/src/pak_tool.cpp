/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <fstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include "common/logging.h"
#include "common/fs.h"
#include "common/endian.h"
#include "common/compress.hpp"

struct tPakEntry {
	std::string ShortPath;
	std::string Path;
	std::uint32_t ulChecksum;
	std::uint32_t ulUncompressedSize;
	std::vector<std::uint8_t> vData;
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
	print("Usage:\n\t{} inDir outPak [extraOpts]\n\n", szAppName);
	print("Required arguments:\n");
	print("\tinDir   Path to input directory.\n");
	print("\toutPak  Path to output pak file.\n");
	print("Extra options:\n");
	print("\t-c                Enable compression.\n");
	print("\t-r orderfile.txt  Reorder files with list file - one path per line. Omitted files will be appended at the end.\n");
}

int main(int lArgCount, const char *pArgs[])
{
	using namespace std::string_view_literals;

	const std::uint8_t ubMandatoryArgCnt = 2;
	if(lArgCount - 1 < ubMandatoryArgCnt) {
		nLog::error("Too few arguments, expected {}", ubMandatoryArgCnt);
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	std::string InPath(pArgs[1]);
	std::string OutPath(pArgs[2]);
	std::string OrderPath;
	bool isCompressed = false;

	for(auto ArgIndex = 3; ArgIndex < lArgCount; ++ArgIndex) {
		std::string_view Arg = pArgs[ArgIndex];
		if(Arg == "-c"sv) {
			isCompressed = true;
		}
		else if(Arg == "-r"sv && ArgIndex + 1 < lArgCount) {
			OrderPath = pArgs[++ArgIndex];
		}
		else {
			nLog::error("Unknown arg or missing value: '{}'", pArgs[ArgIndex]);
			printUsage(pArgs[0]);
			return EXIT_FAILURE;
		}
	}

	if(!nFs::isDir(InPath)) {
		nLog::error("Path {} isn't a folder", InPath);
		return EXIT_FAILURE;
	}

	std::ofstream FilePak;
	FilePak.open(OutPath, std::ios::binary);
	if(FilePak.fail()) {
		nLog::error("Can't open the file for writing");
		return EXIT_FAILURE;
	}
	std::vector<tPakEntry> vEntries;

	auto AbsoluteBasePath = std::filesystem::absolute(InPath);
	bool isCollided = false;
	std::vector<std::uint8_t> vFileContents;
	std::vector<std::uint8_t> vPackBuffer;
	std::vector<std::uint8_t> vDecompressed;
  for (std::filesystem::recursive_directory_iterator i(InPath), end; i != end; ++i) {
    if (!is_directory(i->path())) {
			tPakEntry Entry;
			Entry.ShortPath = std::filesystem::relative(i->path(), AbsoluteBasePath).generic_string();
			Entry.Path = i->path().generic_string();
			Entry.ulUncompressedSize = std::uint32_t(std::filesystem::file_size(Entry.Path));
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

			std::ifstream FileIn;
			FileIn.open(Entry.Path, std::ios::binary);
			if(FileIn.fail()) {
				nLog::error("Can't open the file");
				return EXIT_FAILURE;
			}
			vFileContents.resize(Entry.ulUncompressedSize);
			FileIn.read(reinterpret_cast<char*>(vFileContents.data()), Entry.ulUncompressedSize);

			if(isCompressed) {
				if(vPackBuffer.size() < Entry.ulUncompressedSize * 2) {
					vPackBuffer.resize(Entry.ulUncompressedSize * 2);
				}
				auto CompressedSize = (std::uint32_t)compressPack(
					vFileContents.data(), Entry.ulUncompressedSize, vPackBuffer.data()
				);

				vDecompressed.resize(Entry.ulUncompressedSize);
				tCompressUnpacker UnpackState;
				compressUnpackerInit(&UnpackState, vPackBuffer.data(), CompressedSize, Entry.ulUncompressedSize);
				while(true) {
					std::uint8_t ubRead;
					tCompressUnpackResult eResult = compressUnpackerProcess(&UnpackState, &ubRead);
					if(eResult == COMPRESS_UNPACK_RESULT_DONE) {
						break;
					}
					if(eResult == COMPRESS_UNPACK_RESULT_BUSY_WROTE_BYTE) {
						vDecompressed[UnpackState.ulWriteOffset - 1] = ubRead;
					}
				}

				for(std::size_t i = 0; i < Entry.ulUncompressedSize; ++i) {
					if(vDecompressed[i] != vFileContents[i]) {
						nLog::error("mismatch at index {}", i);
						return EXIT_FAILURE;
					}
				}

				if(CompressedSize < vFileContents.size() - 10) {
					Entry.vData = std::vector(&vPackBuffer[0], &vPackBuffer[CompressedSize]);
				}
				else {
					Entry.vData = vFileContents;
				}
			}
			else {
				Entry.vData = vFileContents;
			}

			vEntries.push_back(Entry);
		}
	}
	fmt::print("Discovered {} files\n", vEntries.size());
	if(isCollided) {
		nLog::error("Aborting due to checksum collisions! Report an issue and/or change your file names a bit.");
		return EXIT_FAILURE;
	}

	if(!OrderPath.empty()) {
		fmt::println(FMT_STRING("Reordering files with {}..."), OrderPath);
		std::vector<tPakEntry> vOrderedEntries;
		std::ifstream FileOrder;
		FileOrder.open(OrderPath);
		if(FileOrder.fail()) {
			nLog::error("Can't open the file");
			return EXIT_FAILURE;
		}
		std::string NextPath;
		while(std::getline(FileOrder, NextPath)) {
			auto Found = std::find_if(
				vEntries.begin(), vEntries.end(),
				[&NextPath](tPakEntry &Entry){ return Entry.ShortPath == NextPath;}
			);
			if(Found != vEntries.end()) {
				vOrderedEntries.push_back(*Found);
				vEntries.erase(Found);
			}
			else {
				fmt::println(FMT_STRING("WARN: ordered path '{}' not found or duplicate in added files"), NextPath);
			}
		}

		for(const auto &Unordered: vEntries) {
			fmt::println(FMT_STRING("WARN: unassigned order for file {}"), Unordered.ShortPath);
		}

		vOrderedEntries.insert(vOrderedEntries.end(), vEntries.begin(), vEntries.end());
		vEntries = vOrderedEntries;
	}

	std::uint16_t uwFileCount = std::uint16_t(vEntries.size());
	std::uint16_t uwFileCountBe = nEndian::toBig16(uwFileCount);
	FilePak.write(reinterpret_cast<char*>(&uwFileCountBe), sizeof(uwFileCountBe));
	std::uint32_t ulNextFileOffs = sizeof(uwFileCount) + (uwFileCount * 4 * sizeof(std::uint32_t));
	std::uint16_t i = 0;
	for(const auto &Entry: vEntries) {
		fmt::print(
			"Writing subfile {:4d}: '{}', offset: {}, uncompressed: {}, size: {}, ratio: {:.2f}, checksum: {:08X}...\n",
			i++, Entry.ShortPath, ulNextFileOffs, Entry.ulUncompressedSize,
			Entry.vData.size(), float(Entry.vData.size()) / Entry.ulUncompressedSize * 100, Entry.ulChecksum
		);

		std::uint32_t ulChecksumBe = nEndian::toBig32(Entry.ulChecksum);
		std::uint32_t ulOffsBe = nEndian::toBig32(ulNextFileOffs);
		std::uint32_t ulUncompressedSizeBe = nEndian::toBig32(Entry.ulUncompressedSize);
		std::uint32_t ulDataSizeBe = nEndian::toBig32(std::uint32_t(Entry.vData.size()));

		FilePak.write(reinterpret_cast<char*>(&ulChecksumBe), sizeof(ulChecksumBe));
		FilePak.write(reinterpret_cast<char*>(&ulOffsBe), sizeof(ulOffsBe));
		FilePak.write(reinterpret_cast<const char*>(&ulUncompressedSizeBe), sizeof(ulUncompressedSizeBe));
		FilePak.write(reinterpret_cast<char*>(&ulDataSizeBe), sizeof(ulDataSizeBe));

		ulNextFileOffs += std::uint32_t(Entry.vData.size());
	}

	for(const auto &Entry: vEntries) {
		std::uint32_t UncompressedSizeBe = isCompressed ? nEndian::toBig32(Entry.ulUncompressedSize) : 0;
		FilePak.write(reinterpret_cast<const char *>(Entry.vData.data()), Entry.vData.size());
	}

	fmt::print("All done!\n");
	return EXIT_SUCCESS;
}
