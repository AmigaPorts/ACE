/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_TOOLS_BITMAP_CONVERTER_H_
#define _ACE_TOOLS_TOOLS_BITMAP_CONVERTER_H_

#include <cstdint>
#include <vector>
#include <string>
#include "../common/rgb.h"
#include "palette_converter.h"

class tPlanarBitmap;

class tChunkyBitmap {
public:
	uint16_t m_uwWidth = 0;
	uint16_t m_uwHeight = 0;
	std::vector<tRgb> m_vData;

	tChunkyBitmap(
		const tPlanarBitmap &Planar, const tPalette &vPalette
	);

	tChunkyBitmap(uint16_t uwWidth, uint16_t uwHeight, const uint8_t *pData);

	tChunkyBitmap(void) { };

	bool toPng(const std::string &szPngPath);

	static tChunkyBitmap fromPng(const std::string &szPath);

	static tChunkyBitmap get1bppMask(
		const tChunkyBitmap &Src, const tRgb &ColorTransparent, bool isInterleaved
	);
};

class tPlanarBitmap {
public:
	uint16_t m_uwWidth;
	uint16_t m_uwHeight;
	uint8_t m_ubDepth;
	std::vector<uint16_t> m_pPlanes[8];

	tPlanarBitmap(
		const tChunkyBitmap &Chunky, const tPalette &Palette,
		const tPalette &PaletteIgnore
	);

	void toBm(const std::string &szPath, bool isInterleaved);
};

#endif // _ACE_TOOLS_TOOLS_BITMAP_CONVERTER_H_
