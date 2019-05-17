/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include "common/logging.h"
#include "common/glyph_set.h"
#include "common/fs.h"

static const std::string s_szDefaultCharset = (
	" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
);

enum class tOutType: uint8_t {
	INVALID,
	TTF,
	DIR,
	PNG,
	FNT,
};

std::map<std::string, tOutType> s_mOutType = {
	{"ttf", tOutType::TTF},
	{"dir", tOutType::DIR},
	{"png", tOutType::PNG},
	{"fnt", tOutType::FNT}
};

void printUsage(const std::string &szAppName) {
	using fmt::print;
	print("Usage:\n\t{} inPath outType [extraOpts]\n\n", szAppName);
	print("inPath\t- path to TTF file or directory with PNG glyphs\n");
	print("outType\t- one of following:\n");
	print("\tdir\tDirectory with each glyph as separate PNG\n");
	print("\tpng\tSingle PNG file with whole font\n");
	print("\tfnt\tACE font file\n");
	print("extraOpts:\n");
	// -chars
	print("\t-chars \"AB D\"\tInclude only 'A', 'B', ' ' and 'D' chars in font. Only for TTF input.\n");
	print("\t\t\tUse backslash (\\) to escape quote (\") char inside charset specifier\n");
	print("\t\t\tDefault charset is: \"{}\"\n", s_szDefaultCharset);
	// -out
	print("\t-out outPath\tSpecify output path, including file name.\n");
	print("\t\t\tDefault is same name as input with changed extension\n");
}

int main(int lArgCount, const char *pArgs[])
{
	const uint8_t ubMandatoryArgCnt = 2;
	// Mandatory args
	if(lArgCount - 1 < ubMandatoryArgCnt) {
		nLog::error("Too few arguments, expected {}", ubMandatoryArgCnt);
		printUsage(pArgs[0]);
		return 1;
	}

	std::string szFontPath = pArgs[1];
	tOutType eOutType = s_mOutType[pArgs[2]];

	// Optional args' default values
	std::string szCharset = s_szDefaultCharset;
	std::string szOutPath = "";

	// Search for optional args
	for(auto i = ubMandatoryArgCnt+1; i < lArgCount; ++i) {
		if(pArgs[i] == std::string("-chars") && i < lArgCount - 1) {
			++i;
			szCharset = pArgs[i];
		}
		else if(pArgs[i] == std::string("-out") && i < lArgCount - 1) {
			++i;
			szOutPath = pArgs[i];
		}
		else {
			nLog::error("Unknown arg or missing value: '{}'", pArgs[i]);
			printUsage(pArgs[0]);
			return 1;
		}
	}

	// Load glyphs from input file
	tGlyphSet mGlyphs;
	tOutType eInType = tOutType::INVALID;
	if(szFontPath.find(".ttf") != std::string::npos) {
		mGlyphs = tGlyphSet::fromTtf(szFontPath, 20, szCharset, 128);
		eInType = tOutType::TTF;
	}
	else if(nFs::isDir(szFontPath)) {
		mGlyphs = tGlyphSet::fromDir(szFontPath);
		if(!mGlyphs.isOk()) {
			nLog::error("Loading glyphs from dir '{}' failed", szFontPath);
			return 1;
		}
		eInType = tOutType::DIR;
	}
	else {
		nLog::error("Unsupported font source: '{}'", pArgs[1]);
		return 1;
	}
	if(eInType == tOutType::INVALID || !mGlyphs.isOk()) {
		nLog::error("Couldn't read any font glyphs");
		return 1;
	}

	// Determine default output path
	if(szOutPath == "") {
		szOutPath = szFontPath;
		auto PosDot = szOutPath.find_last_of(".");
		if(PosDot != std::string::npos) {
			szOutPath = szOutPath.substr(0, PosDot);
		}
	}
	if(eInType == eOutType) {
		nLog::error("Output file type can't be same as input");
		return 1;
	}

	if(eOutType == tOutType::DIR) {
		if(szOutPath == szFontPath) {
			szOutPath += ".dir";
		}
		mGlyphs.toDir(szOutPath);
	}
	else {
		tChunkyBitmap FontChunky = mGlyphs.toPackedBitmap();
		if(eOutType == tOutType::PNG) {
			if(szOutPath.substr(szOutPath.length() - 4) != ".png") {
				szOutPath += ".png";
			}
			FontChunky.toPng(szOutPath);
		}
		else if(eOutType == tOutType::FNT) {
			tPlanarBitmap FontPlanar(FontChunky, tPalette({
				tRgb(0xFF), tRgb(0x00)
			}), tPalette());
			if(szOutPath.substr(szOutPath.length() - 4) != ".fnt") {
				szOutPath += ".fnt";
			}
			mGlyphs.toAceFont(szOutPath);
		}
		else {
			nLog::error("Unsupported output type");
			return 1;
		}
	}
	fmt::print("All done!\n");
	return 0;
}
