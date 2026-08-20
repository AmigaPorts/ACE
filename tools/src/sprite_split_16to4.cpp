/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Split a 16-color attached-sprite PNG into two 4-color (2BPP) plane PNGs.
 * lo = bits 0-1, hi = bits 2-3. Outputs use match.plt colors 0..3.
 *
 * Usage: sprite_split_16to4 match.plt in.png -o lo.png hi.png
 */

#include "common/logging.h"
#include "common/bitmap.h"
#include "common/palette.h"

void printUsage(const std::string &szAppName)
{
	using fmt::print;
	print("Usage:\n\t{} match.plt in.png -o lo.png hi.png\n", szAppName);
}

static bool writePlanePng(
	const tChunkyBitmap &Src, const tPalette &Match,
	std::uint8_t ubShift, const std::string &szPath
)
{
	tChunkyBitmap Dst(Src.m_uwWidth, Src.m_uwHeight);
	for(std::uint16_t Y = 0; Y < Src.m_uwHeight; ++Y) {
		for(std::uint16_t X = 0; X < Src.m_uwWidth; ++X) {
			auto Color = Src.pixelAt(X, Y);
			std::int16_t wIdx = Match.getColorIdx(Color);
			if(wIdx < 0) {
				nLog::error(
					"Unexpected color #{:02X}{:02X}{:02X} at {},{}",
					Color.ubR, Color.ubG, Color.ubB, X, Y
				);
				return false;
			}
			std::uint8_t ubSpr = static_cast<std::uint8_t>(wIdx);
			if(ubSpr >= 16) {
				ubSpr = static_cast<std::uint8_t>(ubSpr - 16);
			}
			std::uint8_t ubOut = static_cast<std::uint8_t>((ubSpr >> ubShift) & 3);
			Dst.pixelAt(X, Y) = Match.m_vColors[ubOut];
		}
	}
	if(!Dst.toPng(szPath)) {
		nLog::error("Couldn't write '{}'", szPath);
		return false;
	}
	return true;
}

int main(int lArgCount, const char *pArgs[])
{
	if(lArgCount < 6 || std::string(pArgs[3]) != "-o") {
		nLog::error("Expected match.plt in.png -o lo.png hi.png");
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	std::string szMatch = pArgs[1];
	std::string szInput = pArgs[2];
	std::string szLo = pArgs[4];
	std::string szHi = pArgs[5];

	auto Match = tPalette::fromFile(szMatch);
	if(!Match.isValid() || Match.m_vColors.size() < 4) {
		nLog::error("Match palette '{}' needs at least 4 colors", szMatch);
		return EXIT_FAILURE;
	}

	auto Img = tChunkyBitmap::fromPng(szInput);
	if(!Img.m_uwHeight) {
		nLog::error("Couldn't open '{}'", szInput);
		return EXIT_FAILURE;
	}
	if(Img.m_uwWidth & 0xF) {
		nLog::error("Width must be divisible by 16, got {}", Img.m_uwWidth);
		return EXIT_FAILURE;
	}

	fmt::print("sprite_split_16to4: {} → lo={} hi={}\n", szInput, szLo, szHi);
	if(!writePlanePng(Img, Match, 0, szLo)) {
		return EXIT_FAILURE;
	}
	if(!writePlanePng(Img, Match, 2, szHi)) {
		return EXIT_FAILURE;
	}
	fmt::print("All done!\n");
	return EXIT_SUCCESS;
}
