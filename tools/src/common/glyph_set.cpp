/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "glyph_set.h"
#include <fstream>
#include <fmt/format.h>
#include <freetype/freetype.h>
#include "../common/lodepng.h"
#include "../common/endian.h"
#include "../common/rgb.h"
#include "../common/fs.h"
#include "../common/bitmap.h"
#include "../common/logging.h"
#include "utf8.h"

tGlyphSet tGlyphSet::fromPmng(const std::string &szPngPath, std::uint8_t ubStartIdx)
{
	tGlyphSet GlyphSet;

	auto Bitmap = tChunkyBitmap::fromPng(szPngPath);
	if(!Bitmap.m_uwHeight) {
		nLog::error("Couldn't open image '{}'", szPngPath);
		return GlyphSet;
	}

	const auto &Delimiter = Bitmap.pixelAt(0, 0);
	const auto &Bg = Bitmap.pixelAt(0, 1);
	std::uint16_t uwStart = 0;
	std::uint16_t uwCurrChar = ubStartIdx;

	for(std::uint16_t i = 1; i < Bitmap.m_uwWidth; ++i) {
		if(Bitmap.pixelAt(i, 0) == Delimiter) {
			std::uint16_t uwStop = i;

			tGlyphSet::tBitmapGlyph Glyph;
			Glyph.m_ubWidth = (uwStop - uwStart) - 1;
			Glyph.m_ubHeight = Bitmap.m_uwHeight;
			Glyph.m_ubBearing = 0;
			Glyph.m_vData.reserve(Glyph.m_ubWidth * Glyph.m_ubHeight);

			for(std::uint16_t y = 0; y < Bitmap.m_uwHeight; ++y) {
				for(std::uint16_t x = uwStart + 1; x < uwStop; ++x) {
					const auto &Pixel = Bitmap.pixelAt(x, y);
					Glyph.m_vData.push_back((Pixel == Bg ? 0 : 0xFF));
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
	std::uint8_t ubCharCount;
	std::uint16_t uwBitmapWidth, uwBitmapHeight;
	FileFnt.read(reinterpret_cast<char*>(&uwBitmapWidth), sizeof(uwBitmapWidth));
	FileFnt.read(reinterpret_cast<char*>(&uwBitmapHeight), sizeof(uwBitmapHeight));
	FileFnt.read(reinterpret_cast<char*>(&ubCharCount), sizeof(ubCharCount));
	uwBitmapWidth = nEndian::fromBig16(uwBitmapWidth);
	uwBitmapHeight = nEndian::fromBig16(uwBitmapHeight);

	// Read char offsets - read offset of one more to get last char's width
	std::vector<uint16_t> vCharOffsets;
	vCharOffsets.reserve(ubCharCount);
	for(std::uint16_t c = 0; c < ubCharCount; ++c) {
		std::uint16_t uwOffs;
		FileFnt.read(reinterpret_cast<char*>(&uwOffs), sizeof(uwOffs));
		uwOffs = nEndian::fromBig16(uwOffs);
		vCharOffsets.push_back(uwOffs);
	}

	tPlanarBitmap GlyphBitmapPlanar(uwBitmapWidth, uwBitmapHeight, 1);
	std::uint32_t ulOffs = 0;
	for(std::uint16_t uwY = 0; uwY < uwBitmapHeight; ++uwY) {
		for(std::uint16_t uwX = 0; uwX < uwBitmapWidth / 16; ++uwX) {
			std::uint16_t uwData;
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
	for(std::uint16_t c = 0; c < ubCharCount - 1; ++c) {
		std::uint8_t ubGlyphWidth = vCharOffsets[c + 1] - vCharOffsets[c];
		if(ubGlyphWidth) {
			tBitmapGlyph Glyph;
			Glyph.m_vData.resize(uwBitmapHeight * ubGlyphWidth);
			for(std::uint8_t ubY = 0; ubY < uwBitmapHeight; ++ubY) {
				for(std::uint8_t ubX = 0; ubX < ubGlyphWidth; ++ubX) {
					// Since filled in Chunky is (FF,FF,FF), use data from any channel.
					Glyph.m_vData[ubY * ubGlyphWidth + ubX] = Chunky.pixelAt(vCharOffsets[c] + ubX, ubY).ubR;
				}
			}
			Glyph.m_ubWidth =  ubGlyphWidth;
			Glyph.m_ubHeight = uwBitmapHeight;
			Glyph.m_ubBearing = 0;
			GlyphSet.m_mGlyphs.emplace(std::make_pair(c, std::move(Glyph)));
		}
	}
	return GlyphSet;
}

tGlyphSet tGlyphSet::fromTtf(
	const std::string &szTtfPath, std::uint8_t ubSize, const std::string &szCharSet,
	std::uint8_t ubThreshold
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

	std::uint8_t ubMaxBearing = 0, ubMaxAddHeight = 0;
  std::uint32_t ulCodepoint, ulState = 0;
	for(const auto &c: szCharSet) {
		auto CharCode = *reinterpret_cast<const uint8_t*>(&c);
		if (
			decode(&ulState, &ulCodepoint, CharCode) != UTF8_ACCEPT ||
			ulCodepoint == '\n' || ulCodepoint == '\r'
		) {
			continue;
		}

		FT_Load_Char(Face, ulCodepoint, FT_LOAD_RENDER);

		std::uint8_t ubWidth = Face->glyph->bitmap.width;
		std::uint8_t ubHeight = Face->glyph->bitmap.rows;

		GlyphSet.m_mGlyphs[ulCodepoint] = {
			static_cast<uint8_t>(Face->glyph->metrics.horiBearingY / 64),
			ubWidth, ubHeight, std::vector<uint8_t>(ubWidth * ubHeight, 0)
		};
		auto &Glyph = GlyphSet.m_mGlyphs[ulCodepoint];

		// Copy bitmap graphics with threshold
		for(std::uint32_t ulPos = 0; ulPos < Glyph.m_vData.size(); ++ulPos) {
			std::uint8_t ubVal = (Face->glyph->bitmap.buffer[ulPos] >= ubThreshold) ? 0xFF : 0;
			Glyph.m_vData[ulPos] = ubVal;
		}

		// Trim left & right
		if(ubWidth != 0 && ubWidth != 0) {
			Glyph.trimHorz(false);
			Glyph.trimHorz(true);
		}
		else {
			// At least write proper width
			Glyph.m_ubWidth = Face->glyph->metrics.horiAdvance / 64;
		}

		ubMaxBearing = std::max(ubMaxBearing, Glyph.m_ubBearing);
		ubMaxAddHeight = std::max(
			ubMaxAddHeight,
			static_cast<uint8_t>(std::max(0, Glyph.m_ubHeight - Glyph.m_ubBearing))
		);
	}
	FT_Done_Face(Face);
	FT_Done_FreeType(FreeType);

	std::uint8_t ubBmHeight = ubMaxBearing + ubMaxAddHeight;

	// Normalize Glyph height
	for(auto &GlyphPair: GlyphSet.m_mGlyphs) {
		auto &Glyph = GlyphPair.second;
		std::vector<uint8_t> vNewData(Glyph.m_ubWidth * ubBmHeight, 0);
		auto Dst = vNewData.begin() + Glyph.m_ubWidth * (ubMaxBearing - Glyph.m_ubBearing);
		for(auto Src = Glyph.m_vData.begin(); Src != Glyph.m_vData.end(); ++Src, ++Dst) {
			*Dst = *Src;
		}
		Glyph.m_vData = vNewData;
		Glyph.m_ubHeight = ubBmHeight;
	}

	return GlyphSet;
}

tGlyphSet tGlyphSet::fromDir(const std::string &szDirPath)
{
	tGlyphSet GlyphSet;
	for(std::uint16_t c = 0; c <= 255; ++c) {
		auto Chunky = tChunkyBitmap::fromPng(fmt::format("{}/{}.png", szDirPath, c));
		if(Chunky.m_uwHeight) {
			tGlyphSet::tBitmapGlyph Glyph;
			Glyph.m_ubWidth = Chunky.m_uwWidth;
			Glyph.m_ubHeight = Chunky.m_uwHeight;
			Glyph.m_ubBearing = 0;
			Glyph.m_vData.reserve(Glyph.m_ubWidth * Glyph.m_ubHeight);
			// It's sufficient to base on R channel
			for(const auto &Pixel: Chunky.m_vData) {
				Glyph.m_vData.push_back(Pixel.ubR ? 0xFF : 0x00);
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
		std::vector<tRgb> vImage(Glyph.m_ubWidth * Glyph.m_ubHeight, tRgb(0));

		for(auto y = 0; y < Glyph.m_ubHeight; ++y) {
			for(auto x = 0; x < Glyph.m_ubWidth; ++x) {
				auto Val = Glyph.m_vData[y * Glyph.m_ubWidth + x];
				vImage[y * Glyph.m_ubWidth + x] = tRgb(Val);
			}
		}

		auto LodeErr = lodepng_encode_file(
			fmt::format("{}/{}.png", szDirPath, static_cast<unsigned char>(GlyphPair.first)).c_str(),
			reinterpret_cast<uint8_t*>(vImage.data()),
			Glyph.m_ubWidth, Glyph.m_ubHeight, LCT_RGB, 8
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
	std::uint8_t ubHeight = m_mGlyphs.begin()->second.m_ubHeight;

	// Calculate total width & round up to multiple of 16
	std::uint16_t uwWidth = 0;
	for(const auto &[Key, Glyph]: m_mGlyphs) {
		uwWidth += Glyph.m_ubWidth + (isPmng ? 1 : 0);
	}
	uwWidth = ((uwWidth + 15) / 16) * 16;

	tChunkyBitmap Chunky(uwWidth, ubHeight, tRgb(0));
	std::uint16_t uwOffsX = 0;
	for(const auto &[Key, Glyph]: m_mGlyphs) {
		if(isPmng) {
			// Add start marker
			Chunky.pixelAt(uwOffsX, 0) = tRgb(0xFF, 0, 0);
			uwOffsX += 1;
		}
		for(auto y = 0; y < Glyph.m_ubHeight; ++y) {
			for(auto x = 0; x < Glyph.m_ubWidth; ++x) {
				auto Val = Glyph.m_vData[y * Glyph.m_ubWidth + x];
				Chunky.pixelAt(uwOffsX + x, y) = tRgb(Val);
			}
		}
		uwOffsX += Glyph.m_ubWidth;
	}
	return Chunky;
}

std::uint8_t tGlyphSet::tBitmapGlyph::getValueAt(std::uint8_t ubX, std::uint8_t ubY)
{
	std::uint8_t ubValue = this->m_vData[ubY * this->m_ubWidth + ubY];
	return ubValue;
}

bool tGlyphSet::tBitmapGlyph::hasEmptyColumn(std::uint8_t ubX)
{
	for(std::uint8_t ubY = 0; ubY < this->m_ubHeight; ++ubY) {
		if(getValueAt(ubX, ubY) != 0) {
			return false;
		}
	}
	return true;
}

bool tGlyphSet::tBitmapGlyph::isEmpty(void)
{
	for(std::uint8_t ubX = 0; ubX < this->m_ubWidth; ++ubX) {
		if(!hasEmptyColumn(ubX)) {
			return false;
		}
	}
	return true;
}

void tGlyphSet::tBitmapGlyph::trimHorz(bool isRight)
{
	if(isEmpty()) {
		return;
	}

	std::uint8_t ubNewWidth;
	std::uint8_t ubNewStart;
	if(isRight) {
		ubNewStart = 0;
		for(ubNewWidth = m_ubWidth; ubNewWidth > 0; --ubNewWidth) {
			if(!hasEmptyColumn(ubNewWidth - 1)) {
				break;
			}
		}
	}
	else {
		for(ubNewStart = 0; ubNewStart < m_ubWidth; ++ubNewStart) {
			if(!hasEmptyColumn(ubNewStart)) {
				break;
			}
		}
		ubNewWidth = m_ubWidth - ubNewStart;
	}

	std::vector<uint8_t> vNew(ubNewWidth * m_ubHeight);
	for(std::uint8_t ubNewX = 0; ubNewX < ubNewWidth; ++ubNewX) {
		for(std::uint8_t ubNewY = 0; ubNewY < m_ubHeight; ++ubNewY) {
			vNew[ubNewY * ubNewWidth + ubNewX] = this->m_vData[ubNewY * m_ubWidth + ubNewStart + ubNewX];
		}
	}
	m_vData = vNew;
	m_ubWidth = ubNewWidth;
}

void tGlyphSet::toAceFont(const std::string &szFontPath)
{
	std::uint16_t uwOffs = 0;
	std::uint8_t ubCharCount = 0;
	for(const auto &GlyphPair: m_mGlyphs) {
		if(GlyphPair.first > ubCharCount) {
			ubCharCount = GlyphPair.first;
		}
	}
	++ubCharCount;
	// Generate char offsets
	std::vector<uint16_t> vCharOffsets(256);
	for(std::uint16_t c = 0; c < ubCharCount; ++c) {
		vCharOffsets[c] = nEndian::toBig16(uwOffs);
		if(m_mGlyphs.count(c) != 0) {
			const auto &Glyph = m_mGlyphs.at(c);
			uwOffs += Glyph.m_ubWidth;
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
	std::uint16_t uwBitmapWidth = nEndian::toBig16(Planar.m_uwWidth);
	Out.write(reinterpret_cast<char*>(&uwBitmapWidth), sizeof(uint16_t));
	std::uint16_t uwBitmapHeight = nEndian::toBig16(m_mGlyphs.begin()->second.m_ubHeight);
	Out.write(reinterpret_cast<char*>(&uwBitmapHeight), sizeof(uint16_t));
	Out.write(reinterpret_cast<char*>(&ubCharCount), sizeof(uint8_t));

	// Write char offsets
	Out.write(
		reinterpret_cast<char*>(vCharOffsets.data()), sizeof(uint16_t) * ubCharCount
	);

	// Write font bitplane
	std::uint16_t uwRowWords = Planar.m_uwWidth / 16;
	for(std::uint16_t y = 0; y < Planar.m_uwHeight; ++y) {
		for(std::uint16_t x = 0; x < uwRowWords; ++x) {
			std::uint16_t uwData = nEndian::toBig16(Planar.m_pPlanes[0][y * uwRowWords + x]);
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
