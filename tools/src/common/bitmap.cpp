/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bitmap.h"
#include <fstream>
#include <algorithm>
#include "../common/logging.h"
#include "../common/lodepng.h"
#include "../common/endian.h"
#include "../common/flags/flags.hpp"

enum class tBmFlags: std::uint8_t {
	NONE = 0,
	INTERLEAVED = 1
};
ALLOW_FLAGS_FOR_ENUM(tBmFlags);

tChunkyBitmap::tChunkyBitmap(
	const tPlanarBitmap &Planar, const tPalette &Palette
):
	m_uwWidth(Planar.m_uwWidth), m_uwHeight(Planar.m_uwHeight)
{
	if(!Planar.m_pPlanes[0].size()) {
		m_uwWidth = 0;
		m_uwHeight = 0;
		return;
	}
	m_vData.resize(m_uwWidth * m_uwHeight, Palette.m_vColors[0]);
	auto PxPerCell = sizeof(Planar.m_pPlanes[0][0]) * 8;
	for(std::uint32_t ulY = 0; ulY < m_uwHeight; ++ulY) {
		for(std::uint32_t ulX = 0; ulX < m_uwWidth; ++ulX) {
			std::uint8_t ubColorIdx = 0;
			std::uint32_t ulOffs = (ulY * m_uwWidth + ulX) / PxPerCell;
			for(std::uint8_t ubPlane = Planar.m_ubDepth; ubPlane--;) {
				auto &Plane = Planar.m_pPlanes[ubPlane];
				ubColorIdx <<= 1;
				ubColorIdx |= (Plane.at(ulOffs) >> (15 - (ulX & 15))) & 1;
			}
			if(ubColorIdx >= Palette.m_vColors.size()) {
				nLog::error(
					"Attempted to read color {} from palette of size {}",
					ubColorIdx, Palette.m_vColors.size()
				);
				m_uwWidth = 0;
				m_uwHeight = 0;
				return;
			}
			auto Color = Palette.m_vColors.at(ubColorIdx);
			this->pixelAt(ulX, ulY) = Color;
		}
	}
}

tChunkyBitmap::tChunkyBitmap(std::uint16_t uwWidth, std::uint16_t uwHeight, tRgb Bg):
	m_uwWidth(uwWidth), m_uwHeight(uwHeight), m_vData(m_uwWidth * m_uwHeight, Bg)
{

}

tChunkyBitmap::tChunkyBitmap(
	std::uint16_t uwWidth, std::uint16_t uwHeight, const std::uint8_t *pData
):
	m_uwWidth(uwWidth), m_uwHeight(uwHeight)
{
	auto *pRgbData = reinterpret_cast<const tRgb*>(pData);
	auto *pRgbDataEnd = &pRgbData[uwWidth * uwHeight];
	m_vData = std::vector<tRgb>(pRgbData, pRgbDataEnd);
}

tChunkyBitmap tChunkyBitmap::fromPng(const std::string &szPath)
{
	unsigned uWidth, uHeight;
	std::uint8_t *pData;
	auto LodeError = lodepng_decode24_file(&pData, &uWidth, &uHeight, szPath.c_str());
	if(LodeError) {
		return tChunkyBitmap();
	}

	tChunkyBitmap Chunky(uWidth, uHeight, pData);
	free(pData);
	return Chunky;
}

bool tChunkyBitmap::toPng(const std::string &szPngPath) const
{
	auto LodeErr = lodepng_encode_file(
		szPngPath.c_str(), reinterpret_cast<const uint8_t*>(m_vData.data()),
		m_uwWidth, m_uwHeight, LCT_RGB, 8
	);

	return LodeErr == 0;
}

tPlanarBitmap::tPlanarBitmap(
	std::uint16_t uwWidth, std::uint16_t uwHeight, std::uint8_t ubDepth
)
{
	if(uwWidth & 0xF) {
		nLog::error("Width is not divisible by 16");
		return;
	}
	if(ubDepth > 8) {
		nLog::error("More than 8bpp not supported, got {}", m_ubDepth);
		return;
	}

	m_uwWidth = uwWidth;
	m_uwHeight = uwHeight;
	m_ubDepth = ubDepth;

	for(std::uint8_t i = 0; i < ubDepth; ++i) {
		m_pPlanes[i].resize(uwWidth * uwHeight / sizeof(m_pPlanes[0][0]));
	}
}

