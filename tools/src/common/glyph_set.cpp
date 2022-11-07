/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "glyph_set.h"
#include <fstream>
#include <fmt/format.h>
#include "../common/FreeTypeAmalgam.h"
#include "../common/lodepng.h"
#include "../common/endian.h"
#include "../common/rgb.h"
#include "../common/fs.h"
#include "../common/bitmap.h"
#include "../common/logging.h"
#include "utf8.h"

tGlyphSet tGlyphSet::fromPmng(const std::string &szPngPath, uint8_t ubStartIdx)
{
	tGlyphSet GlyphSet;

	auto Bitmap = tChunkyBitmap::fromPng(szPngPath);
	if(!Bitmap.m_uwHeight) {
		nLog::error("Couldn't open image '{}'", szPngPath);
		return GlyphSet;
	}

	const auto &Delimiter = Bitmap.pixelAt(0, 0);
	const auto &Bg = Bitmap.pixelAt(0, 1);
	uint16_t uwStart = 0;
	uint16_t uwCurrChar = ubStartIdx;

	for(uint16_t i = 1; i < Bitmap.m_uwWidth; ++i) {
		if(Bitmap.pixelAt(i, 0) == Delimiter) {
			uint16_t uwStop = i;

			tGlyphSet::tBitmapGlyph Glyph;
			Glyph.ubWidth = (uwStop - uwStart) - 1;
			Glyph.ubHeight = Bitmap.m_uwHeight;
			Glyph.ubBearing = 0;
			Glyph.vData.reserve(Glyph.ubWidth * Glyph.ubHeight);

			for(uint16_t y = 0; y < Bitmap.m_uwHeight; ++y) {
				for(uint16_t x = uwStart + 1; x < uwStop; ++x) {
					const auto &Pixel = Bitmap.pixelAt(x, y);
					Glyph.vData.push_back((Pixel == Bg ? 0 : 0xFF));
				}
			}
			GlyphSet.m_mGlyphs.insert(std::make_pair(uwCurrChar, Glyph));
			++uwCurrChar;

			uwStart = uwStop;
		}
	}
	return GlyphSet;
}

tGlyphSet tGlyphSet::fromAceFont(const std::string &szFntPath)
{
	tGlyphSet GlyphSet;
	std::ifstream FileFnt(szFntPath, std::ifstream::binary);
	if(!FileFnt.good()) {
		nLog::error("Couldn't open image '{}'", szFntPath);
		return GlyphSet;
	}

	// Read header
	uint8_t ubCharCount;
	uint16_t uwBitmapWidth, uwBitmapHeight;
	FileFnt.read(reinterpret_cast<char*>(&uwBitmapWidth), sizeof(uwBitmapWidth));
	FileFnt.read(reinterpret_cast<char*>(&uwBitmapHeight), sizeof(uwBitmapHeight));
	FileFnt.read(reinterpret_cast<char*>(&ubCharCount), sizeof(ubCharCount));
	uwBitmapWidth = nEndian::fromBig16(uwBitmapWidth);
	uwBitmapHeight = nEndian::fromBig16(uwBitmapHeight);

	// Read char offsets - read offset of one more to get last char's width
	std::vector<uint16_t> vCharOffsets;
	vCharOffsets.reserve(ubCharCount);
	for(uint16_t c = 0; c < ubCharCount; ++c) {
		uint16_t uwOffs;
		FileFnt.read(reinterpret_cast<char*>(&uwOffs), sizeof(uwOffs));
		uwOffs = nEndian::fromBig16(uwOffs);
		vCharOffsets.push_back(uwOffs);
	}

	tPlanarBitmap GlyphBitmapPlanar(uwBitmapWidth, uwBitmapHeight, 1);
	uint32_t ulOffs = 0;
	for(uint16_t uwY = 0; uwY < uwBitmapHeight; ++uwY) {
		for(uint16_t uwX = 0; uwX < uwBitmapWidth / 16; ++uwX) {
			uint16_t uwData;
			FileFnt.read(reinterpret_cast<char*>(&uwData), sizeof(uwData));
			uwData = nEndian::fromBig16(uwData);
			GlyphBitmapPlanar.m_pPlanes[0][ulOffs++] = uwData;
		}
	}
	FileFnt.close();

	tPalette Palette;
	Palette.m_vColors.push_back(tRgb(0));
	Palette.m_vColors.push_back(tRgb(0xFF));
	tChunkyBitmap Chunky(GlyphBitmapPlanar, Palette);
	for(uint16_t c = 0; c < ubCharCount - 1; ++c) {
		uint8_t ubGlyphWidth = vCharOffsets[c + 1] - vCharOffsets[c];
		if(ubGlyphWidth) {
			tBitmapGlyph Glyph;
			Glyph.vData.resize(uwBitmapHeight * ubGlyphWidth);
			for(uint8_t ubY = 0; ubY < uwBitmapHeight; ++ubY) {
				for(uint8_t ubX = 0; ubX < ubGlyphWidth; ++ubX) {
					// Since filled in Chunky is (FF,FF,FF), use data from any channel.
					Glyph.vData[ubY * ubGlyphWidth + ubX] = Chunky.pixelAt(vCharOffsets[c] + ubX, ubY).ubR;
				}
			}
			Glyph.ubWidth =  ubGlyphWidth;
			Glyph.ubHeight = uwBitmapHeight;
			Glyph.ubBearing = 0;
			GlyphSet.m_mGlyphs.emplace(std::make_pair(c, std::move(Glyph)));
		}
	}
	return GlyphSet;
}

