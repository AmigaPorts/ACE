/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "common/bitmap.h"
#include "common/logging.h"
#include "common/parse.h"
#include "common/fs.h"

void printUsage(const std::string &szAppName)
{
	// tileset_conv inPath tileSize outPath
	using fmt::print;
	print("Usage:\n\t{} inPath tileSize outPath\n", szAppName);
	print("\ninPath\t\t- path to input file/directory\n");
	print("tileSize\t- size of tile's edge, in pixels\n");
	print("outPath\t\t- path to output file/directory\n");
	print("\nWhen using .bm as output:\n");
	print("-i\t\t\t- save as interleaved bitmap\n");
	print("-plt palettePath\t- use following palette\n");
}

int main(int lArgCount, const char *pArgs[])
{
	if(lArgCount - 1 < 3) {
		nLog::error("Too few arguments, got {}", lArgCount - 1);
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	int32_t lTileSize;
	if(!nParse::toInt32(pArgs[2], "tileSize", lTileSize) || lTileSize < 0) {
		return EXIT_FAILURE;
	}

	std::string szInPath = pArgs[1];
	std::string szOutPath = pArgs[3];
	bool isInterleaved = false;
	tPalette Palette;
	for(uint8_t i = 4; i < lArgCount - 1; ++i) {
		if(pArgs[i] == std::string("-i")) {
			isInterleaved = true;
		}
		else if(pArgs[i] == std::string("-plt") && i + 1 <= lArgCount - 1) {
			++i;
			Palette = tPalette::fromFile(pArgs[i]);
			if(!Palette.m_vColors.size()) {
				nLog::error("Couldn't read palette: '{}'", pArgs[i]);
				return EXIT_FAILURE;
			}
			fmt::print(
				"Read {} colors from '{}'\n", Palette.m_vColors.size(), pArgs[i]
			);
		}
	}

	// Read input
	std::string szInExt = nFs::getExt(szInPath);
	std::vector<tChunkyBitmap> vTiles; // NOTE: empty tiles will have zero w/h
	if(szInExt == "png") {
		tChunkyBitmap In = tChunkyBitmap::fromPng(szInPath);
		if(In.m_uwHeight <= 0) {
			nLog::error("Couldn't load input file: '{}'", szInPath);
			return EXIT_FAILURE;
		}
		if(In.m_uwHeight % lTileSize != 0) {
			nLog::error("Input bitmap is not divisible by {}", lTileSize);
			return EXIT_FAILURE;
		}
		uint16_t uwTileCount = In.m_uwHeight / lTileSize;
		vTiles.reserve(uwTileCount);
		for(uint16_t i = 0; i < uwTileCount; ++i) {
			tChunkyBitmap Tile(lTileSize, lTileSize);
			In.copyRect(0, i * lTileSize, Tile, 0, 0, lTileSize, lTileSize);
			vTiles.push_back(std::move(Tile));
		}
	}
	else if(nFs::isDir(szInPath)) {
		int16_t wLastFull = -1;
		for(uint16_t i = 0; i < 256; ++i) {
			auto Tile = tChunkyBitmap::fromPng(fmt::format("{}/{}.png", szInPath, i));
			vTiles.push_back(std::move(Tile));
			if(Tile.m_uwHeight != 0) {
				wLastFull = i;
			}
		}
		vTiles.resize(wLastFull + 1);
	}
	else {
		nLog::error("Unsupported input extension: '{}'", szInExt);
		return EXIT_FAILURE;
	}

	// Save output
	uint16_t uwTileCount = vTiles.size();
	fmt::print("Read {} tiles from '{}'\n", uwTileCount, szInPath);
	std::string szOutExt = nFs::getExt(szOutPath);
	if(szOutExt == "png" || szOutExt == "bm") {
		tRgb Bg;
		if(Palette.m_vColors.size()) {
			Bg = Palette.m_vColors.at(0);
		}
		fmt::print("Using color for bg: {} {} {}\n", Bg.ubR, Bg.ubG, Bg.ubB);
		tChunkyBitmap Out(lTileSize, uwTileCount * lTileSize, Bg);
		for(uint16_t i = 0; i < uwTileCount; ++i) {
			auto &Tile = vTiles.at(i);
			if(Tile.m_uwHeight != 0) {
				Tile.copyRect(0, 0, Out, 0, lTileSize * i, lTileSize, lTileSize);
			}
		}
		if(szOutExt == "png" && !Out.toPng(szOutPath)) {
			nLog::error("Couldn't write output to '{}'", szOutPath);
			return EXIT_FAILURE;
		}
		else if(szOutExt == "bm") {
			if(Palette.m_vColors.size() == 0) {
				nLog::error("No palette specified");
				return EXIT_FAILURE;
			}
			tPlanarBitmap PlanarOut(Out, Palette);
			if(PlanarOut.m_uwHeight == 0) {
				nLog::error("Problem with planar conversion");
				return EXIT_FAILURE;
			}
			if(!PlanarOut.toBm(szOutPath, isInterleaved)) {
				nLog::error("Couldn't write to '{}'", szOutPath);
				return EXIT_FAILURE;
			}
		}
	}
	else if(szOutExt == "") {
		// Tile directory
		nFs::dirCreate(szOutPath);
		for(uint16_t i = 0; i < uwTileCount; ++i) {
			auto &Tile = vTiles.at(i);
			if(Tile.m_uwHeight != 0) {
				std::string szTilePath = fmt::format("{}/{}.png", szOutPath, i);
				if(!Tile.toPng(szTilePath)) {
					nLog::error("Couldn't write tile to '{}'", szTilePath);
					return EXIT_FAILURE;
				}
			}
		}
	}
	else {
		nLog::error("Unsupported output extension: '{}'", szOutExt);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
