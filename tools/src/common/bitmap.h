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
	std::uint16_t m_uwWidth = 0;
	std::uint16_t m_uwHeight = 0;
	std::vector<tRgb> m_vData;

	tChunkyBitmap(const tPlanarBitmap &Planar, const tPalette &Palette);

	tChunkyBitmap(std::uint16_t uwWidth, std::uint16_t uwHeight, tRgb Bg = tRgb(0));

	tChunkyBitmap(std::uint16_t uwWidth, std::uint16_t uwHeight, const std::uint8_t *pData);

	tChunkyBitmap(void) { };

	bool toPng(const std::string &szPngPath) const;

	static tChunkyBitmap fromPng(const std::string &szPath);

	tRgb &pixelAt(std::uint16_t uwX, std::uint16_t uwY);
	const tRgb &pixelAt(std::uint16_t uwX, std::uint16_t uwY) const;

	bool copyRect(
		std::uint16_t uwSrcX, std::uint16_t uwSrcY, tChunkyBitmap &Dst,
		std::uint16_t uwDstX, std::uint16_t uwDstY, std::uint16_t uwWidth, std::uint16_t uwHeight
	) const;

	bool fillRect(
		std::uint16_t uwDstX, std::uint16_t uwDstY, std::uint16_t uwWidth, std::uint16_t uwHeight,
		const tRgb &Color
	);

	bool mergeWithMask(const tChunkyBitmap &Mask);

	tChunkyBitmap filterColors(const tPalette &Palette, const tRgb &ColorDefault);
};

class tPlanarBitmap {
public:
	std::uint16_t m_uwWidth;
	std::uint16_t m_uwHeight;
	std::uint8_t m_ubDepth;
	std::vector<uint16_t> m_pPlanes[8];

	tPlanarBitmap(
		const tChunkyBitmap &Chunky, const tPalette &Palette,
		const tPalette &PaletteIgnore = tPalette()
	);

	tPlanarBitmap(std::uint16_t uwWidth, std::uint16_t uwHeight, std::uint8_t ubDepth);

	bool toBm(const std::string &szPath, bool isInterleaved);

	static tPlanarBitmap fromBm(const std::string &szPath);
};

#endif // _ACE_TOOLS_COMMON_BITMAP_H_
