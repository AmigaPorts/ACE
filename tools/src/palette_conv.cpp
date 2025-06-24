/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "common/logging.h"
#include "common/fs.h"
#include "common/palette.h"
#include "common/bitmap.h"

void printUsage(const std::string& szAppName) {
	using fmt::print;
	fmt::print("Usage:\n\t{} -i <input_file> -o <output_file> [-f <format>] [options]\n", szAppName);
	print("\nRequired arguments:\n");
	print("  -i, --input <file>    Input palette file\n");
	print("  -o, --output <file>   Output palette file\n");
	print("\nOptional arguments:\n");
	print("  -f, --format <fmt>    Output format (default: plt)\n");
	print("\nSupported formats:\n");
	print("  gpl    GIMP Palette\n");
	print("  act    Adobe Color Table\n");
	print("  pal    ProMotion palette\n");
	print("  plt    ACE palette\n");
	print("  png    Palette preview\n");
	print("\nOptions:\n");
	print("  -a, --aga             Use AGA palette mode (default: OCS/ECS)\n");
	print("  -n, --no-strict       Allow non-OCS colors in OCS mode (default: strict)\n");
	print("  -h, --help            Show this help message\n");
	print("\nExamples:\n");
	print("  {} -i palette.gpl -o palette.plt\n", szAppName);
	print("  {} -i colors.act -o preview.png -f png --aga\n", szAppName);
	print("  {} -i palette.gpl -o palette.plt --no-strict\n", szAppName);
}

int main(int lArgCount, const char* pArgs[])
{
	std::string szPathIn;
	std::string szPathOut;
	std::string szFormat;
	bool isAGA = false;
	bool isForceOCS = true;

	// Parse command line arguments
	for (int i = 1; i < lArgCount; ++i) {
		std::string arg = pArgs[i];
		
		if (arg == "-h" || arg == "--help") {
			printUsage(pArgs[0]);
			return EXIT_SUCCESS;
		}
		else if (arg == "-i" || arg == "--input") {
			if (i + 1 >= lArgCount) {
				nLog::error("Missing value for input file argument");
				printUsage(pArgs[0]);
				return EXIT_FAILURE;
			}
			szPathIn = pArgs[++i];
		}
		else if (arg == "-o" || arg == "--output") {
			if (i + 1 >= lArgCount) {
				nLog::error("Missing value for output file argument");
				printUsage(pArgs[0]);
				return EXIT_FAILURE;
			}
			szPathOut = pArgs[++i];
		}
		else if (arg == "-f" || arg == "--format") {
			if (i + 1 >= lArgCount) {
				nLog::error("Missing value for format argument");
				printUsage(pArgs[0]);
				return EXIT_FAILURE;
			}
			szFormat = pArgs[++i];
		}
		else if (arg == "-a" || arg == "--aga") {
			isAGA = true;
		}
		else if (arg == "-n" || arg == "--no-strict") {
			isForceOCS = false;
		}
		else {
			nLog::error("Unknown argument: {}", arg);
			printUsage(pArgs[0]);
			return EXIT_FAILURE;
		}
	}

	// Validate required arguments
	if (szPathIn.empty()) {
		nLog::error("Input file is required");
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}
	if (szPathOut.empty()) {
		nLog::error("Output file is required");
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}
	if (szFormat.empty()) {
		szFormat = "plt";  // Default to ACE palette format
	}

	// Validate format
	if (szFormat != "gpl" && szFormat != "act" && szFormat != "pal" && 
		szFormat != "plt" && szFormat != "png") {
		nLog::error("Unsupported format: '{}'. Supported formats: gpl, act, pal, plt, png", szFormat);
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	// Load input palette
	auto Palette = tPalette::fromFile(szPathIn);
	if (Palette.m_vColors.empty()) {
		nLog::error("Invalid input path or palette is empty: '{}'", szPathIn);
		return EXIT_FAILURE;
	}
	fmt::print("Loaded palette: '{}' ({})\n", szPathIn, isAGA ? "AGA mode" : "OCS/ECS mode");

	// Generate output palette
	bool isOk = false;
	try {
		if (szFormat == "gpl") {
			isOk = Palette.toGpl(szPathOut);
		}
		else if (szFormat == "act") {
			isOk = Palette.toAct(szPathOut);
		}
		else if (szFormat == "pal") {
			isOk = Palette.toPromotionPal(szPathOut);
		}
		else if (szFormat == "plt") {
			isOk = Palette.toPlt(szPathOut, isAGA, isForceOCS);
		}
		else if (szFormat == "png") {
			auto ColorCount = Palette.m_vColors.size();
			tChunkyBitmap PltPreview(ColorCount * 32, 16);

			for (uint8_t i = 0; i < ColorCount; ++i) {
				const auto& Color = Palette.m_vColors[i];
				PltPreview.fillRect(i * 32, 0, 32, 16, Color);
			}
			isOk = PltPreview.toPng(szPathOut);
		}
	}
	catch (const std::exception& Exc) {
		nLog::error("Writing palette failed: {}", Exc.what());
		return EXIT_FAILURE;
	}

	if (!isOk) {
		nLog::error("Couldn't write to '{}'", szPathOut);
		return EXIT_FAILURE;
	}
	fmt::print("Generated {} palette: '{}'\n", szFormat.c_str(), szPathOut);

	return EXIT_SUCCESS;
}
