/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <fmt/format.h>
#include "font_conv.h"

int main(void)
{
	std::string szCharset =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

	tFontConv FontConv;
	auto mGlyphs = FontConv.glyphsFromTtf("./arial.ttf", 20, szCharset);
	FontConv.glyphsToDir(mGlyphs, "./letters", 128);
	FontConv.glyphsToLongPng(mGlyphs, "./letters_dbg.png");

	fmt::print("All done!\n");

	return 0;
}
