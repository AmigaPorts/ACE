/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <fmt/format.h>
#include "common/fs.h"
#include "common/palette.h"

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
}

int main(int lArgCount, const char *pArgs[])
{
	const uint8_t ubMandatoryArgCnt = 1;
	// Mandatory args
	if(lArgCount - 1 < ubMandatoryArgCnt) {
		fmt::print("ERR: Too few arguments, expected {}\n", ubMandatoryArgCnt);
		printUsage(pArgs[0]);
		return 1;
	}

	std::string szPathIn = pArgs[1];

	// Optional args' default values
	std::string szPathOut = nFs::trimExt(szPathIn) + ".gpl";

	// Search for optional args
	if(lArgCount - 1 > 1) {
		szPathOut = szPathIn;
	}

	// Load input palette
	std::string szExtIn = nFs::getExt(szPathIn);
	tPalette Palette;
	if(szExtIn == "gpl") {
		Palette = tPalette::fromGpl(szPathIn);
	}
	else if(szExtIn == "act") {
		// Looks like it's same as promotion
		Palette = tPalette::fromAct(szPathIn);
	}
	else if(szExtIn == "pal") {
		Palette = tPalette::fromPromotionPal(szPathIn);
	}
	else if(szExtIn == "plt") {
		Palette = tPalette::fromPlt(szPathIn);
	}
	else {
		fmt::print("ERR: unsupported input extension: '{}'\n", szExtIn);
		printUsage(pArgs[0]);
		return 1;
	}

	if(Palette.m_vColors.empty()) {
		fmt::print("ERR: Invalid input path or palette is empty: '{}'\n", szPathIn);
		return 1;
	}
	fmt::print("Loaded palette: '{}'\n", szPathIn);

	// Generate output palette
	std::string szExtOut = nFs::getExt(szPathOut);
	bool isOk = false;
	if(szExtIn == szExtOut) {
		fmt::print("ERR: Input and output extensions are the same\n");
		return 1;
	}
	else if(szExtOut == "gpl") {
		isOk = Palette.toGpl(szPathOut);
	}
	else if(szExtOut == "act") {
		isOk = Palette.toAct(szPathOut);
	}
	else if(szExtOut == "pal") {
		isOk = Palette.toPromotionPal(szPathOut);
	}
	else if(szExtOut == "plt") {
		Palette.toPlt(szPathOut);
	}
	else {
		fmt::print("ERR: unsupported output extension: '{}'\n", szExtOut);
		printUsage(pArgs[0]);
		return 1;
	}
	fmt::print("Generated palette: '{}'\n", szPathIn);

	return 0;
}