tPlanarBitmap::tPlanarBitmap(
	const tChunkyBitmap &Chunky, const tPalette &Palette,
	const tPalette &PaletteIgnore
):
	m_uwWidth(0),
	m_uwHeight(0),
	m_ubDepth(0)
{
	if(Chunky.m_uwWidth & 0xF) {
		nLog::error("Width is not divisible by 16");
		return;
	}

	// Determine depth
	std::uint8_t ubDepth = Palette.getBpp();
	if(ubDepth > 8) {
		nLog::error("More than 8bpp not supported, got {}", ubDepth);
		return;
	}

	// Write bitplanes - from LSB to MSB
	std::uint16_t uwPixelBuffer;
	std::uint32_t ulPos;
	for(std::uint8_t ubPlane = 0; ubPlane != ubDepth; ++ubPlane) {
		for(std::uint16_t y = 0; y != Chunky.m_uwHeight; ++y) {
			uwPixelBuffer = 0;
			for(std::uint16_t x = 0; x != Chunky.m_uwWidth; ++x) {
				// Determine bit value for given color at specified bitplane's pos
				auto Color = Chunky.pixelAt(x, y);
				std::int16_t wIdx = Palette.getColorIdx(Color);
				std::uint8_t ubBit = 0;
				if(wIdx == -1) {
					if(PaletteIgnore.getColorIdx(Color) == -1) {
						nLog::error(
							"Unexpected color: {0}, {1}, {2} (#{0:02X}{1:02X}{2:02X}) @{3},{4}",
							Color.ubR, Color.ubG,	Color.ubB, x, y
						);
						return;
					}
				}
				else if(wIdx & (1 << ubPlane)) {
					ubBit = 1;
				}

				uwPixelBuffer <<= 1;
				uwPixelBuffer |= ubBit;
				if((x & 0xF) == 0xF) {
					m_pPlanes[ubPlane].push_back(uwPixelBuffer);
				}
			}
		}
	}

	// Everything's okay - write dimensions to apropriate fields
	m_uwWidth = Chunky.m_uwWidth;
	m_uwHeight = Chunky.m_uwHeight;
	m_ubDepth = ubDepth;
}

bool tPlanarBitmap::toBm(const std::string &szPath, bool isInterleaved)
{
	flags::flags<tBmFlags> eFlags(tBmFlags::NONE);
	if(isInterleaved) {
		eFlags |= tBmFlags::INTERLEAVED;
	}

	std::ofstream OutFile(szPath.c_str(), std::ios::out | std::ios::binary);
	if(!OutFile.is_open()) {
		return false;
	}

	// Write .bm header
	std::uint16_t uwOut = nEndian::toBig16(m_uwWidth);
	OutFile.write(reinterpret_cast<char*>(&uwOut), 2);
	uwOut = nEndian::toBig16(m_uwHeight);
	OutFile.write(reinterpret_cast<char*>(&uwOut), 2);
	OutFile.write(reinterpret_cast<char*>(&m_ubDepth), 1);

	std::uint8_t ubOut = 0;
	OutFile.write(reinterpret_cast<char*>(&ubOut), 1); // Version
	OutFile.write(reinterpret_cast<char*>(&eFlags), 1); // Flags
	OutFile.write(reinterpret_cast<char*>(&ubOut), 1); // Reserved 1
	OutFile.write(reinterpret_cast<char*>(&ubOut), 1); // Reserved 2

	// Write bitplanes
	std::uint16_t uwRowWordCount = m_uwWidth / 16;
	if(isInterleaved) {
		for(std::uint16_t y = 0; y < m_uwHeight; ++y) {
			for(std::uint8_t ubPlane = 0; ubPlane < m_ubDepth; ++ubPlane) {
				for(std::uint16_t x = 0; x < uwRowWordCount; ++x) {
					std::uint16_t uwData = nEndian::toBig16(
						m_pPlanes[ubPlane].at(y * uwRowWordCount + x)
					);
					OutFile.write(reinterpret_cast<char*>(&uwData), sizeof(uint16_t));
				}
			}
		}
	}
	else {
		for(std::uint8_t ubPlane = 0; ubPlane < m_ubDepth; ++ubPlane) {
			for(std::uint16_t y = 0; y < m_uwHeight; ++y) {
				for(std::uint16_t x = 0; x < uwRowWordCount; ++x) {
					std::uint16_t uwData = nEndian::toBig16(
						m_pPlanes[ubPlane].at(y * uwRowWordCount + x)
					);
					OutFile.write(reinterpret_cast<char*>(&uwData), sizeof(uint16_t));
				}
			}
		}
	}
	OutFile.close();
	return true;
}

