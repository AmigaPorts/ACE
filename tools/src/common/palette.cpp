/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "palette.h"
#include <fstream>
#include <sstream>
#include "fs.h"
#include <fmt/format.h>

static bool beginsWith(
	const std::string &szHaystack, const std::string &szNeedle
)
{
	if(szHaystack.length() < szNeedle.length()) {
		return false;
	}
	return szHaystack.substr(0, szNeedle.length()) == szNeedle;
}

tPalette tPalette::fromGpl(const std::string &szPath)
{
	tPalette Palette;
	std::ifstream Source(szPath, std::ios::in);

	// Skip header
	std::string szLine;
	do {
	std::getline(Source, szLine);
	} while(!Source.eof() && (
		beginsWith(szLine, "GIMP Palette") ||
		beginsWith(szLine, "Name:") ||
		beginsWith(szLine, "Columns:") ||
		beginsWith(szLine, "#")
	));

	// Read colors
	bool isEnd = false;
	bool isNextEnd = false;
	do {
		std::stringstream ss(szLine);
		int r, g, b;
		ss >> r;
		ss >> g;
		ss >> b;
		tRgb Color(r, g, b);
		Palette.m_vColors.push_back(Color);

		std::getline(Source, szLine);
		if(isNextEnd || szLine == "") {
			isEnd = true;
		}
		if(Source.eof())
			isNextEnd=true;
	} while(!isEnd);

	fmt::print("Palette color count: {}\n", Palette.m_vColors.size());
	return Palette;
}