tGlyphSet tGlyphSet::fromTtf(
	const std::string &szTtfPath, uint8_t ubSize, const std::string &szCharSet,
	uint8_t ubThreshold
)
{
	tGlyphSet GlyphSet;

	FT_Library FreeType;
	auto Error = FT_Init_FreeType(&FreeType);
	if(Error) {
		fmt::print("Couldn't open FreeType\n");
		return GlyphSet;
	}

	FT_Face Face;
	Error = FT_New_Face(FreeType, szTtfPath.c_str(), 0, &Face);
	if(Error) {
		fmt::print("Couldn't open font '{}'\n", szTtfPath);
		return GlyphSet;
	}

	FT_Set_Pixel_Sizes(Face, 0, ubSize);

	uint8_t ubMaxBearing = 0, ubMaxAddHeight = 0;
  uint32_t ulCodepoint, ulState = 0;
	for(const auto &c: szCharSet) {
		auto CharCode = *reinterpret_cast<const uint8_t*>(&c);
		if (
			decode(&ulState, &ulCodepoint, CharCode) != UTF8_ACCEPT ||
			ulCodepoint == '\n' || ulCodepoint == '\r'
		) {
			continue;
		}

		FT_Load_Char(Face, ulCodepoint, FT_LOAD_RENDER);

		uint8_t ubWidth = Face->glyph->bitmap.width;
		uint8_t ubHeight = Face->glyph->bitmap.rows;

		GlyphSet.m_mGlyphs[ulCodepoint] = {
			static_cast<uint8_t>(Face->glyph->metrics.horiBearingY / 64),
			ubWidth, ubHeight, std::vector<uint8_t>(ubWidth * ubHeight, 0)
		};
		auto &Glyph = GlyphSet.m_mGlyphs[ulCodepoint];

		// Copy bitmap graphics with threshold
		for(uint32_t ulPos = 0; ulPos < GlyphSet.m_mGlyphs[c].vData.size(); ++ulPos) {
			uint8_t ubVal = (Face->glyph->bitmap.buffer[ulPos] >= ubThreshold) ? 0xFF : 0;
			GlyphSet.m_mGlyphs[c].vData[ulPos] = ubVal;
		}

		// Trim left & right
		if(ubWidth != 0 && ubWidth != 0) {
			Glyph.trimHorz(false);
			Glyph.trimHorz(true);
		}
		else {
			// At least write proper width
			Glyph.ubWidth = Face->glyph->metrics.horiAdvance / 64;
		}

		ubMaxBearing = std::max(ubMaxBearing, Glyph.ubBearing);
		ubMaxAddHeight = std::max(
			ubMaxAddHeight,
			static_cast<uint8_t>(std::max(0, Glyph.ubHeight - Glyph.ubBearing))
		);
	}
	FT_Done_Face(Face);
	FT_Done_FreeType(FreeType);

	uint8_t ubBmHeight = ubMaxBearing + ubMaxAddHeight;

	// Normalize Glyph height
	for(auto &GlyphPair: GlyphSet.m_mGlyphs) {
		auto &Glyph = GlyphPair.second;
		std::vector<uint8_t> vNewData(Glyph.ubWidth * ubBmHeight, 0);
		auto Dst = vNewData.begin() + Glyph.ubWidth * (ubMaxBearing - Glyph.ubBearing);
		for(auto Src = Glyph.vData.begin(); Src != Glyph.vData.end(); ++Src, ++Dst) {
			*Dst = *Src;
		}
		Glyph.vData = vNewData;
		Glyph.ubHeight = ubBmHeight;
	}

	return GlyphSet;
}

