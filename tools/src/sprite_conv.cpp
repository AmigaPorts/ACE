/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Convert a PNG to an ACE hardware-sprite .bm (always 2BPP interleaved) and .plt.
 *
 * Usage:
 *   sprite_conv pal in.png -o out.bm
 *   sprite_conv pal in.png -attached -o lo.bm hi.bm
 *
 * pal       Palette used to resolve PNG colors (gpl/plt/act/pal).
 *           Indices 16..31 are treated as sprite COLOR16..31.
 * -o        Output .bm. With -attached, two paths (lo then hi).
 * -p        Output .plt (default: first .bm with .plt suffix). Writes the
 *           input palette, not a 4-color subset.
 * -aga      Write AGA v2 .plt (default is OCS packed 12-bit).
 * -np       Do not write .plt.
 */

#include "common/logging.h"
#include "common/fs.h"
#include "common/bitmap.h"
#include "common/palette.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

void printUsage(const std::string &szAppName)
{
	using fmt::print;
	print("Usage:\n\t{} pal in.png -o out.bm\n", szAppName);
	print("\t{} pal in.png -attached -o lo.bm hi.bm\n\n", szAppName);
	print("pal\tPalette for PNG colors (gpl/plt/act/pal). COLOR16..31 ok.\n");
	print("-o\tOutput .bm (two paths with -attached)\n");
	print("-p\tOutput .plt (default: first .bm with .plt suffix)\n");
	print("-aga\tWrite AGA v2 .plt (default OCS 12-bit)\n");
	print("-np\tDo not write .plt\n");
	print("-attached\tSplit 16-color attached sprite into lo/hi 2BPP .bm\n");
}

static bool spriteIndex(
	const tPalette &Match, const tRgb &Color, std::uint8_t *pIdx
)
{
	std::int16_t wIdx = Match.getColorIdx(Color);
	if(wIdx < 0) {
		return false;
	}
	auto ub = static_cast<std::uint8_t>(wIdx);
	if(ub >= 16) {
		ub = static_cast<std::uint8_t>(ub - 16);
	}
	*pIdx = ub;
	return true;
}

static bool writeSpriteBm(
	const tChunkyBitmap &Src, const tPalette &Match,
	std::uint8_t ubShift, const std::string &szPath
)
{
	if(Match.m_vColors.size() < 4) {
		nLog::error("Palette needs at least 4 colors to encode 2BPP sprites");
		return false;
	}

	tPalette Pal4;
	Pal4.m_vColors.assign(Match.m_vColors.begin(), Match.m_vColors.begin() + 4);

	tChunkyBitmap Dst(Src.m_uwWidth, Src.m_uwHeight);
	for(std::uint16_t Y = 0; Y < Src.m_uwHeight; ++Y) {
		for(std::uint16_t X = 0; X < Src.m_uwWidth; ++X) {
			auto Color = Src.pixelAt(X, Y);
			std::uint8_t ubSpr;
			if(!spriteIndex(Match, Color, &ubSpr)) {
				nLog::error(
					"Unexpected color #{:02X}{:02X}{:02X} at {},{}",
					Color.ubR, Color.ubG, Color.ubB, X, Y
				);
				return false;
			}
			Dst.pixelAt(X, Y) = Pal4.m_vColors[(ubSpr >> ubShift) & 3];
		}
	}

	auto Planar = tPlanarBitmap(Dst, Pal4);
	if(!Planar.m_uwWidth) {
		return false;
	}
	if(!Planar.toBm(szPath, true)) {
		nLog::error("Couldn't write '{}'", szPath);
		return false;
	}
	return true;
}

int main(int lArgCount, const char *pArgs[])
{
	if(lArgCount < 3) {
		nLog::error("Too few arguments");
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	std::string szPalette = pArgs[1];
	std::string szInput = pArgs[2];
	std::vector<std::string> vBm;
	std::string szPlt;
	bool isAttached = false;
	bool isAgaPlt = false;
	bool isWritePlt = true;

	for(int i = 3; i < lArgCount; ++i) {
		if(std::strcmp(pArgs[i], "-attached") == 0) {
			isAttached = true;
		}
		else if(std::strcmp(pArgs[i], "-aga") == 0) {
			isAgaPlt = true;
		}
		else if(std::strcmp(pArgs[i], "-np") == 0) {
			isWritePlt = false;
		}
		else if(std::strcmp(pArgs[i], "-o") == 0) {
			while(i + 1 < lArgCount && pArgs[i + 1][0] != '-') {
				vBm.push_back(pArgs[++i]);
			}
			if(vBm.empty()) {
				nLog::error("-o needs an output path");
				return EXIT_FAILURE;
			}
		}
		else if(std::strcmp(pArgs[i], "-p") == 0) {
			if(i + 1 >= lArgCount) {
				nLog::error("-p needs a .plt path");
				return EXIT_FAILURE;
			}
			szPlt = pArgs[++i];
		}
		else {
			nLog::error("Unknown arg '{}'", pArgs[i]);
			printUsage(pArgs[0]);
			return EXIT_FAILURE;
		}
	}

	if(isAttached) {
		if(vBm.size() != 2) {
			nLog::error("-attached needs -o lo.bm hi.bm");
			printUsage(pArgs[0]);
			return EXIT_FAILURE;
		}
	}
	else if(vBm.empty()) {
		vBm.push_back(nFs::removeExt(szInput) + ".bm");
	}
	else if(vBm.size() != 1) {
		nLog::error("Expected a single -o .bm path");
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	const std::string &szBm = vBm[0];
	const std::string szBmHi = isAttached ? vBm[1] : "";

	if(isWritePlt && szPlt.empty()) {
		szPlt = nFs::removeExt(szBm) + ".plt";
	}

	auto Match = tPalette::fromFile(szPalette);
	if(!Match.isValid() || Match.m_vColors.size() < 4) {
		nLog::error("Palette '{}' needs at least 4 colors", szPalette);
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

	if(isAttached) {
		fmt::print("sprite_conv: {} → {} + {}\n", szInput, szBm, szBmHi);
		if(!writeSpriteBm(Img, Match, 0, szBm) || !writeSpriteBm(Img, Match, 2, szBmHi)) {
			return EXIT_FAILURE;
		}
	}
	else {
		fmt::print("sprite_conv: {} → {}\n", szInput, szBm);
		if(!writeSpriteBm(Img, Match, 0, szBm)) {
			return EXIT_FAILURE;
		}
	}

	if(isWritePlt) {
		try {
			if(!Match.toPlt(szPlt, !isAgaPlt)) {
				nLog::error("Couldn't write '{}'", szPlt);
				return EXIT_FAILURE;
			}
		}
		catch(const std::exception &Exc) {
			nLog::error("{}", Exc.what());
			return EXIT_FAILURE;
		}
		fmt::print("sprite_conv: palette → {}\n", szPlt);
	}

	fmt::print("All done!\n");
	return EXIT_SUCCESS;
}
