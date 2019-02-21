/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "common/bitmap.h"
#include "common/logging.h"
#include "common/parse.h"

void printUsage(const std::string &szAppName)
{
	using fmt::print;
	print("Usage:\n\t{} in.png x y width height out.png\n", szAppName);
	print("\nCurrently ony PNG is supported, sorry!\n");
}

int main(int lArgCount, char *pArgs[])
{
	if(lArgCount - 1 != 6) {
		nLog::error(
			"Invalid number of arguments, got {}, expected 6", lArgCount - 1
		);
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	std::string szSrcPath = pArgs[1];
	auto Src = tChunkyBitmap::fromPng(szSrcPath);
	if(!Src.m_uwHeight) {
		nLog::error("Couldn't open image '{}'", szSrcPath);
		return EXIT_FAILURE;
	}

	int32_t lSrcX, lSrcY, lWidth, lHeight;
	if(
		!nParse::toInt32(pArgs[2], "source X", lSrcX) || lSrcX <= 0 ||
		!nParse::toInt32(pArgs[3], "source Y", lSrcY) || lSrcY <= 0 ||
		!nParse::toInt32(pArgs[4], "width", lWidth) || lWidth <= 0 ||
		!nParse::toInt32(pArgs[5], "height", lHeight) || lHeight <= 0
	) {
		return EXIT_FAILURE;
	}
	std::string szDstPath = pArgs[6];

	tChunkyBitmap Dst(lWidth, lHeight);
	bool isOk = Src.copyRect(lSrcX, lSrcY, Dst, 0, 0, lWidth, lHeight);
	if(!isOk) {
		nLog::error("Couldn't extract given rectangle from source image");
		return EXIT_FAILURE;
	}

	isOk = Dst.toPng(szDstPath);
	if(!isOk) {
		nLog::error("Couldn't write to '{}'", szDstPath);
		return EXIT_FAILURE;
	}

	fmt::print("All done!\n");
	return EXIT_SUCCESS;
}
