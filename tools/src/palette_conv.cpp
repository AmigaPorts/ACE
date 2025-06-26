/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "common/logging.h"
#include "common/fs.h"
#include "common/palette.h"
#include "common/bitmap.h"

void printUsage(const std::string &szAppName) {
	using fmt::print;
	fmt::print("Usage:\n\t{} inPath.ext [outPath.ext] [extraOpts]\n", szAppName);
	print("\ninPath\t- path to supported input palette file\n");
	print("outPath\t- path to output palette file\n");
	print("ext\t- one of the following:\n");
	print("\tgpl\tGIMP Palette (default)\n");
	print("\tact\tAdobe Color Table\n");
	print("\tpal\tProMotion palette\n");
	print("\tplt\tACE palette\n");
	print("\tpng\tPalette preview\n");
	print("extraOpts:\n");
	print("\t-cc\tConvert colors. Truncate non-OCS colors to OCS precision if necessary\n");
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
	bool isForceOcs = true;

	// Search for optional args
	for(int ArgIndex = 2; ArgIndex < lArgCount; ++ArgIndex) {
		const char *const pArg = pArgs[ArgIndex];
		if(pArg == std::string("-cc")) {
			isForceOcs = false;
		}
		else {
			szPathOut = pArg;
		}
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
			isOk = Palette.toPlt(szPathOut, isForceOcs);
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
