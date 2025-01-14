/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>
#include "common/bitmap.h"
#include "common/logging.h"
#include "common/parse.h"
#include "common/fs.h"
#include "common/math.h"
#include "common/exception.h"

struct tConfig {
	std::int32_t m_lTileSize;
	std::int32_t m_lTileHeight;
	std::string m_szInPath;
	std::string m_szOutPath;
	std::string m_szPalettePath;
	bool m_isInterleaved;
	std::int32_t m_lColumns;
	std::optional<int32_t> m_lColumnWidth;
	bool m_isVaryingHeight;
	bool m_isHeightOverride;

	tConfig(const std::vector<const char*> &vArgs);
};

tConfig::tConfig(const std::vector<const char*> &vArgs)
{
	auto ArgCount = vArgs.size();
	if(ArgCount - 1 < 3) {
		throw std::runtime_error(fmt::format("Too few arguments, got {}", ArgCount - 1));
	}

	if(!nParse::toInt32(vArgs[2], "tileSize", m_lTileSize) || m_lTileSize < 0) {
		throw std::runtime_error(nullptr);
	}
	m_lTileHeight = m_lTileSize;

	m_szInPath = vArgs[1];
	m_szOutPath = vArgs[3];
	m_isInterleaved = false;
	m_lColumns = 1;
	m_isVaryingHeight = false;
	m_isHeightOverride = false;

	for(auto ArgIndex = 4; ArgIndex < ArgCount; ++ArgIndex) {
		if(vArgs[ArgIndex] == std::string("-i")) {
			m_isInterleaved = true;
		}
		else if(vArgs[ArgIndex] == std::string("-vh")) {
			m_isVaryingHeight = true;
		}
		else if(vArgs[ArgIndex] == std::string("-plt") && ArgIndex < ArgCount - 1) {
			++ArgIndex;
			m_szPalettePath = vArgs[ArgIndex];
		}
		else if(vArgs[ArgIndex] == std::string("-cols") && ArgIndex < ArgCount - 1) {
			++ArgIndex;
			if(!nParse::toInt32(vArgs[ArgIndex], "-cols", m_lColumns)) {
				throw std::runtime_error(nullptr); // error message inside parsing fn
			}
			if(m_lColumns <= 0) {
				throw std::runtime_error("-cols value must be positive");
			}
		}
		else if(vArgs[ArgIndex] == std::string("-cw") && ArgIndex < ArgCount - 1) {
			++ArgIndex;
			std::int32_t lColumnWidth;
			if(!nParse::toInt32(vArgs[ArgIndex], "-cw", lColumnWidth)) {
				throw std::runtime_error(nullptr); // error message inside parsing fn
			}
			if(lColumnWidth <= 0) {
				throw std::runtime_error("-cw value must be positive");
			}
			m_lColumnWidth = lColumnWidth;
		}
		else if(vArgs[ArgIndex] == std::string("-h") && ArgIndex < ArgCount - 1) {
			++ArgIndex;
			if(!nParse::toInt32(vArgs[ArgIndex], "-h", m_lTileHeight)) {
				throw std::runtime_error(nullptr);
			}
			if(m_lTileHeight <= 0) {
				throw std::runtime_error("-h value must be positive");
			}
			m_isHeightOverride = true;
			fmt::print("Override tile height to {}\n", m_lTileHeight);
		}
	}

	if(m_isVaryingHeight && m_isHeightOverride) {
		throw std::runtime_error("Can't use -vh and -h params together!");
	}

	if(m_isVaryingHeight && m_lColumns != 1) {
		throw std::runtime_error("Can't use -vh and -cols params togeter!");
	}
}

static void printUsage(const std::string &szAppName)
{
	// tileset_conv inPath tileSize outPath
	using fmt::print;
	print("Usage:\n\t{} inPath tileSize outPath\n\n", szAppName);
	print("inPath  \t- path to input file/directory\n");
	print("tileSize\t- size of tile's edge, in pixels. For rectangular tiles, controls the tile width.\n");
	print("outPath \t- path to output file/directory\n");
	print("\nWhen using .bm as output:\n");
	print("-i              \t- save as interleaved bitmap\n");
	print("\nWhen using .bm as input or output:\n");
	print("-plt palettePath\t- use following palette\n");
	print("\nAdditional options:\n");
	print("-cols        \t- number of columns to use in output (default: 1)\n");
	print("-cw          \t- override tile column width, useful for tiles of width not equal to multiple of 16px\n");
	print("-h tileHeight\t- override height for rectangular tiles\n");
	print("-vh          \t- enable varying height (can't be used with -h and -cols)\n");
}