tPlanarBitmap tPlanarBitmap::fromBm(const std::string &szPath)
{
	std::ifstream File(szPath, std::ios::in | std::ios::binary);
	if(!File.is_open()) {
		return tPlanarBitmap(0, 0, 0);
	}

	std::uint16_t uwWidth, uwHeight;
	std::uint8_t ubBpp, ubVersion, ubReserved1, ubReserved2;
	tBmFlags eFlags;
	File.read(reinterpret_cast<char*>(&uwWidth), sizeof(uwWidth));
	File.read(reinterpret_cast<char*>(&uwHeight), sizeof(uwHeight));
	File.read(reinterpret_cast<char*>(&ubBpp), sizeof(ubBpp));
	File.read(reinterpret_cast<char*>(&ubVersion), sizeof(ubVersion));
	File.read(reinterpret_cast<char*>(&eFlags), sizeof(eFlags));
	File.read(reinterpret_cast<char*>(&ubReserved1), sizeof(ubReserved1));
	File.read(reinterpret_cast<char*>(&ubReserved2), sizeof(ubReserved2));

	uwWidth = nEndian::fromBig16(uwWidth);
	uwHeight = nEndian::fromBig16(uwHeight);

	if(ubVersion == 0) {
		tPlanarBitmap Bm(uwWidth, uwHeight, ubBpp);
		for(std::uint8_t i = 0; i < ubBpp; ++i) {
			Bm.m_pPlanes[i].resize(uwWidth * uwHeight / sizeof(Bm.m_pPlanes[0][0]));
		}
		if(eFlags & tBmFlags::INTERLEAVED) {
			for(std::uint32_t y = 0; y < uwHeight; ++y) {
				for(std::uint8_t i = 0; i < ubBpp; ++i) {
					File.read(
						&(reinterpret_cast<char*>(Bm.m_pPlanes[i].data())[y * uwWidth / 8]),
						uwWidth / 8
					);
				}
			}
		}
		else {
			for(std::uint8_t i = 0; i < ubBpp; ++i) {
				File.read(
					reinterpret_cast<char*>(Bm.m_pPlanes[i].data()),
					(uwWidth / 8) * uwHeight
				);
			}
		}

		// Convert endianness on data
		for(std::uint8_t i = ubBpp; i--;) {
			for(auto &Cell: Bm.m_pPlanes[i]) {
				Cell = nEndian::fromBig16(Cell);
			}
		}

		Bm.m_uwWidth = uwWidth;
		Bm.m_uwHeight = uwHeight;
		Bm.m_ubDepth = ubBpp;
		return Bm;
	}
	else {
		nLog::error("Unsupported bitmap file version: 0x{:02X}", ubVersion);
		return tPlanarBitmap(0, 0, 0);
	}
}

tRgb &tChunkyBitmap::pixelAt(std::uint16_t uwX, std::uint16_t uwY)
{
	std::uint32_t ulPos = m_uwWidth * (uwY) + uwX;
	return m_vData[ulPos];
}

const tRgb &tChunkyBitmap::pixelAt(std::uint16_t uwX, std::uint16_t uwY) const
{
	std::uint32_t ulPos = m_uwWidth * (uwY) + uwX;
	return m_vData[ulPos];
}

bool tChunkyBitmap::copyRect(
	std::uint16_t uwSrcX, std::uint16_t uwSrcY, tChunkyBitmap &Dst,
	std::uint16_t uwDstX, std::uint16_t uwDstY, std::uint16_t uwWidth, std::uint16_t uwHeight
) const
{
	if(uwSrcX + uwWidth > m_uwWidth || uwSrcY + uwHeight > m_uwHeight) {
		// Source out of range
		return false;
	}
	if(uwDstX + uwWidth > Dst.m_uwWidth || uwDstY + uwHeight > Dst.m_uwHeight) {
		// Dest out of range
		return false;
	}

	for(std::uint16_t uwY = 0; uwY < uwHeight; ++uwY) {
		for(std::uint16_t uwX = 0; uwX < uwWidth; ++uwX) {
			Dst.pixelAt(uwDstX + uwX, uwDstY + uwY) = pixelAt(uwSrcX + uwX, uwSrcY + uwY);
		}
	}

	return true;
}

bool tChunkyBitmap::fillRect(
	std::uint16_t uwDstX, std::uint16_t uwDstY, std::uint16_t uwWidth, std::uint16_t uwHeight,
	const tRgb &Color
) {
	if(uwDstX + uwWidth > m_uwWidth || uwDstY + uwHeight > m_uwHeight) {
		// Source out of range
		return false;
	}

	for(std::uint16_t uwY = 0; uwY < uwHeight; ++uwY) {
		for(std::uint16_t uwX = 0; uwX < uwWidth; ++uwX) {
			pixelAt(uwDstX + uwX, uwDstY + uwY) = Color;
		}
	}

	return true;
}

bool tChunkyBitmap::mergeWithMask(const tChunkyBitmap &Mask)
{
	if(Mask.m_uwHeight != m_uwHeight || Mask.m_uwWidth != m_uwWidth) {
		return false;
	}

	for(std::uint16_t uwY = 0; uwY < m_uwHeight; ++uwY) {
		for(std::uint16_t uwX = 0; uwX < m_uwWidth; ++uwX) {
			auto & MaskPixel = Mask.pixelAt(uwX, uwY);
			if(MaskPixel != tRgb(0)) {
				pixelAt(uwX, uwY) = MaskPixel;
			}
		}
	}
	return true;
}

tChunkyBitmap tChunkyBitmap::filterColors(
	const tPalette &Palette, const tRgb &ColorDefault
)
{
	const auto &Colors = Palette.m_vColors;
	tChunkyBitmap Out(m_uwWidth, m_uwHeight);
	for(std::uint16_t uwY = 0; uwY < m_uwHeight; ++uwY) {
		for(std::uint16_t uwX = 0; uwX < m_uwWidth; ++uwX) {
			const auto &Ref = pixelAt(uwX, uwY);
			if(std::find(Colors.begin(), Colors.end(), Ref) != Colors.end()) {
				Out.pixelAt(uwX, uwY) = Ref;
			}
			else {
				Out.pixelAt(uwX, uwY) = ColorDefault;
			}
		}
	}
	return Out;
}
