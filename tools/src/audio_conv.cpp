/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>
#include "common/logging.h"
#include "common/fs.h"
#include "common/sfx.h"
#include "common/wav.h"
#include "common/math.h"

void printUsage(const std::string &szAppName) {
	using fmt::print;
	print("Usage:\n\t{} inPath [extraOpts]\n\n", szAppName);
	print("Required arguments:\n");
	print("\tinPath      Path to supported file format\n");
	print("Extra options:\n");
	print("\t-o outPath  Specify output file path. If ommited, it will perform default conversion\n");
	print("\t-c          Enable compression for output file\n");
	print("\t-d N        Specify amplitude division. Useful for some audio-mixing libraries\n");
	print("\t-cd N       Check if sound effect fits max amplitude divided by specified factor and raise error otherwise. Useful for some audio-mixing libraries\n");
	print("\t-strict     Treat warinings as errors (recommended)\n");
	print("\t-n          Normalize audio files\n");
	print("\t-fpt        Enforce ptplayer-friendly mode: adds empty sample at the beginning, if missing\n");
	print("\t-fpad N     Force given byte-padding\n");
	print("\t-sa N       Split sample after every given number of bytes, or kbytes if value ends with k\n");
	print("Default conversions:\n");
	print("\t.wav -> .sfx\n");
	print("\t.sfx -> .wav\n");
}

int main(int lArgCount, const char *pArgs[]) {
	using namespace std::string_view_literals;

	std::uint8_t ubMandatoryArgCnt = 2;

	if(lArgCount - 1 < ubMandatoryArgCnt) {
		nLog::error("Too few arguments, expected {}", ubMandatoryArgCnt);
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	bool isCompressed = false;
	bool isStrict = false;
	bool isNormalizing = false;
	std::uint8_t ubDivisor = 1;
	std::uint8_t ubFitDivisor = 1;
	std::string szInput(pArgs[1]);
	std::string szOutput;
	bool isForcePt = false;
	std::optional<uint8_t> oForcePad;
	std::optional<uint32_t> oSplitAfter;
	for(auto ArgIndex = 2; ArgIndex < lArgCount; ++ArgIndex) {
		std::string_view Arg = pArgs[ArgIndex];
		if(Arg == "-o"sv && ArgIndex < lArgCount -1) {
			szOutput = pArgs[++ArgIndex];
		}
		else if(Arg == "-c"sv) {
			isCompressed = true;
		}
		else if(Arg == "-d"sv && ArgIndex < lArgCount -1) {
			ubDivisor = uint8_t(std::stoul(pArgs[++ArgIndex]));
			if(ubDivisor == 0) {
				nLog::error("Illegal -d value: '{}'", ubDivisor);
				return EXIT_FAILURE;
			}
		}
		else if(Arg == "-cd"sv && ArgIndex < lArgCount -1) {
			ubFitDivisor = uint8_t(std::stoul(pArgs[++ArgIndex]));
			if(ubFitDivisor == 0) {
				nLog::error("Illegal -cd value: '{}'", ubFitDivisor);
				return EXIT_FAILURE;
			}
		}
		else if(Arg == "-n"sv) {
			isNormalizing = true;
		}
		else if(Arg == "-strict"sv) {
			isStrict = true;
		}
		else if(Arg == "-fpt"sv) {
			isForcePt = true;
			oForcePad = std::min<uint8_t>(oForcePad.value_or(0), 2);
		}
		else if(Arg == "-fpad"sv && ArgIndex < lArgCount -1) {
			oForcePad = uint8_t(std::stoul(pArgs[++ArgIndex]));
			if(oForcePad.value() < 1 || 4 < oForcePad.value()) {
				nLog::error("Illegal -fpad value: '{}'", oForcePad.value());
				return EXIT_FAILURE;
			}
		}
		else if(Arg == "-sa"sv && ArgIndex < lArgCount -1) {
			std::string_view Value(pArgs[++ArgIndex]);
			std::size_t CharsParsed = 0;
			auto ValueEnd = Value.end();
			oSplitAfter = uint32_t(std::stoul(Value.data(), &CharsParsed));
			if(CharsParsed < Value.size() && Value[CharsParsed] == 'k') {
				oSplitAfter = oSplitAfter.value() * 1024;
			}
		}
		else {
			nLog::error("Unknown arg or missing value: '{}'", pArgs[ArgIndex]);
			printUsage(pArgs[0]);
			return EXIT_FAILURE;
		}
	}

	// Determine output path and extension
	std::string szInExt = nFs::getExt(szInput);
	if(szInExt != "wav") {
		nLog::error("Input file type not supported: {}", szInExt);
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	if(szOutput.empty()) {
		szOutput = nFs::removeExt(szInput);
		if(szInExt == "wav") {
			szOutput += ".sfx";
		}
	}
	std::string szOutExt = nFs::getExt(szOutput);

	// Load input
	tSfx In;
	if(szInExt == "wav") {
		tWav Wav(szInput);
		if(Wav.getData().empty()) {
			nLog::error("No data read from WAV file");
			return EXIT_FAILURE;
		}
		In = tSfx(Wav, isStrict);
		if(In.isEmpty()) {
			nLog::error("No valid data to convert");
			return EXIT_FAILURE;
		}
	}
	else {
		nLog::error("Input file type not supported: {}", szInExt);
		return EXIT_FAILURE;
	}

	// Ptplayer-like requirements
	if(isForcePt) {
		In.enforceEmptyFirstWord();
	}

	// Mixer-like requirements
	if(isNormalizing) {
		In.normalize();
	}
	if(ubDivisor != 1) {
		In.divideAmplitude(ubDivisor);
	}

	std::int8_t bMaxAmplitude = std::numeric_limits<int8_t>::max() / ubFitDivisor;
	if(!In.isFittingMaxAmplitude(bMaxAmplitude)) {
		nLog::error(
			"Sound effect doesn't fit the amplitude divisor {}, max amplitude: {}",
			ubFitDivisor, bMaxAmplitude
		);
	}

	if(oForcePad.has_value()) {
		In.padContents(oForcePad.value());
	}

	if(oSplitAfter.has_value()) {
		auto PartCount = (In.getLength() + oSplitAfter.value() - 1) / oSplitAfter.value();
		fmt::print("Splitting to {} parts, {} bytes each\n", PartCount, oSplitAfter.value());
		std::uint8_t ubPart = 0;
		tSfx SfxRemaining;
		auto BaseOutputPath = nFs::removeExt(szOutput);
		do {
			SfxRemaining = In.splitAfter(oSplitAfter.value());
			auto PartOutPath = fmt::format(FMT_STRING("{}_{}.{}"), BaseOutputPath, ubPart, szOutExt);
			fmt::print("Writing to {}\n", PartOutPath);
			if(szOutExt == "sfx") {
				if(!In.toSfx(PartOutPath, isCompressed)) {
					return EXIT_FAILURE;
				}
			}
			else {
				nLog::error("Output file type not supported: {}", szInExt);
				return EXIT_FAILURE;
			}

			In = SfxRemaining;
			++ubPart;
		} while(!SfxRemaining.isEmpty());
	}
	else {
		// Save to output
		fmt::print("Writing to {}...\n", szOutput);
		if(szOutExt == "sfx") {
			if(!In.toSfx(szOutput, isCompressed)) {
				return EXIT_FAILURE;
			}
		}
		else {
			nLog::error("Output file type not supported: {}", szInExt);
			return EXIT_FAILURE;
		}
	}


	fmt::print("All done!\n");
}