tGlyphSet tGlyphSet::fromDir(const std::string &szDirPath)
{
	tGlyphSet GlyphSet;
	for(uint16_t c = 0; c <= 255; ++c) {
		auto Chunky = tChunkyBitmap::fromPng(fmt::format("{}/{}.png", szDirPath, c));
		if(Chunky.m_uwHeight) {
			tGlyphSet::tBitmapGlyph Glyph;
			Glyph.ubWidth = Chunky.m_uwWidth;
			Glyph.ubHeight = Chunky.m_uwHeight;
			Glyph.ubBearing = 0;
			Glyph.vData.reserve(Glyph.ubWidth * Glyph.ubHeight);
			// It's sufficient to base on R channel
			for(const auto &Pixel: Chunky.m_vData) {
				Glyph.vData.push_back(Pixel.ubR ? 0xFF : 0x00);
			}
			GlyphSet.m_mGlyphs.emplace(std::make_pair(c, std::move(Glyph)));
		}
	}
	return GlyphSet;
}

bool tGlyphSet::toDir(const std::string &szDirPath)
{
	nFs::dirCreate(szDirPath);
	for(const auto &GlyphPair: m_mGlyphs) {
		auto Glyph = GlyphPair.second;
		std::vector<tRgb> vImage(Glyph.ubWidth * Glyph.ubHeight, tRgb(0));

		for(auto y = 0; y < Glyph.ubHeight; ++y) {
			for(auto x = 0; x < Glyph.ubWidth; ++x) {
				auto Val = Glyph.vData[y * Glyph.ubWidth + x];
				vImage[y * Glyph.ubWidth + x] = tRgb(Val);
			}
		}

		auto LodeErr = lodepng_encode_file(
			fmt::format("{}/{}.png", szDirPath, static_cast<unsigned char>(GlyphPair.first)).c_str(),
			reinterpret_cast<uint8_t*>(vImage.data()),
			Glyph.ubWidth, Glyph.ubHeight, LCT_RGB, 8
		);

		if(LodeErr) {
			fmt::print("Lode Err: {}", LodeErr);
			return false;
		}
	}
	return true;
}

tChunkyBitmap tGlyphSet::toPackedBitmap(bool isPmng)
{
	uint8_t ubHeight = m_mGlyphs.begin()->second.ubHeight;

	// Calculate total width & round up to multiple of 16
	uint16_t uwWidth = 0;
	for(const auto &[Key, Glyph]: m_mGlyphs) {
		uwWidth += Glyph.ubWidth + (isPmng ? 1 : 0);
	}
	uwWidth = ((uwWidth + 15) / 16) * 16;

	tChunkyBitmap Chunky(uwWidth, ubHeight, tRgb(0));
	uint16_t uwOffsX = 0;
	for(const auto &[Key, Glyph]: m_mGlyphs) {
		if(isPmng) {
			// Add start marker
			Chunky.pixelAt(uwOffsX, 0) = tRgb(0xFF, 0, 0);
			uwOffsX += 1;
		}
		for(auto y = 0; y < Glyph.ubHeight; ++y) {
			for(auto x = 0; x < Glyph.ubWidth; ++x) {
				auto Val = Glyph.vData[y * Glyph.ubWidth + x];
				Chunky.pixelAt(uwOffsX + x, y) = tRgb(Val);
			}
		}
		uwOffsX += Glyph.ubWidth;
	}
	return Chunky;
}

