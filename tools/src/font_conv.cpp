/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <fstream>
#include "common/logging.h"
#include "common/glyph_set.h"
#include "common/fs.h"
#include "common/json.h"
#include "common/utf8.h"

static const std::string s_szDefaultCharset = (
	" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
);

enum class tFontFormat: std::uint8_t {
	INVALID,
	TTF,
	DIR,
	PNG,
	FNT,
	PMNG,
};

std::map<std::string, tFontFormat> s_mOutType = {
	{"ttf", tFontFormat::TTF},
	{"dir", tFontFormat::DIR},
	{"png", tFontFormat::PNG},
	{"fnt", tFontFormat::FNT},
	{"pmng", tFontFormat::PMNG},
};

void printUsage(const std::string &szAppName) {
	using fmt::print;
	print("Usage:\n\t{} inPath outType [extraOpts]\n\n", szAppName);
	print("inPath\t- path to input. Supported types:\n");
	print("\tTTF file.\n");
	print("\tDirectory with PNG glyphs. All have to be of same height and named with ASCII indices of chars.\n");
	print("\tProMotion NG's PNG font.\n");
	print("outType\t- one of following:\n");
	print("\tdir\tDirectory with each glyph as separate PNG\n");
	print("\tpng\tSingle PNG file with whole font\n");
	print("\tfnt\tACE font file\n");
	print("extraOpts:\n");
	// -chars
	print("\t-chars \"AB D\"\t\tInclude only 'A', 'B', ' ' and 'D' chars in font. Only for TTF input.\n");
	print("\t\t\t\tUse backslash (\\) to escape quote (\") char inside charset specifier\n");
	print("\t\t\t\tDefault charset is: \"{}\"\n\n", s_szDefaultCharset);
	// -charFile
	print("\t-charfile \"file.txt\"\tInclude chars specified in file.txt.\n");
	print("\t\t\t\tNewline chars (\\r, \\n) and repeats are omitted.\n\n");
	// -size
	print("\t-size 8\t\t\tRasterize font using size of 8pt. Default: 20.\n\n");
	// -out
	print("\t-out outPath\tSpecify output path, including file name.\n");
	print("\t\t\tDefault is same name as input with changed extension\n");
	// -fc
	print("\t-fc firstchar\tSpecify first ASCII character idx in ProMotion NG font. Default: 33.");
}

static std::uint32_t getCharCodeFromTok(const tJson *pJson, std::uint16_t uwTok) {
	std::uint32_t ulVal = 0;
	const auto &Token = pJson->pTokens[uwTok];
	if(Token.type == JSMN_STRING) {
		// read unicode char and return its codepoint
		std::uint32_t ulCodepoint, ulState = 0;

		for(auto i = Token.start; i <= Token.end; ++i) {
			auto CharCode = *reinterpret_cast<const uint8_t*>(&pJson->szData[i]);
			if (
				decode(&ulState, &ulCodepoint, CharCode) != UTF8_ACCEPT ||
				ulCodepoint == '\n' || ulCodepoint == '\r'
			) {
				continue;
			}
			ulVal = ulCodepoint;
			break;
		}
	}
	else {
		// read number as it is in file and return it
		ulVal = jsonTokToUlong(pJson, uwTok);
	}
	return ulVal;
}