/**
 * @brief Reads config from source file.
 *
 * @param Config Configuration parameters of read operation.
 * @return Vector of tiles, each stored in separate chunky bitmap.
 *
 * @note Empty tiles are returned as chunky bitmaps with width/height set to zero.
 */
static std::vector<tChunkyBitmap> readTiles(
	const tConfig &Config, const std::optional<tPalette> &Palette
)
{
	std::vector<tChunkyBitmap> vTiles;
	std::string szInExt = nFs::getExt(Config.m_szInPath);
	if(szInExt == "png" || szInExt == "bm") {
		tChunkyBitmap In;
		auto ColumnWidth = Config.m_lColumnWidth.has_value() ? Config.m_lColumnWidth.value() : Config.m_lTileSize;
		if(szInExt == "png") {
			In = tChunkyBitmap::fromPng(Config.m_szInPath);
		}
		else if(szInExt == "bm") {
			if(!Palette.has_value()) {
				throw new std::runtime_error("No palette specified to use for .bm file read");
			}

			auto InPlanar = tPlanarBitmap::fromBm(Config.m_szInPath);
			In = tChunkyBitmap(InPlanar, Palette.value());
		}

		if(In.m_uwHeight <= 0) {
			throw std::runtime_error(fmt::format("Couldn't load input file: '{}'", Config.m_szInPath));
		}

		if(In.m_uwHeight % Config.m_lTileHeight != 0 && !Config.m_isVaryingHeight) {
			throw std::runtime_error(fmt::format("Input bitmap height is not divisible by {}", Config.m_lTileHeight));
		}

		if(In.m_uwWidth % Config.m_lTileSize != 0) {
			throw std::runtime_error(fmt::format("Input bitmap width is not divisible by {}", Config.m_lTileSize));
		}

		std::uint16_t TileCountHoriz = In.m_uwWidth / ColumnWidth;
		std::uint16_t TileCountVert = In.m_uwHeight / Config.m_lTileHeight;

		vTiles.reserve(TileCountHoriz * TileCountVert);
		for(std::uint16_t y = 0; y < TileCountVert; ++y) {
			for(std::uint16_t x = 0; x < TileCountHoriz; ++x) {
				tChunkyBitmap Tile(Config.m_lTileSize, Config.m_lTileHeight);
				In.copyRect(
					x * ColumnWidth, y * Config.m_lTileHeight, Tile, 0, 0,
					Config.m_lTileSize, Config.m_lTileHeight
				);
				vTiles.push_back(std::move(Tile));
			}
		}
	}
	else if(nFs::isDir(Config.m_szInPath)) {
		std::int16_t wLastFull = -1;
		for(std::uint16_t i = 0; i < 256; ++i) {
			auto Tile = tChunkyBitmap::fromPng(fmt::format("{}/{}.png", Config.m_szInPath, i));
			if(Tile.m_uwHeight != 0) {
				wLastFull = i;
				if(!Config.m_isVaryingHeight && Tile.m_uwHeight != Config.m_lTileHeight) {
					throw std::runtime_error(fmt::format(
						"Tile {} height doesn't match others: got {}, expected {}",
						i, Tile.m_uwHeight, Config.m_lTileHeight
					));
				}
			}
			vTiles.push_back(std::move(Tile));
		}
		vTiles.resize(wLastFull + 1);
	}
	else {
		throw std::runtime_error(fmt::format("Unsupported input extension: '{}'", szInExt));
	}
	return vTiles;
}

