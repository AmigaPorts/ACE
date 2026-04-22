/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "common/logging.h"
#include "common/fs.h"
#include "common/palette.h"
#include "common/bitmap.h"
#include <cstring>
#include <string>
#include <vector>

void printUsage(const std::string& szAppName) {
	using fmt::print;
	fmt::print("Usage:\n\t{} [options] inPath.ext [outPath.ext]\n", szAppName);
	print("\nOptions:\n");
	print("\t--ocs\t\tWrite .plt v2 with ECS/OCS packed 12-bit entries (default)\n");
	print("\t--aga\t\tWrite .plt v2 with AGA colour entries\n");
	print("\t--legacy\tWrite old single-byte count .plt (not v2)\n");
	print("\ninPath\t- path to supported input palette file\n");
	print("outPath\t- path to output palette file\n");
	print("ext\t- one of the following:\n");
	print("\tgpl\tGIMP Palette\n");
	print("\tact\tAdobe Color Table\n");
	print("\tpal\tProMotion palette\n");
	print("\tplt\tACE palette (default)\n");
	print("\tpng\tPalette preview\n");
}

int main(int lArgCount, const char* pArgs[])
{
	bool isLegacyPlt = false;
	bool isForceOcs = true;
	bool isExplicitMode = false;

	std::vector<const char*> Positionals;

	for (int i = 1; i < lArgCount; ++i) {
		if (std::strcmp(pArgs[i], "--legacy") == 0) {
			isLegacyPlt = true;
		}
		else if (std::strcmp(pArgs[i], "--ocs") == 0) {
			isForceOcs = true;
			isExplicitMode = true;
		}
		else if (std::strcmp(pArgs[i], "--aga") == 0) {
			isForceOcs = false;
			isExplicitMode = true;
		}
		else if (pArgs[i][0] == '-') {
			nLog::error("Unknown option: '{}'", pArgs[i]);
			printUsage(pArgs[0]);
			return EXIT_FAILURE;
		}
		else {
			Positionals.push_back(pArgs[i]);
		}
	}

	if (Positionals.empty()) {
		nLog::error("Too few arguments, expected input path");
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	std::string szPathIn = Positionals[0];
	std::string szPathOut = nFs::removeExt(szPathIn) + ".gpl";

	if (Positionals.size() > 1) {
		szPathOut = Positionals[1];
	}

	// Load input palette
	auto Palette = tPalette::fromFile(szPathIn);
	if (Palette.m_vColors.empty()) {
		nLog::error("Invalid input path or palette is empty: '{}'", szPathIn);
		return 1;
	}
	fmt::print("Loaded palette: '{}'\n", szPathIn);

	// Generate output palette
	std::string szExtOut = nFs::getExt(szPathOut);
	bool isOk = false;
	if (nFs::getExt(szPathIn) == szExtOut) {
		nLog::error("Input and output extensions are the same");
		return EXIT_FAILURE;
	}

	try {
		if (szExtOut == "gpl") {
			isOk = Palette.toGpl(szPathOut);
		}
		else if (szExtOut == "act") {
			isOk = Palette.toAct(szPathOut);
		}
		else if (szExtOut == "pal") {
			isOk = Palette.toPromotionPal(szPathOut);
		}
		else if (szExtOut == "plt") {
			isOk = Palette.toPlt(szPathOut, isForceOcs, isLegacyPlt);
		}
		else if (szExtOut == "png") {
			auto ColorCount = Palette.m_vColors.size();
			tChunkyBitmap PltPreview(ColorCount * 32, 16);

			for (uint8_t i = 0; i < ColorCount; ++i) {
				const auto& Color = Palette.m_vColors[i];

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
	catch (const std::exception& Exc) {
		nLog::error("Writing palette failed: {}", Exc.what());
	}

	if (!isOk) {
		nLog::error("Couldn't write to '{}'", szPathOut);
		return EXIT_FAILURE;
	}
	fmt::print("Generated palette: '{}'\n", szPathOut);

	return EXIT_SUCCESS;
}