int main(int lArgCount, const char *pArgs[])
{
	const std::uint8_t ubMandatoryArgCnt = 2;
	// Mandatory args
	if(lArgCount - 1 < ubMandatoryArgCnt) {
		nLog::error("Too few arguments, expected {}", ubMandatoryArgCnt);
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	std::string szFontPath = pArgs[1];
	tFontFormat eOutType = s_mOutType[pArgs[2]];

	// Optional args' default values
	std::string szCharset = "";
	std::string szOutPath = "";
	std::uint8_t ubFirstChar = 33;
	std::string szRemapPath = "";
	std::int32_t lSize = -1;

	// Search for optional args
	for(auto ArgIndex = ubMandatoryArgCnt+1; ArgIndex < lArgCount; ++ArgIndex) {
		if(pArgs[ArgIndex] == std::string("-chars") && ArgIndex < lArgCount - 1) {
			++ArgIndex;
			szCharset += pArgs[ArgIndex];
		}
		else if(pArgs[ArgIndex] == std::string("-out") && ArgIndex < lArgCount - 1) {
			++ArgIndex;
			szOutPath = pArgs[ArgIndex];
		}
		else if(pArgs[ArgIndex] == std::string("-fc") && ArgIndex < lArgCount - 1) {
			++ArgIndex;
			try {
				ubFirstChar = uint8_t(std::stoul(pArgs[ArgIndex]));
			}
			catch(std::exception Ex) {
				nLog::error(
					"Couldn't parse 'first char' param: '{}', expected number", pArgs[ArgIndex]
				);
				return EXIT_FAILURE;
			}
		}
		else if(pArgs[ArgIndex] == std::string("-charFile") && ArgIndex < lArgCount - 1) {
			++ArgIndex;
			auto File = std::ifstream(pArgs[ArgIndex], std::ios::binary);
			while(!File.eof()) {
				char c;
				File.read(&c, 1);
				szCharset += c;
			}
		}
		else if(pArgs[ArgIndex] == std::string("-size") && ArgIndex < lArgCount - 1) {
			++ArgIndex;
			lSize = std::stol(pArgs[ArgIndex]);
		}
		else if(pArgs[ArgIndex] == std::string("-remap") && ArgIndex < lArgCount - 1) {
			++ArgIndex;
			szRemapPath = pArgs[ArgIndex];
		}
		else {
			nLog::error("Unknown arg or missing value: '{}'", pArgs[ArgIndex]);
			printUsage(pArgs[0]);
			return EXIT_FAILURE;
		}
	}

	if(szCharset.length() == 0) {
		szCharset = s_szDefaultCharset;
	}
	if(lSize < 0) {
		lSize = 20;
	}

	// Load glyphs from input file
	tGlyphSet mGlyphs;
	tFontFormat eInType = tFontFormat::INVALID;
	if(szFontPath.find(".ttf") != std::string::npos) {
		mGlyphs = tGlyphSet::fromTtf(szFontPath, lSize, szCharset, 128);
		eInType = tFontFormat::TTF;
	}
	else if(nFs::isDir(szFontPath)) {
		mGlyphs = tGlyphSet::fromDir(szFontPath);
		if(!mGlyphs.isOk()) {
			nLog::error("Loading glyphs from dir '{}' failed", szFontPath);
			return EXIT_FAILURE;
		}
		eInType = tFontFormat::DIR;
	}
	else if(szFontPath.find(".png") != std::string::npos) {
		// TODO param for determining whether png is pmng-format or ace format
		mGlyphs = tGlyphSet::fromPmng(szFontPath, ubFirstChar);
		eInType = tFontFormat::PMNG;
	}
	else if(szFontPath.find(".fnt") != std::string::npos) {
		mGlyphs = tGlyphSet::fromAceFont(szFontPath);
		eInType = tFontFormat::FNT;
	}
	else {
		nLog::error("Unsupported font source: '{}'", pArgs[1]);
		return EXIT_FAILURE;
	}
	if(eInType == tFontFormat::INVALID || !mGlyphs.isOk()) {
		nLog::error("Couldn't read any font glyphs");
		return EXIT_FAILURE;
	}

	// Remap chars accordingly
	if(!szRemapPath.empty()) {
		auto *pJson = jsonCreate(szRemapPath.c_str());
		if(pJson == nullptr) {
			nLog::error("Couldn't open remap file: '{}'", szRemapPath);
			return EXIT_FAILURE;
		}
		auto TokRemapArray = jsonGetDom(pJson, "remap");
		auto ElementCount = pJson->pTokens[TokRemapArray].size;
		std::vector<std::pair<uint32_t, uint32_t>> vFromTo;
		for(auto i = ElementCount; i--;) {
			auto TokRemapEntry = jsonGetElementInArray(pJson, TokRemapArray, i);
			auto TokFrom = jsonGetElementInArray(pJson, TokRemapEntry, 0);
			auto TokTo = jsonGetElementInArray(pJson, TokRemapEntry, 1);
			auto From = getCharCodeFromTok(pJson, TokFrom);
			auto To = getCharCodeFromTok(pJson, TokTo);
			fmt::print("Remapping {} => {}\n", From, To);
			vFromTo.push_back({std::move(From), std::move(To)});
		}
		mGlyphs.remapGlyphs(vFromTo);
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
		return EXIT_FAILURE;
	}

	if(eOutType == tFontFormat::DIR) {
		if(szOutPath == szFontPath) {
			szOutPath += ".dir";
		}
		mGlyphs.toDir(szOutPath);
	}
	else {
		if(eOutType == tFontFormat::PNG) {
			tChunkyBitmap FontChunky = mGlyphs.toPackedBitmap(true);
			if(szOutPath.substr(szOutPath.length() - 4) != ".png") {
				szOutPath += ".png";
			}
			FontChunky.toPng(szOutPath);
		}
		else if(eOutType == tFontFormat::FNT) {
			if(szOutPath.substr(szOutPath.length() - 4) != ".fnt") {
				szOutPath += ".fnt";
			}
			mGlyphs.toAceFont(szOutPath);
		}
		else {
			nLog::error("Unsupported output type");
			return EXIT_FAILURE;
		}
	}
	fmt::print("All done!\n");
	return EXIT_SUCCESS;
}
