/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_BITMAP_H_
#define _ACE_TOOLS_COMMON_BITMAP_H_

#include <cstdint>
#include <vector>
#include <string>
#include "../common/rgb.h"
#include "palette.h"

class tPlanarBitmap;

class tChunkyBitmap {
public:
	uint16_t m_uwWidth = 0;
	uint16_t m_uwHeight = 0;
	std::vector<tRgb> m_vData;

	tChunkyBitmap(const tPlanarBitmap &Planar, const tPalette &Palette);

	tChunkyBitmap(uint16_t uwWidth, uint16_t uwHeight, tRgb Bg = tRgb(0));

	tChunkyBitmap(uint16_t uwWidth, uint16_t uwHeight, const uint8_t *pData);

	tChunkyBitmap(void) { };

	bool toPng(const std::string &szPngPath) const;

	static tChunkyBitmap fromPng(const std::string &szPath);

	tRgb &pixelAt(uint16_t uwX, uint16_t uwY);
	const tRgb &pixelAt(uint16_t uwX, uint16_t uwY) const;

	bool copyRect(
		uint16_t uwSrcX, uint16_t uwSrcY, tChunkyBitmap &Dst,
		uint16_t uwDstX, uint16_t uwDstY, uint16_t uwWidth, uint16_t uwHeight
	) const;

	bool fillRect(
		uint16_t uwDstX, uint16_t uwDstY, uint16_t uwWidth, uint16_t uwHeight,
		const tRgb &Color
	);

	bool mergeWithMask(const tChunkyBitmap &Mask);

	tChunkyBitmap filterColors(const tPalette &Palette, const tRgb &ColorDefault);
};

class tPlanarBitmap {
public:
	uint16_t m_uwWidth;
	uint16_t m_uwHeight;
	uint8_t m_ubDepth;
	std::vector<uint16_t> m_pPlanes[8];

	tPlanarBitmap(
		const tChunkyBitmap &Chunky, const tPalette &Palette,
		const tPalette &PaletteIgnore = tPalette()
	);

	tPlanarBitmap(uint16_t uwWidth, uint16_t uwHeight, uint8_t ubDepth);

	bool toBm(const std::string &szPath, bool isInterleaved);

	static tPlanarBitmap fromBm(const std::string &szPath);
};

#endif // _ACE_TOOLS_COMMON_BITMAP_H_
