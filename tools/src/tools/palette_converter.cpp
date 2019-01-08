/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "palette_converter.h"
#include <fstream>
#include <fmt/format.h>

tPaletteConverter::tPalette tPaletteConverter::fromPlt(
	const std::string &szPath
) {
	tPaletteConverter::tPalette Palette;

	std::ifstream Source(szPath.c_str(), std::ios::in | std::ios::binary);

	uint8_t ubPaletteCount;
	Source.read(reinterpret_cast<char*>(&ubPaletteCount), 1);
	fmt::print("Palette color count: {}\n", ubPaletteCount);

	for(uint8_t i = 0; i != ubPaletteCount; ++i) {
		uint8_t ubXR, ubGB;
		Source.read(reinterpret_cast<char*>(&ubXR), 1);
		Source.read(reinterpret_cast<char*>(&ubGB), 1);
		Palette.m_vColors.push_back(tRgb(
			((ubXR & 0x0F) << 4) | (ubXR & 0x0F),
			((ubGB & 0xF0) >> 4) | (ubGB & 0xF0),
			((ubGB & 0x0F) << 4) | (ubGB & 0x0F)
		));
	}
	return Palette;
}

static tPaletteConverter::tPalette fromPromotionPal(const std::string &szPath) {
	tPaletteConverter::tPalette Palette;

	std::ifstream Source(szPath.c_str(), std::ios::in | std::ios::binary);
	uint8_t ubLastNonZero = 0;
	for(uint16_t i = 0; i < 256; ++i) {
		uint8_t ubR, ubG, ubB;
		Source.read(reinterpret_cast<char*>(&ubR), 1);
		Source.read(reinterpret_cast<char*>(&ubG), 1);
		Source.read(reinterpret_cast<char*>(&ubB), 1);
		Palette.m_vColors.push_back(tRgb(ubR, ubG, ubB));
		if(ubR || ubG || ubB) {
			ubLastNonZero = i;
		}
	}

	// Palette is always 256 colors long, so now it's time to trim it
	Palette.m_vColors = std::vector<tRgb>(
		Palette.m_vColors.begin(), Palette.m_vColors.begin() + ubLastNonZero + 1
	);

	fmt::print("Palette color count: {}\n", Palette.m_vColors.size());
}

int16_t tPaletteConverter::tPalette::getColorIdx(const tRgb &Ref) const {
	uint8_t i = 0;
	for(const auto &Color: m_vColors) {
		if(Color == Ref) {
			return i;
		}
		++i;
	}
	return -1;
}