static void saveTiles(
	const std::vector<tChunkyBitmap> &vTiles, const std::optional<tPalette> &Palette,
	const tConfig &Config
)
{
	auto TileCount = vTiles.size();
	std::string szOutExt = nFs::getExt(Config.m_szOutPath);
	if(szOutExt == "png" || szOutExt == "bm") {
		tRgb Bg;
		if(Palette.has_value() && !Palette.value().m_vColors.empty()) {
			Bg = Palette.value().m_vColors.at(0);
		}
		fmt::print("Using color for bg: #{:02X}{:02X}{:02X}\n", Bg.ubR, Bg.ubG, Bg.ubB);

		std::optional<tChunkyBitmap> Out;
		auto ColumnWidth = Config.m_lColumnWidth.has_value() ? Config.m_lColumnWidth.value() : Config.m_lTileSize;
		if(Config.m_lColumns != 1) {
			Out = std::make_optional<tChunkyBitmap>(
				uint16_t(ColumnWidth * Config.m_lColumns),
				uint16_t(ceilToFactor(TileCount, Config.m_lColumns) * Config.m_lTileHeight),
				Bg
			);

			for(std::uint16_t i = 0; i < TileCount; ++i) {
				auto &Tile = vTiles.at(i);
				if(Tile.m_uwHeight != 0) {
					Tile.copyRect(
						0, 0, Out.value(),
						(i % Config.m_lColumns) * ColumnWidth,
						Config.m_lTileHeight * (i / Config.m_lColumns),
						Config.m_lTileSize, Config.m_lTileHeight
					);
				}
			}
		}
		else {
			std::uint16_t uwTilesetHeight = 0;
			if(Config.m_isVaryingHeight) {
				bool hasEmptyTile = false;
				for(const auto &Tile: vTiles) {
					if(Tile.m_uwHeight == 0) {
						throw std::runtime_error("Empty tiles not allowed inside varying-height tilesets!");
					}
					uwTilesetHeight += Tile.m_uwHeight;
				}
			}
			else {
				uwTilesetHeight = uint16_t(Config.m_lTileHeight * TileCount);
			}

			Out = std::make_optional<tChunkyBitmap>(
				ColumnWidth, uwTilesetHeight, Bg
			);

			std::uint16_t uwOffsY = 0;
			for(const auto &Tile: vTiles) {
				Tile.copyRect(
					0, 0, Out.value(), 0, uwOffsY, Tile.m_uwWidth, Tile.m_uwHeight
				);
				uwOffsY += Config.m_isVaryingHeight ? Tile.m_uwHeight : Config.m_lTileHeight;
			}
		}

		if(szOutExt == "png" && !Out.value().toPng(Config.m_szOutPath)) {
			throw std::runtime_error(fmt::format("Couldn't write output to '{}'", Config.m_szOutPath));
		}
		else if(szOutExt == "bm") {
			if(!Palette.has_value() || Palette.value().m_vColors.size() == 0) {
				throw std::runtime_error("No valid palette specified");
			}
			tPlanarBitmap PlanarOut(Out.value(), Palette.value());
			if(PlanarOut.m_uwHeight == 0) {
				throw std::runtime_error("Problem with planar conversion");
			}
			if(!PlanarOut.toBm(Config.m_szOutPath, Config.m_isInterleaved)) {
				throw std::runtime_error(fmt::format("Couldn't write to '{}'", Config.m_szOutPath));
			}
		}
	}
	else if(szOutExt == "") {
		// Tile directory
		nFs::dirCreate(Config.m_szOutPath);
		for(std::uint16_t i = 0; i < TileCount; ++i) {
			auto &Tile = vTiles.at(i);
			if(Tile.m_uwHeight != 0) {
				std::string szTilePath = fmt::format("{}/{}.png", Config.m_szOutPath, i);
				if(!Tile.toPng(szTilePath)) {
					throw std::runtime_error(fmt::format("Couldn't write tile to '{}'", szTilePath));
				}
			}
		}
	}
	else {
		throw std::runtime_error(fmt::format("Unsupported output extension: '{}'", szOutExt));
	}
}

int main(int lArgCount, const char *pArgs[])
{
	std::optional<tConfig> Config;
	try {
		std::vector<const char*> Args(pArgs, pArgs + lArgCount);
		Config = std::make_optional<tConfig>(Args);
	}
	catch(std::exception &Ex) {
		exceptionHandle(Ex, "parsing parameters");
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	std::optional<tPalette> Palette;
	if(!Config->m_szPalettePath.empty()) {
		Palette = tPalette::fromFile(Config->m_szPalettePath);
		if(Palette.value().m_vColors.empty()) {
			nLog::error("Couldn't read palette: '{}'", Config->m_szPalettePath);
			return EXIT_FAILURE;
		}
		fmt::print(
			"Read {} colors from '{}'\n",
			Palette.value().m_vColors.size(), Config->m_szPalettePath
		);
	}

	std::vector<tChunkyBitmap> vTiles;
	try {
		vTiles = readTiles(Config.value(), Palette);
		fmt::print("Read {} tiles from '{}'\n", vTiles.size(), Config->m_szInPath);
	}
	catch(std::exception &Ex) {
		exceptionHandle(Ex, "reading tiles");
		return EXIT_FAILURE;
	}

	try {
		saveTiles(vTiles, Palette, Config.value());
	}
	catch(std::exception &Ex) {
		exceptionHandle(Ex, "writing tiles");
	}

	return EXIT_SUCCESS;
}