void tGlyphSet::tBitmapGlyph::trimHorz(bool isRight)
{
	uint8_t ubHeight = this->ubHeight;
	if(ubHeight == 0) {
		return;
	}
	bool isTrimmable = true;
	while(isTrimmable) {
		uint8_t ubWidth = this->ubWidth;
		uint8_t ubDx = 0;
		uint8_t ubStartX = 1;
		if(isRight) {
			ubDx = ubWidth - 1;
			ubStartX = 0;
		}

		for(uint8_t y = 0; y < ubHeight; ++y) {
			if(this->vData[y * ubWidth + ubDx] != 0xFF) {
				isTrimmable = false;
				break;
			}
		}
		if(isTrimmable) {
			std::vector<uint8_t> vNew((ubWidth - 1) * ubHeight);
			for(uint8_t x = 0; x < ubWidth-1; ++x) {
				for(uint8_t y = 0; y < ubHeight; ++y) {
					vNew[y * (ubWidth - 1) + x] = this->vData[y * ubWidth + ubStartX + x];
				}
			}
			this->vData = vNew;
			--this->ubWidth;
		}
	}
}

void tGlyphSet::toAceFont(const std::string &szFontPath)
{
	uint16_t uwOffs = 0;
	uint8_t ubCharCount = 0;
	for(const auto &GlyphPair: m_mGlyphs) {
		if(GlyphPair.first > ubCharCount) {
			ubCharCount = GlyphPair.first;
		}
	}
	++ubCharCount;
	// Generate char offsets
	std::vector<uint16_t> vCharOffsets(256);
	for(uint16_t c = 0; c < ubCharCount; ++c) {
		vCharOffsets[c] = nEndian::toBig16(uwOffs);
		if(m_mGlyphs.count(c) != 0) {
			const auto &Glyph = m_mGlyphs.at(c);
			uwOffs += Glyph.ubWidth;
		}
	}
	// This allows drawing of last char
	vCharOffsets[ubCharCount] = nEndian::toBig16(uwOffs);
	++ubCharCount;

	tPlanarBitmap Planar(
		this->toPackedBitmap(false), tPalette({tRgb(0), tRgb(0xFF)}), tPalette()
	);

	std::ofstream Out(szFontPath, std::ofstream::out | std::ofstream::binary);
	// Write header
	uint16_t uwBitmapWidth = nEndian::toBig16(Planar.m_uwWidth);
	Out.write(reinterpret_cast<char*>(&uwBitmapWidth), sizeof(uint16_t));
	uint16_t uwBitmapHeight = nEndian::toBig16(m_mGlyphs.begin()->second.ubHeight);
	Out.write(reinterpret_cast<char*>(&uwBitmapHeight), sizeof(uint16_t));
	Out.write(reinterpret_cast<char*>(&ubCharCount), sizeof(uint8_t));

	// Write char offsets
	Out.write(
		reinterpret_cast<char*>(vCharOffsets.data()), sizeof(uint16_t) * ubCharCount
	);

	// Write font bitplane
	uint16_t uwRowWords = Planar.m_uwWidth / 16;
	for(uint16_t y = 0; y < Planar.m_uwHeight; ++y) {
		for(uint16_t x = 0; x < uwRowWords; ++x) {
			uint16_t uwData = nEndian::toBig16(Planar.m_pPlanes[0][y * uwRowWords + x]);
			Out.write(reinterpret_cast<char*>(&uwData), sizeof(uint16_t));
		}
	}

	Out.close();
}

bool tGlyphSet::isOk(void)
{
	return m_mGlyphs.size() != 0;
}

void tGlyphSet::remapGlyphs(const std::vector<std::pair<uint32_t, uint32_t>> &vFromTo)
{
	// Extract one by one and replace key so that other elements won't
	// be overwritten before key changing
	// This allows 'a' <=> 'b' replacement in one go
	std::vector<decltype(m_mGlyphs)::node_type> vExtracted;
	for(const auto &FromTo: vFromTo) {
		auto Pos = m_mGlyphs.find(FromTo.first);
		if(Pos != m_mGlyphs.end()) {
			auto Node = m_mGlyphs.extract(Pos);
			Node.key() = FromTo.second;
			vExtracted.push_back(std::move(Node));
		}
	}

	for(auto &Node: vExtracted) {
		m_mGlyphs.insert(std::move(Node));
	}
}
