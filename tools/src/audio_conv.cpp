#include "common/logging.h"
#include "common/fs.h"
#include "common/sfx.h"
#include "common/wav.h"

void printUsage(const std::string &szAppName) {
	using fmt::print;
	print("Usage:\n\t{} inPath [extraOpts]\n\n", szAppName);
	print("Required arguments:\n");
	print("\tinPath      Path to supported file format\n");
	print("Extra options:\n");
	print("\t-o outPath  Specify output file path. If ommited, it will perform default conversion\n");
	print("\t-d N        Specify amplitude division. Useful for some audio-mixing libraries\n");
	print("\t-fd N       Ensure that sound effect fits max amplitude divided by specified factor. Useful for some audio-mixing libraries\n");
	print("\t-strict     Treat warinings as errors (recommended)\n");
	print("\t-n          Normalize audio files\n");
	print("Default conversions:\n");
	print("\t.wav -> .sfx\n");
	print("\t.sfx -> .wav\n");
}

int main(int lArgCount, const char *pArgs[]) {
	uint8_t ubMandatoryArgCnt = 2;

	if(lArgCount < ubMandatoryArgCnt) {
		nLog::error("Too few arguments, expected {}", ubMandatoryArgCnt);
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	bool isStrict = false;
	bool isNormalizing = false;
	uint8_t ubDivisor = 1;
	uint8_t ubFitDivisor = 1;
	std::string szInput(pArgs[1]);
	std::string szOutput("");
	for(auto ArgIndex = 2; ArgIndex < lArgCount; ++ArgIndex) {
		if(pArgs[ArgIndex] == std::string("-o") && ArgIndex < lArgCount -1) {
			szOutput = pArgs[++ArgIndex];
		}
		else if(pArgs[ArgIndex] == std::string("-d") && ArgIndex < lArgCount -1) {
			ubDivisor = uint8_t(std::stoul(pArgs[++ArgIndex]));
			if(ubDivisor == 0) {
				nLog::error("Illegal -d value: '{}'", ubDivisor);
				return EXIT_FAILURE;
			}
		}
		else if(pArgs[ArgIndex] == std::string("-fd") && ArgIndex < lArgCount -1) {
			ubFitDivisor = uint8_t(std::stoul(pArgs[++ArgIndex]));
			if(ubFitDivisor == 0) {
				nLog::error("Illegal -fd value: '{}'", ubFitDivisor);
				return EXIT_FAILURE;
			}
		}
		else if(pArgs[ArgIndex] == std::string("-n")) {
			isNormalizing = true;
		}
		else if(pArgs[ArgIndex] == std::string("-strict")) {
			isStrict = true;
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
		szOutput = nFs::trimExt(szInput);
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

	if(isNormalizing) {
		In.normalize();
	}
	if(ubDivisor != 1) {
		In.divideAmplitude(ubDivisor);
	}

	int8_t bMaxAmplitude = std::numeric_limits<int8_t>::max() / ubFitDivisor;
	if(!In.isFittingMaxAmplitude(bMaxAmplitude)) {
		nLog::error(
			"Sound effect doesn't fit the amplitude divisor {}, max amplitude: {}",
			ubFitDivisor, bMaxAmplitude
		);
	}

	// Save to output
	fmt::print("Writing to {}...\n", szOutput);
	if(szOutExt == "sfx") {
		In.toSfx(szOutput);
	}
	else {
		nLog::error("Output file type not supported: {}", szInExt);
		return EXIT_FAILURE;
	}

	fmt::print("All done!\n");
}
