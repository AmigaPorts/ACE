/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <fmt/format.h>
#include "tools/font_converter.h"

int main(void)
{
	std::string szCharset =
		" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

	auto mGlyphs = tGlyphSet::fromTtf("./arial.ttf", 20, szCharset, 128);
	// auto mGlyphs = tGlyphSet::fromDir("./letters");
	mGlyphs.toDir("./letters");
	tChunkyBitmap FontChunky = mGlyphs.toPackedBitmap();
	FontChunky.toPng("./letters_dbg.png");
	tPlanarBitmap FontPlanar(FontChunky, tPaletteConverter::tPalette({
		tRgb(0xFF), tRgb(0x00)
	}), tPaletteConverter::tPalette());
	mGlyphs.toAceFont("./silkscreen.fnt");

	fmt::print("All done!\n");

	return 0;
}
