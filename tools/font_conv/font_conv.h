/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_FONT_CONV_H_
#define _ACE_TOOLS_FONT_CONV_H_

#include <vector>
#include <map>
#include <cstdint>

class tFontConv {
public:

	struct tBitmapGlyph {
		uint8_t ubBearing;
		uint8_t ubWidth, ubHeight;
		std::vector<uint8_t> vData;

		void trimHorz(bool isRight);
	};

	using tGlyphSet = std::map<char, tBitmapGlyph>;

	tFontConv(void);
	~tFontConv(void);

	std::map<char, tBitmapGlyph> glyphsFromTtf(
		const std::string &szTtfPath, uint8_t ubSize, const std::string &szCharSet,
		uint8_t ubThreshold
	);

	std::map<char, tBitmapGlyph> glyphsFromDir(const std::string &szDirPath);

	bool glyphsToDir(
		const tGlyphSet &mGlyphs, const std::string &szDirPath
	);

	void glyphsToAceFont(
		const std::string &szFontPath, const tGlyphSet &mGlyphs
	);

	void glyphsToLongPng(const tGlyphSet &mGlyphs, const std::string &szPngPath);
};


#endif // _ACE_TOOLS_FONT_CONV_H_