tPalette tPalette::fromPlt(const std::string &szPath)
{
	tPalette Palette;

	std::ifstream Source(szPath, std::ios::in | std::ios::binary);

	std::uint8_t ubPaletteCount;
	Source.read(reinterpret_cast<char*>(&ubPaletteCount), 1);
	fmt::print("Palette color count: {}\n", ubPaletteCount);

	for(std::uint8_t i = 0; i != ubPaletteCount; ++i) {
		std::uint8_t ubXR, ubGB;
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

tPalette tPalette::fromPromotionPal(const std::string &szPath)
{
	tPalette Palette;

	std::ifstream Source(szPath, std::ios::in | std::ios::binary);
	std::uint16_t uwLastNonZero = 0;
	for(std::uint16_t i = 0; i < 256; ++i) {
		std::uint8_t ubR, ubG, ubB;
		Source.read(reinterpret_cast<char*>(&ubR), 1);
		Source.read(reinterpret_cast<char*>(&ubG), 1);
		Source.read(reinterpret_cast<char*>(&ubB), 1);
		Palette.m_vColors.push_back(tRgb(ubR, ubG, ubB));
		if(ubR || ubG || ubB) {
			uwLastNonZero = i;
		}
	}

	// Palette is always 256 colors long, so now it's time to trim it
	Palette.m_vColors = std::vector<tRgb>(
		Palette.m_vColors.begin(), Palette.m_vColors.begin() + uwLastNonZero + 1
	);

	fmt::print("Palette color count: {}\n", Palette.m_vColors.size());
	return Palette;
}

tPalette tPalette::fromAct(const std::string &szPath)
{
	tPalette Palette;

	std::ifstream Source(szPath, std::ios::in | std::ios::binary);
	for(std::uint16_t i = 0; i < 256; ++i) {
		std::uint8_t ubR, ubG, ubB;
		Source.read(reinterpret_cast<char*>(&ubR), 1);
		Source.read(reinterpret_cast<char*>(&ubG), 1);
		Source.read(reinterpret_cast<char*>(&ubB), 1);
		Palette.m_vColors.push_back(tRgb(ubR, ubG, ubB));
	}

	// Read color count
	std::uint8_t ubSizeHi, ubSizeLo;
	std::uint16_t uwSize;
	Source.read(reinterpret_cast<char*>(&ubSizeHi), 1);
	Source.read(reinterpret_cast<char*>(&ubSizeLo), 1);
	uwSize = (ubSizeHi << 8) | ubSizeLo;

	// Trim palette
	Palette.m_vColors = std::vector<tRgb>(
		Palette.m_vColors.begin(), Palette.m_vColors.begin() + uwSize
	);
	fmt::print("Palette color count: {}\n", Palette.m_vColors.size());
	return Palette;
}

tPalette tPalette::fromFile(const std::string &szPath)
{
	std::string szExtIn = nFs::getExt(szPath);
	tPalette Palette;
	if(szExtIn == "gpl") {
		Palette = tPalette::fromGpl(szPath);
	}
	else if(szExtIn == "act") {
		// Looks like it's same as promotion
		Palette = tPalette::fromAct(szPath);
	}
	else if(szExtIn == "pal") {
		Palette = tPalette::fromPromotionPal(szPath);
	}
	else if(szExtIn == "plt") {
		Palette = tPalette::fromPlt(szPath);
	}
	return Palette;
}

bool tPalette::toPlt(const std::string &szPath, bool isForceOcs)
{
	std::ofstream Dest(szPath, std::ios::out | std::ios::binary);
	if(!Dest.is_open()) {
		return false;
	}
	auto PaletteSize = m_vColors.size();
	Dest.write(reinterpret_cast<char*>(&PaletteSize), 1);
	for(std::uint16_t uwColorIdx = 0; uwColorIdx < PaletteSize; ++uwColorIdx) {
		const auto &Color = m_vColors[uwColorIdx];
		if(isForceOcs) {
			const auto &ColorOcs = Color.to12Bit();
			if(ColorOcs != Color) {
				throw std::runtime_error(fmt::format(
					FMT_STRING(
						"Color at index {} ({}) is not suited for OCS. "
						"Expected 4-bit channels, e.g. {}"
					),
					uwColorIdx, Color.toString(), ColorOcs.toString()
				));
			}
		}

		std::uint8_t ubXR = Color.ubR >> 4;
		std::uint8_t ubGB = ((Color.ubG >> 4) << 4) | (Color.ubB >> 4);
		Dest.write(reinterpret_cast<char*>(&ubXR), 1);
		Dest.write(reinterpret_cast<char*>(&ubGB), 1);
	}
	return true;
}

bool tPalette::toGpl(const std::string &szPath)
{
	using namespace nFs;
	std::ofstream Dest(szPath, std::ios::out);
	if(!Dest.is_open()) {
		return false;
	}

	// Header
	Dest << "GIMP Palette\n";
	Dest << fmt::format("Name: {}\n", removeExt(getBaseName(szPath)));
	Dest << "Columns: 4\n";
	Dest << "#\n";

	// Colors
	for(auto i = 0; i < m_vColors.size(); ++i) {
		const auto &Color = m_vColors.at(i);
		Dest << fmt::format(
			"{:3d} {:3d} {:3d} Index {}\n", Color.ubR, Color.ubG, Color.ubB, i
		);
	}

	return true;
}

bool tPalette::toPromotionPal(const std::string &szPath)
{
	std::ofstream Dest(szPath, std::ios::out | std::ios::binary);
	if(!Dest.is_open()) {
		return false;
	}

	std::uint32_t i;
	for(i = 0; i < m_vColors.size(); ++i) {
		const auto &Color = m_vColors.at(i);
		Dest.write(reinterpret_cast<const char*>(&Color.ubR), 1);
		Dest.write(reinterpret_cast<const char*>(&Color.ubG), 1);
		Dest.write(reinterpret_cast<const char*>(&Color.ubB), 1);
	}
	const char pBlank[3] = {0};
	while(i < 256) {
		Dest.write(pBlank, 3);
		++i;
	}

	return true;
}

bool tPalette::toAct(const std::string &szPath)
{
	std::ofstream Dest(szPath, std::ios::out | std::ios::binary);
	if(!Dest.is_open()) {
		return false;
	}

	std::uint32_t i;
	for(i = 0; i < m_vColors.size(); ++i) {
		const auto &Color = m_vColors.at(i);
		Dest.write(reinterpret_cast<const char*>(&Color.ubR), 1);
		Dest.write(reinterpret_cast<const char*>(&Color.ubG), 1);
		Dest.write(reinterpret_cast<const char*>(&Color.ubB), 1);
	}
	const char pBlank[3] = {0};
	while(i < 256) {
		Dest.write(pBlank, 3);
		++i;
	}
	std::uint8_t ubSizeHi = uint16_t(m_vColors.size()) >> 8;
	std::uint8_t ubSizeLo = m_vColors.size() & 0xFF;

	Dest.write(reinterpret_cast<char*>(&ubSizeHi), 1);
	Dest.write(reinterpret_cast<char*>(&ubSizeLo), 1);

	return true;
}

std::int16_t tPalette::getColorIdx(const tRgb &Ref) const
{
	std::uint8_t i = 0;
	for(const auto &Color: m_vColors) {
		if(Color == Ref) {
			return i;
		}
		++i;
	}
	return -1;
}

bool tPalette::isValid(void) const {
	return m_vColors.size() != 0;
}

std::uint8_t tPalette::getBpp(void) const {
	std::uint8_t ubBpp = 1;
	for(std::size_t i = 2; i < m_vColors.size(); i <<= 1) {
		++ubBpp;
	}
	return ubBpp;
}

bool tPalette::convertToEhb(void)
{
	if(m_vColors.size() > 32) {
		return false;
	}

	m_vColors.resize(64);
	for(std::size_t i = 0; i < 32; ++i) {
		m_vColors[i + 32] = m_vColors[i].toEhb();
	}
	return true;
}
