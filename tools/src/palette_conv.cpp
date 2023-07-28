/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "common/logging.h"
#include "common/fs.h"
#include "common/palette.h"
#include "common/bitmap.h"

void printUsage(const std::string &szAppName) {
	using fmt::print;
	fmt::print("Usage:\n\t{} inPath.ext [outPath.ext]\n", szAppName);
	print("\ninPath\t- path to supported input palette file\n");
	print("outPath\t- path to output palette file\n");
	print("ext\t- one of the following:\n");
	print("\tgpl\tGIMP Palette\n");
	print("\tact\tAdobe Color Table\n");
	print("\tpal\tProMotion palette\n");
	print("\tplt\tACE palette (default)\n");
	print("\tpng\tPalette preview\n");
}

int main(int lArgCount, const char *pArgs[])
{
	const std::uint8_t ubMandatoryArgCnt = 1;
	// Mandatory args
	if(lArgCount - 1 < ubMandatoryArgCnt) {
		nLog::error("Too few arguments, expected {}", ubMandatoryArgCnt);
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	std::string szPathIn = pArgs[1];

	// Optional args' default values
	std::string szPathOut = nFs::removeExt(szPathIn) + ".gpl";

	// Search for optional args
	if(lArgCount - 1 > 1) {
		szPathOut = pArgs[2];
	}

	// Load input palette
	auto Palette = tPalette::fromFile(szPathIn);
	if(Palette.m_vColors.empty()) {
		nLog::error("Invalid input path or palette is empty: '{}'", szPathIn);
		return 1;
	}
	fmt::print("Loaded palette: '{}'\n", szPathIn);

	// Generate output palette
	std::string szExtOut = nFs::getExt(szPathOut);
	bool isOk = false;
	if(nFs::getExt(szPathIn) == szExtOut) {
		nLog::error("Input and output extensions are the same");
		return EXIT_FAILURE;
	}

	try {
		if(szExtOut == "gpl") {
			isOk = Palette.toGpl(szPathOut);
		}
		else if(szExtOut == "act") {
			isOk = Palette.toAct(szPathOut);
		}
		else if(szExtOut == "pal") {
			isOk = Palette.toPromotionPal(szPathOut);
		}
		else if(szExtOut == "plt") {
			isOk = Palette.toPlt(szPathOut, true);
		}
		else if(szExtOut == "png") {
			auto ColorCount = Palette.m_vColors.size();
			tChunkyBitmap PltPreview(ColorCount * 32, 16);
			for(std::uint8_t i = 0; i < ColorCount; ++i) {
				const auto &Color = Palette.m_vColors[i];
				PltPreview.fillRect(i * 32, 0, 32, 16, Color);
				isOk = PltPreview.toPng(szPathOut);
			}
		}
		else {
			nLog::error("unsupported output extension: '{}'", szExtOut);
			printUsage(pArgs[0]);
			return EXIT_FAILURE;
		}
	}
	catch(const std::exception &Exc) {
		nLog::error("Writing palette failed: {}", Exc.what());
	}

	if(!isOk) {
		nLog::error("Couldn't write to '{}'", szPathOut);
		return EXIT_FAILURE;
	}
	fmt::print("Generated palette: '{}'\n", szPathOut);

	return EXIT_SUCCESS;
}
