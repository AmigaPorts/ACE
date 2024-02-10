/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_GLYPH_SET_H_
#define _ACE_TOOLS_COMMON_GLYPH_SET_H_

#include <vector>
#include <map>
#include <cstdint>
#include "bitmap.h"

class tGlyphSet {
public:
	static tGlyphSet fromTtf(
		const std::string &szTtfPath, std::uint8_t ubSize, const std::string &szCharSet,
		std::uint8_t ubThreshold
	);

	/**
	 * @brief Creates glyph set based on ProMotion NG's PNG font file.
	 *
	 * @param szPngPath Path to png file saved in PMNG's way of saving fonts.
	 * @param ubStartIdx ASCII index of first glyph in PMNG file.
	 * @return Glyph set filled with characters from file.
	 */
	static tGlyphSet fromPmng(const std::string &szPngPath, std::uint8_t ubStartIdx = 33);

	/**
	 * @brief Creates glyph set based on ACE font (.fnt) file.
	 *
	 * @param szFntPath Path to fnt file.
	 * @return Glyph set filled with characters from file.
	 */
	static tGlyphSet fromAceFont(const std::string &szFntPath);

	static tGlyphSet fromDir(const std::string &szDirPath);

	bool toDir(const std::string &szDirPath);

	void toAceFont(const std::string &szFontPath);

	tChunkyBitmap toPackedBitmap(bool isPmng);

	bool isOk(void);

	void remapGlyphs(const std::vector<std::pair<uint32_t, uint32_t>> &vFromTo);

private:
	struct tBitmapGlyph {
		std::uint8_t m_ubBearing;
		std::uint8_t m_ubWidth, m_ubHeight;
		std::vector<uint8_t> m_vData; ///< One byte per pixel, 0 for bg, 0xFF otherwise.

		void trimHorz(bool isRight);

		std::uint8_t getValueAt(std::uint8_t ubX, std::uint8_t ubY);
		bool isEmpty(void);
		bool hasEmptyColumn(std::uint8_t ubX);
	};

	std::map<uint16_t, tBitmapGlyph> m_mGlyphs;
};

#endif // _ACE_TOOLS_COMMON_GLYPH_SET_H_
