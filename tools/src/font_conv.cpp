/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <fmt/format.h>
#include "tools/glyph_set.h"

static const std::string s_szDefaultCharset = (
	" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
);

void printUsage(const std::string &szAppName) {
	using fmt::print;
	print("Usage:\n\t{} inPath outType [extraOpts]\n", szAppName);
	print("\ninPath\t- path to TTF file or directory with PNG glyphs\n");
	print("outType\t- one of following:\n");
	print("\tdir\tDirectory with each glyph as separate PNG\n");
	print("\tpng\tSingle PNG file with whole font\n");
	print("\tfnt\tACE font file\n");
	print("extraOpts:\n");
	// -chars
	print("\t-chars \"AB D\"\tInclude only 'A', 'B', ' ' and 'D' chars in font.\n");
	print("\t\t\tUse backslash (\\) to escape quote (\") char inside charset specifier\n");
	print("\t\t\tDefault charset is: \"{}\"\n", s_szDefaultCharset);
	// -out
	// -op
	print("\t-out outPath\tSpecify output path, including file name.\n");
	print("\t\t\tDefault is same name as input with changed extension\n");
}

int main(int lArgCount, const char *pArgs[])
{
	const uint8_t ubMandatoryArgCnt = 1;
	// Mandatory args

	auto mGlyphs = tGlyphSet::fromTtf("./arial.ttf", 20, szCharset, 128);
	// auto mGlyphs = tGlyphSet::fromDir("./letters");
	mGlyphs.toDir("./letters");
	tChunkyBitmap FontChunky = mGlyphs.toPackedBitmap();
	FontChunky.toPng("./letters_dbg.png");
	tPlanarBitmap FontPlanar(FontChunky, tPalette({
		tRgb(0xFF), tRgb(0x00)
	}), tPalette());
	mGlyphs.toAceFont("./silkscreen.fnt");

	fmt::print("All done!\n");
	return 0;
}
