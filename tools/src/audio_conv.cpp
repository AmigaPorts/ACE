#include "common/logging.h"
#include "common/fs.h"
#include "common/sfx.h"
#include "common/wav.h"

void printUsage(const std::string &szAppName) {
	using fmt::print;
	print("Usage:\n\t{} inPath [extraOpts]\n\n", szAppName);
	print("inPath\t- path to supported file format\n");
	print("extraOpts:\n");
	print("\t-o outPath\tSpecify output file path. If ommited, it will perform default conversion\n");
	print("\t-strict\tTreat warinings as errors (recommended)\n");
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
	std::string szInput(pArgs[1]);
	std::string szOutput("");
	for(uint32_t i = 2; i < lArgCount; ++i) {
		if(pArgs[i] == std::string("-o") && i < lArgCount -1) {
			szOutput = pArgs[++i];
		}
		else if(pArgs[i] == std::string("-strict")) {
			isStrict = true;
		}
		else {
			nLog::error("Unknown arg or missing value: '{}'", pArgs[i]);
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
