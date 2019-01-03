/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "font_conv.h"
#include <fmt/format.h>
#include "FreeTypeAmalgam.h"
#include "lodepng.h"

struct tPixel {
	uint8_t r, g, b;
};

tFontConv::tFontConv(void)
{

}

tFontConv::~tFontConv(void)
{

}

tFontConv::tGlyphSet tFontConv::glyphsFromTtf(
	const std::string &szTtfPath, uint8_t ubSize, const std::string &szCharSet,
	uint8_t ubThreshold
)
{
	tFontConv::tGlyphSet mGlyphs;

	FT_Library FreeType;
	auto Error = FT_Init_FreeType(&FreeType);
	if(Error) {
		fmt::print("Couldn't open FreeType\n");
		return mGlyphs;
	}

	FT_Face Face;
	Error = FT_New_Face(FreeType, szTtfPath.c_str(), 0, &Face);
	if(Error) {
		fmt::print("Couldn't open font '{}'\n", szTtfPath);
		return mGlyphs;
	}

	FT_Set_Pixel_Sizes(Face, 0, ubSize);

	uint8_t ubMaxBearing = 0, ubMaxAddHeight = 0;
	for(const auto &c: szCharSet) {
		FT_Load_Char(Face, c, FT_LOAD_RENDER);

		uint8_t ubWidth = Face->glyph->bitmap.width;
		uint8_t ubHeight = Face->glyph->bitmap.rows;

		mGlyphs[c] = {
			static_cast<uint8_t>(Face->glyph->metrics.horiBearingY / 64),
			ubWidth, ubHeight, std::vector<uint8_t>(ubWidth * ubHeight, 0xFF)
		};

		// Copy bitmap graphics with threshold
		for(uint32_t ulPos = 0; ulPos < mGlyphs[c].vData.size(); ++ulPos) {
			auto Val = (Face->glyph->bitmap.buffer[ulPos] >= ubThreshold) ? 0xFF : 0x00;
			mGlyphs[c].vData[ulPos] = Val;
		}

		ubMaxBearing = std::max(ubMaxBearing, mGlyphs[c].ubBearing);
		ubMaxAddHeight = std::max(
			ubMaxAddHeight, static_cast<uint8_t>(mGlyphs[c].ubHeight - mGlyphs[c].ubBearing)
		);
	}

	uint8_t ubBmHeight = ubMaxBearing + ubMaxAddHeight;

	// Normalize Glyph height
	for(auto &GlyphPair: mGlyphs) {
		auto &Glyph = GlyphPair.second;
		std::vector<uint8_t> vNewData(Glyph.ubWidth * ubBmHeight, 0x00);
		auto Dst = vNewData.begin() + Glyph.ubWidth * (ubMaxBearing - Glyph.ubBearing);
		for(auto Src = Glyph.vData.begin(); Src != Glyph.vData.end(); ++Src, ++Dst) {
			*Dst = *Src;
		}
		Glyph.vData = vNewData;
		Glyph.ubHeight = ubBmHeight;
	}

	FT_Done_Face(Face);
	FT_Done_FreeType(FreeType);
	return mGlyphs;
}

bool tFontConv::glyphsToDir(
	const tFontConv::tGlyphSet &mGlyphs, const std::string &szDirPath
)
{
	for(const auto &GlyphPair: mGlyphs) {
		auto Glyph = GlyphPair.second;
		std::vector<tPixel> vImage(Glyph.ubWidth * Glyph.ubHeight, {0xFF, 0xFF, 0xFF});

		for(auto y = 0; y < Glyph.ubHeight; ++y) {
			for(auto x = 0; x < Glyph.ubWidth; ++x) {
				auto Val = Glyph.vData[y * Glyph.ubWidth + x];
				vImage[y * Glyph.ubWidth + x].r = Val;
				vImage[y * Glyph.ubWidth + x].g = Val;
				vImage[y * Glyph.ubWidth + x].b = Val;
			}
		}

		auto LodeErr = lodepng_encode_file(
			fmt::format("{}/{:d}.png", szDirPath, GlyphPair.first).c_str(),
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

void tFontConv::glyphsToLongPng(
	const tGlyphSet &mGlyphs, const std::string &szPngPath
)
{
	uint8_t ubHeight = mGlyphs.begin()->second.ubHeight;
	uint16_t uwWidth = 0;
	for(const auto &GlyphPair: mGlyphs) {
		uwWidth += GlyphPair.second.ubWidth;
	}
	std::vector<tPixel> vBitmap(uwWidth * ubHeight, {0xFF, 0xFF, 0xFF});

	uint16_t uwOffsX = 0;
	for(const auto &GlyphPair: mGlyphs) {
		const auto &Glyph = GlyphPair.second;
		for(auto y = 0; y < Glyph.ubHeight; ++y) {
			for(auto x = 0; x < Glyph.ubWidth; ++x) {
				auto Val = Glyph.vData[y * Glyph.ubWidth + x];
				uint16_t uwOffs = uwOffsX + y * uwWidth + x;
				vBitmap[uwOffs].r = Val;
				vBitmap[uwOffs].g = Val;
				vBitmap[uwOffs].b = Val;
			}
		}
		uwOffsX += Glyph.ubWidth;
	}

	lodepng_encode_file(
		szPngPath.c_str(), reinterpret_cast<uint8_t*>(vBitmap.data()),
		uwWidth, ubHeight, LCT_RGB, 8
	);
}
