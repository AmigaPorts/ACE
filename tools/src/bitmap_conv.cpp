/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

 #include <optional>
#include "common/logging.h"
#include "common/fs.h"
#include "common/rgb.h"
#include "common/bitmap.h"

void printUsage(const std::string &szAppName)
{
	using fmt::print;
	print("Usage:\n\t{} palPath inPath [extraOpts]\n\n", szAppName);
	print("palPath\t - path to supported palette file\n");
	print("inPath\t - path to supported input bitmap file\n");
	print("extraOpts:\n");
	print("\t-o outPath\tSpecify output file path. If ommited, it will perform default conversion\n");
	print("\t-i\t\tEnable interleaved mode\n");
	print("\t-ehb\t\tExtend palette with EHB colors\n");
	print("\t-mc #RRGGBB\tTreat color #RRGGBB as mask\n");
	print("\t-mf outMaskPath\tSpecify path for mask.bm file. If omitted, it will try\n");
	print("\t\t\tto use same path as .bm with \"_mask.bm\" suffix\n");
	print("\t-nmo\t\tDon't generate mask output file\n");
	print("\t-no\t\tDon't generate bitplane output file\n");
	print("\t-mp\t\tAdd a mask as extra plane. Valid only with -mc and without -i\n");
	print("Default conversions:\n");
	print("\t.bm -> .png (will try to read mask from inPath_mask.bm)\n");
	print("\t.png -> .bm (will write mask to outPath_mask.bm if -mc was specified)\n");
}

int main(int lArgCount, const char *pArgs[])
{
	using namespace std::string_view_literals;

	const std::uint8_t ubMandatoryArgCnt = 2;
	if(lArgCount - 1 < ubMandatoryArgCnt) {
		nLog::error("Too few arguments, expected {}", ubMandatoryArgCnt);
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	std::string szPalette = pArgs[1], szInput = pArgs[2];
	std::string szOutput = "", szMask = "";
	bool isWriteInterleaved = false;
	bool isEhb = false;
	bool isEnabledOutputMask = true;
	bool isEnabledOutput = true;
	bool isAppendMaskPlane = false;
	std::optional<tRgb> MaskColor;

	for(auto ArgIndex = ubMandatoryArgCnt + 1; ArgIndex < lArgCount; ++ArgIndex) {
		if(pArgs[ArgIndex] == "-o"sv) {
			auto &Value = pArgs[++ArgIndex];
			szOutput = Value;
		}
		else if(pArgs[ArgIndex] == "-i"sv) {
			isWriteInterleaved = true;
		}
		else if(pArgs[ArgIndex] == "-ehb"sv) {
			isEhb = true;
		}
		else if(pArgs[ArgIndex] == "-mc"sv && ArgIndex < lArgCount - 1) {
			auto &Value = pArgs[++ArgIndex];
			MaskColor = tRgb(Value);
		}
		else if(pArgs[ArgIndex] == "-mf"sv && ArgIndex < lArgCount - 1) {
			auto &Value = pArgs[++ArgIndex];
			szMask = Value;
		}
		else if(pArgs[ArgIndex] == "-nmo"sv) {
			isEnabledOutputMask = false;
		}
		else if(pArgs[ArgIndex] == "-no"sv) {
			isEnabledOutput = false;
		}
		else if(pArgs[ArgIndex] == "-mp"sv) {
			isAppendMaskPlane = true;
		}
		else {
			nLog::error("Unknown arg or missing value: '{}'", pArgs[ArgIndex]);
			printUsage(pArgs[0]);
			return EXIT_FAILURE;
		}
	}

	if(isAppendMaskPlane && isWriteInterleaved) {
		nLog::error("Can't write extra mask plane in interleaved bitmaps");
	}
	if(isAppendMaskPlane && !MaskColor.has_value()) {
		nLog::error("Can't write extra mask plane without mask color specified");
	}

	std::string szInExt = nFs::getExt(szInput);
	if(szInExt != "png" && szInExt != "bm") {
		nLog::error("Input file type not supported: {}", szInExt);
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}
	if(szOutput.empty()) {
		szOutput = nFs::removeExt(szInput);
		if(szInExt == "png") {
			szOutput += ".bm";
		}
		else if(szInExt == "bm") {
			szOutput += ".png";
		}
	}
	std::string szOutExt = nFs::getExt(szOutput);

	if(szMask == "" && MaskColor.has_value()) {
		if(szOutExt == "bm") {
			szMask = nFs::removeExt(szOutput) + "_mask." + szOutExt;
		}
		else if(szInExt == "bm") {
			szMask = nFs::removeExt(szInput) + "_mask." + szInExt;
		}
	}

	// Load palette
	auto Palette = tPalette::fromFile(szPalette);
	if(!Palette.isValid()) {
		nLog::error("Couldn't read palette '{}'", szPalette);
		return EXIT_FAILURE;
	}
	if(isEhb) {
		if(!Palette.convertToEhb()) {
			nLog::error("Couldn't convert palette to EHB! Does it have at most 32 colors?");
		}
	}


	// Load input
	tChunkyBitmap In;
	if(szInExt == "bm") {
		auto InPlanar = tPlanarBitmap::fromBm(szInput);
		if(!InPlanar.m_uwWidth) {
			nLog::error("Couldn't load input: '{}'", szInput);
		}
		In = tChunkyBitmap(InPlanar, Palette);
		if(MaskColor.has_value()) {
			tPalette PaletteMask;
			PaletteMask.m_vColors.push_back(tRgb(0,0,0));
			for(std::uint16_t i = 1; i < 256; ++i) {
				PaletteMask.m_vColors.push_back(MaskColor.value());
			}
			auto szInMask = nFs::removeExt(szInput) + "_mask." + szInExt;
			auto InMask = tChunkyBitmap(tPlanarBitmap::fromBm(szInMask), PaletteMask);
			if(!In.mergeWithMask(InMask)) {
				nLog::error("Mask incompatible with bitmap");
				return EXIT_FAILURE;
			}
		}
	}
	else if(szInExt == "png") {
		In = tChunkyBitmap::fromPng(szInput);
	}
	else {
		nLog::error("Input file type not supported: {}", szInExt);
		return EXIT_FAILURE;
	}

	// Save to output
	if(szOutExt == "bm") {
		std::optional<tPlanarBitmap> MaskPlanar;
		if(MaskColor.has_value()) {
			MaskPlanar = tPlanarBitmap::createMaskFrom(
				In, MaskColor.value(), isWriteInterleaved ? Palette.getBpp() : 1
			);
			if(isEnabledOutputMask && !isAppendMaskPlane) {
				MaskPlanar.value().toBm(szMask, isWriteInterleaved);
			}
		}
		auto PaletteIgnore = MaskColor.has_value()
			? tPalette({MaskColor.value()})
			: tPalette();
		auto Planar = tPlanarBitmap(In, Palette, PaletteIgnore);
		if(!Planar.m_uwWidth) {
			return EXIT_FAILURE;
		}
		if(isAppendMaskPlane && MaskPlanar.has_value()) {
			if(!Planar.tryAppendPlaneFrom(MaskPlanar.value(), 0)) {
				return EXIT_FAILURE;
			}
		}
		if(isEnabledOutput) {
			Planar.toBm(szOutput, isWriteInterleaved);
		}
	}
	else if(szOutExt == "png") {
		In.toPng(szOutput);
	}

	return EXIT_SUCCESS;
}
