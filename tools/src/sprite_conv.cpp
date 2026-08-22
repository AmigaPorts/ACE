/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "common/logging.h"
#include "common/fs.h"
#include "common/bitmap.h"
#include "common/palette.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

void printUsage(const std::string &szAppName)
{
	using fmt::print;
	print("Usage:\n\t{} pal in.png -o out.bm\n", szAppName);
	print("\t{} pal in.png -attached -o lo.bm hi.bm\n\n", szAppName);
	print("pal\tPalette for PNG colors (gpl/plt/act/pal). COLOR16..31 ok.\n");
	print("-o\tOutput .bm (two paths with -attached)\n");
	print("-attached\tSplit 16-color attached sprite into lo/hi 2BPP .bm\n");
	print("-pad\tAdd empty header/footer rows for sprite control words\n");
}

static bool writeSpriteBm(
	const tChunkyBitmap &SourceBitmap, const tPalette &Palette,
	std::uint8_t ubShift, const std::string &szPath
)
{
	auto SpriteChunky = tChunkyBitmap::toSpriteSubBitmap(SourceBitmap, Palette, ubShift);
	if(!SpriteChunky.m_uwWidth) {
		return false;
	}

	tPalette Pal4;
	Pal4.m_vColors.assign(Palette.m_vColors.begin(), Palette.m_vColors.begin() + 4);
	auto Planar = tPlanarBitmap(SpriteChunky, Pal4);
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

	std::string szPalettePath = pArgs[1];
	std::string szInputPath = pArgs[2];
	std::vector<std::string> vBmOutputPaths;
	bool isAttached = false;
	bool isPad = false;

	for(int i = 3; i < lArgCount; ++i) {
		if(std::strcmp(pArgs[i], "-attached") == 0) {
			isAttached = true;
		}
		else if(std::strcmp(pArgs[i], "-pad") == 0) {
			isPad = true;
		}
		else if(std::strcmp(pArgs[i], "-o") == 0) {
			while(i + 1 < lArgCount && pArgs[i + 1][0] != '-') {
				vBmOutputPaths.push_back(pArgs[++i]);
			}
			if(vBmOutputPaths.empty()) {
				nLog::error("-o needs an output path");
				return EXIT_FAILURE;
			}
		}
		else {
			nLog::error("Unknown arg '{}'", pArgs[i]);
			printUsage(pArgs[0]);
			return EXIT_FAILURE;
		}
	}

	if(isAttached) {
		if(vBmOutputPaths.size() != 2) {
			nLog::error("-attached needs -o lo.bm hi.bm");
			printUsage(pArgs[0]);
			return EXIT_FAILURE;
		}
	}
	else if(vBmOutputPaths.empty()) {
		vBmOutputPaths.push_back(nFs::removeExt(szInputPath) + ".bm");
	}
	else if(vBmOutputPaths.size() != 1) {
		nLog::error("Expected a single -o .bm path");
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	const std::string &szBmOutputPath = vBmOutputPaths[0];
	const std::string szBmHiOutputPath = isAttached ? vBmOutputPaths[1] : "";

	auto Palette = tPalette::fromFile(szPalettePath);
	if(!Palette.isValid() || Palette.m_vColors.size() < 4) {
		nLog::error("Palette '{}' needs at least 4 colors", szPalettePath);
		return EXIT_FAILURE;
	}

	auto SourceBitmap = tChunkyBitmap::fromPng(szInputPath);
	if(!SourceBitmap.m_uwHeight) {
		nLog::error("Couldn't open '{}'", szInputPath);
		return EXIT_FAILURE;
	}
	if(SourceBitmap.m_uwWidth & 0xF) {
		nLog::error("Width must be divisible by 16, got {}", SourceBitmap.m_uwWidth);
		return EXIT_FAILURE;
	}

	if(isPad) {
		tChunkyBitmap Padded(
			SourceBitmap.m_uwWidth, SourceBitmap.m_uwHeight + 2, Palette.m_vColors[0]
		);
		if(!SourceBitmap.copyRect(
			0, 0, Padded, 0, 1, SourceBitmap.m_uwWidth, SourceBitmap.m_uwHeight
		)) {
			nLog::error("Couldn't pad '{}'", szInputPath);
			return EXIT_FAILURE;
		}
		SourceBitmap = Padded;
	}

	if(isAttached) {
		fmt::print(
			"sprite_conv: {} → {} + {}\n",
			szInputPath, szBmOutputPath, szBmHiOutputPath
		);
		if(
			!writeSpriteBm(SourceBitmap, Palette, 0, szBmOutputPath) ||
			!writeSpriteBm(SourceBitmap, Palette, 2, szBmHiOutputPath)
		) {
			return EXIT_FAILURE;
		}
	}
	else {
		fmt::print("sprite_conv: {} → {}\n", szInputPath, szBmOutputPath);
		if(!writeSpriteBm(SourceBitmap, Palette, 0, szBmOutputPath)) {
			return EXIT_FAILURE;
		}
	}

	fmt::print("All done!\n");
	return EXIT_SUCCESS;
}
