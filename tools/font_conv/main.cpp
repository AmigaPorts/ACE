/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include "FreeTypeAmalgam.h"
#include <fmt/format.h>
#include "lodepng.h"

int main(void) {

	FT_Library FreeType;
	auto Error = FT_Init_FreeType(&FreeType);
	if(Error) {
		return 1;
	}

	FT_Face Face;
	std::string szPath = "arial.ttf";
	Error = FT_New_Face(FreeType, szPath.c_str(), 0, &Face);
	if(Error) {
		fmt::print("Couldn't open font '{}'\n", szPath);
		return 1;
	}

	FT_Set_Pixel_Sizes(Face, 0, 20);
	std::string szCharset =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	auto Threshold = 128;

	struct tChar {
		uint8_t ubBearing;
		uint8_t ubHeight;
		uint8_t ubCharWidth, ubCharHeight;
		std::vector<uint8_t> vData;
	};

	std::map<char, tChar> mGlyphs;

	uint8_t ubMaxBearing = 0, ubMaxAddHeight = 0;
	for(const auto &c: szCharset) {
		FT_Load_Char(Face, c, FT_LOAD_RENDER);

		auto CharWidth = Face->glyph->bitmap.width;
		auto CharHeight = Face->glyph->bitmap.rows;

		mGlyphs[c] = {
			static_cast<uint8_t>(Face->glyph->metrics.horiBearingY / 64),
			static_cast<uint8_t>(Face->glyph->metrics.height / 64),
			static_cast<uint8_t>(CharWidth), static_cast<uint8_t>(CharHeight),
			std::vector<uint8_t>(
				Face->glyph->bitmap.buffer,
				&Face->glyph->bitmap.buffer[CharWidth*CharHeight + 1]
			)
		};
		// fmt::print("'{}' Dimensions: {} {} bearing: {} h: {}\n", c, CharWidth, CharHeight, mGlyphs[c].lBearingY, mGlyphs[c].lHeight);
		ubMaxBearing = std::max(ubMaxBearing, mGlyphs[c].ubBearing);
		ubMaxAddHeight = std::max(
			ubMaxAddHeight, static_cast<uint8_t>(mGlyphs[c].ubHeight - mGlyphs[c].ubBearing)
		);
	}

	uint8_t ubBmHeight = ubMaxBearing + ubMaxAddHeight;

	for(const auto &GlyphPair: mGlyphs) {
		auto Glyph = GlyphPair.second;
		struct tPixel {
			uint8_t r, g, b;
		};
		std::vector<tPixel> vImage(Glyph.ubCharWidth * ubBmHeight, {0xFF, 0xFF, 0xFF});

		for(auto y = 0; y < Glyph.ubCharHeight; ++y) {
			for(auto x = 0; x < Glyph.ubCharWidth; ++x) {
				auto Val = Glyph.vData[y * Glyph.ubCharWidth + x] >= Threshold
					? 0 : 255;
				uint8_t ubDy = ubMaxBearing - Glyph.ubBearing;
				vImage[(y + ubDy) * Glyph.ubCharWidth + x].r = Val;
				vImage[(y + ubDy) * Glyph.ubCharWidth + x].g = Val;
				vImage[(y + ubDy) * Glyph.ubCharWidth + x].b = Val;
			}
		}

		auto LodeErr = lodepng_encode_file(
			fmt::format("letters/{:d}.png", GlyphPair.first).c_str(),
			reinterpret_cast<uint8_t*>(vImage.data()),
			Glyph.ubCharWidth, ubBmHeight, LCT_RGB, 8
		);

		if(LodeErr) {
			fmt::print("Lode Err: {}", LodeErr);
		}
	}


	fmt::print("All done!\n");

	FT_Done_Face(Face);
	FT_Done_FreeType(FreeType);

	return 0;
}
