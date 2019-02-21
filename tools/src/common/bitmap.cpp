/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bitmap.h"
#include <fstream>
#include "../common/logging.h"
#include "../common/lodepng.h"
#include "../common/endian.h"

tChunkyBitmap::tChunkyBitmap(uint16_t uwWidth, uint16_t uwHeight):
	m_uwWidth(uwWidth), m_uwHeight(uwHeight), m_vData(m_uwWidth * m_uwHeight)
{

}

tChunkyBitmap::tChunkyBitmap(
	uint16_t uwWidth, uint16_t uwHeight, const uint8_t *pData
):
	m_uwWidth(uwWidth), m_uwHeight(uwHeight)
{
	auto *pRgbData = reinterpret_cast<const tRgb*>(pData);
	auto *pRgbDataEnd = &pRgbData[uwWidth * uwHeight];
	m_vData = std::vector<tRgb>(pRgbData, pRgbDataEnd);
}

tChunkyBitmap tChunkyBitmap::fromPng(const std::string &szPath) {
	unsigned uWidth, uHeight;
	uint8_t *pData;
	auto LodeError = lodepng_decode24_file(&pData, &uWidth, &uHeight, szPath.c_str());
	if(LodeError) {
		free(pData);
		return tChunkyBitmap();
	}

	tChunkyBitmap Chunky(uWidth, uHeight, pData);
	free(pData);
	return Chunky;
}

bool tChunkyBitmap::toPng(const std::string &szPngPath) {
	auto LodeErr = lodepng_encode_file(
		szPngPath.c_str(), reinterpret_cast<uint8_t*>(m_vData.data()),
		m_uwWidth, m_uwHeight, LCT_RGB, 8
	);

	return LodeErr == 0;
}

tPlanarBitmap::tPlanarBitmap(
	const tChunkyBitmap &Chunky, const tPalette &Palette,
	const tPalette &PaletteIgnore
):
	m_uwWidth(Chunky.m_uwWidth), m_uwHeight(Chunky.m_uwHeight)
{
	if(m_uwWidth & 0xF) {
		nLog::error("Width is not divisible by 16");
		return;
	}

	// Determine depth
	m_ubDepth = 1;
	for(uint8_t i = 2; i < Palette.m_vColors.size(); i <<= 1) {
		++m_ubDepth;
	}
	if(m_ubDepth > 8) {
		nLog::error("More than 8bpp not supported, got {}", m_ubDepth);
		return;
	}

	// Write bitplanes - from LSB to MSB
	uint16_t uwPixelBuffer;
	uint32_t ulPos;
	for(uint8_t ubPlane = 0; ubPlane != m_ubDepth; ++ubPlane) {
		for(uint16_t y = 0; y != m_uwHeight; ++y) {
			uwPixelBuffer = 0;
			for(uint16_t x = 0; x != m_uwWidth; ++x) {
				uwPixelBuffer <<= 1;
				auto Color = Chunky.pixelAt(x, y);

				// Determine bit value for given color at specified bitplane's pos
				int16_t wIdx = Palette.getColorIdx(Color);
				uint8_t ubBit = 0;
				if(wIdx == -1) {
					if(PaletteIgnore.getColorIdx(Color) == -1) {
						nLog::error(
							"Unexpected color: {}, {}, {} @{},{}",
							Color.ubR, Color.ubG,	Color.ubB, x, y
						);
						this->m_uwHeight = 0;
						this->m_uwWidth = 0;
						return;
					}
				}
				else if(wIdx & (1 << ubPlane)) {
					ubBit = 1;
				}

				uwPixelBuffer |= ubBit;
				if((x & 0xF) == 0xF) {
					m_pPlanes[ubPlane].push_back(uwPixelBuffer);
				}
			}
		}
	}
}


bool tPlanarBitmap::toBm(const std::string &szPath, bool isInterleaved)
{
	enum class tBmFlags: uint8_t {
		NONE = 0,
		INTERLEAVED = 1
	};
	// TODO: in future include flags.hpp for bit flag ops on enum class

	tBmFlags eFlags = tBmFlags::NONE;
	if(isInterleaved) {
		eFlags = tBmFlags::INTERLEAVED;
	}

	std::ofstream OutFile(szPath.c_str(), std::ios::out | std::ios::binary);
	if(!OutFile.is_open()) {
		return false;
	}

	// Write .bm header
	uint16_t uwOut = nEndian::toBig16(m_uwWidth);
	OutFile.write(reinterpret_cast<char*>(&uwOut), 2);
	uwOut = nEndian::toBig16(m_uwHeight);
	OutFile.write(reinterpret_cast<char*>(&uwOut), 2);
	OutFile.write(reinterpret_cast<char*>(&m_ubDepth), 1);

	uint8_t ubOut = 0;
	OutFile.write(reinterpret_cast<char*>(&ubOut), 1); // Version
	OutFile.write(reinterpret_cast<char*>(&eFlags), 1); // Flags
	OutFile.write(reinterpret_cast<char*>(&ubOut), 1); // Reserved 1
	OutFile.write(reinterpret_cast<char*>(&ubOut), 1); // Reserved 2

	// Write bitplanes
	uint16_t uwRowWordCount = m_uwWidth;
	if(isInterleaved) {
		for(uint8_t ubPlane = 0; ubPlane != m_ubDepth; ++ubPlane) {
			for(uint8_t y = 0; y != m_uwHeight; ++y) {
				for(uint8_t x = 0; x != uwRowWordCount; ++x) {
					OutFile.write(
						reinterpret_cast<char*>(m_pPlanes[ubPlane][y * uwRowWordCount + x]),
						sizeof(uint16_t)
					);
				}
			}
		}
	}
	else {
		for(uint8_t y = 0; y != m_uwHeight; ++y) {
			for(uint8_t ubPlane = 0; ubPlane != m_ubDepth; ++ubPlane) {
				for(uint8_t x = 0; x != uwRowWordCount; ++x) {
					uint16_t uwData = nEndian::toBig16(m_pPlanes[ubPlane][y * uwRowWordCount + x]);
					OutFile.write(reinterpret_cast<char*>(&uwData), sizeof(uint16_t));
				}
			}
		}
	}
	OutFile.close();
	return true;
}

tRgb &tChunkyBitmap::pixelAt(uint16_t uwX, uint16_t uwY) {
	uint32_t ulPos = m_uwWidth * (uwY) + uwX;
	return m_vData[ulPos];
}

const tRgb &tChunkyBitmap::pixelAt(uint16_t uwX, uint16_t uwY) const
{
	uint32_t ulPos = m_uwWidth * (uwY) + uwX;
	return m_vData[ulPos];
}

bool tChunkyBitmap::copyRect(
	uint16_t uwSrcX, uint16_t uwSrcY, tChunkyBitmap &Dst,
	uint16_t uwDstX, uint16_t uwDstY, uint16_t uwWidth, uint16_t uwHeight
)
{
	if(uwSrcX + uwWidth > this->m_uwWidth || uwSrcY + uwHeight > this->m_uwHeight) {
		// Source out of range
		return false;
	}
	if(uwDstX + uwWidth > Dst.m_uwWidth || uwDstY + uwHeight > Dst.m_uwHeight) {
		// Dest out of range
		return false;
	}

	for(uint16_t uwY = 0; uwY < uwHeight; ++uwY) {
		for(uint16_t uwX = 0; uwX < uwWidth; ++uwX) {
			Dst.pixelAt(uwDstX + uwX, uwDstY + uwY) = this->pixelAt(uwSrcX + uwX, uwSrcY + uwY);
		}
	}

	return true;
}
