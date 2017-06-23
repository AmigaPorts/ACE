/**
 *  ACE bitmap converter tool.
 *  Allows converting from widely used image file formats to ACE bitmap (.bm).
 *  Also allows writing transparency masks made from specified color.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lodepng.h"

#define MODE_INTERLEAVED 1

char *g_pKnownBitmapExts[] = {".png", ""};
char *g_pKnownPaletteExts[] = {".plt", ""};
char *g_szInputPath;
char *g_szPalettePath;
char g_szOutputPath[1024];
char g_szMaskOutputPath[1024];
int8_t g_bBitmapExt;
int8_t g_bPaletteExt;
uint16_t g_uwMaskR, g_uwMaskG, g_uwMaskB;
uint8_t ubMode;

typedef struct _tColor {
	uint8_t ubR;
	uint8_t ubG;
	uint8_t ubB;
} tColor;

void writeByte(uint8_t b, FILE *f) {
	fwrite(&b, 1, 1, f);
}

void printUsage(char *szPrgName) {
	// Main usage
	printf("\nUsage: %s palette_path img_input_path [opts]\n", szPrgName);

	// Image types
	puts("\nSupported input image types:");
	puts("\tPNG (.png)");

	// Parameters
	puts("\nOptional parameters:");
	puts("\t-o path\t\tSpecify output file path");
	puts("\t-i\t\tInterleaved mode");
	puts("\t-mc #RRGGBB\tAlpha channel mask color");
	puts("\t-mo path\tAlpha channel output path");
	puts("TODO\t-nmo\t\tDon't generate mask output file");
	puts("TODO\t-no\t\tDon't generate bitplane output file");
}

int16_t findColor(uint8_t *pRGB, tColor *pPalette, uint8_t ubColorCount) {
	while(ubColorCount--)
		if(pPalette[ubColorCount].ubR == pRGB[0] && pPalette[ubColorCount].ubG == pRGB[1] && pPalette[ubColorCount].ubB == pRGB[2])
			return ubColorCount;
	return -1;
}

void writePlanarInterleaved(
	uint8_t *pImgData,
	uint16_t uwWidth, uint16_t uwHeight,
	tColor *pPalette, uint8_t ubColorCount
) {
	uint8_t i, ubPlaneCount, ubPlane;
	uint16_t x, y, uwPixelBuffer;
	uint32_t ulPos;
	int16_t wColorIdx;
	FILE *pOut;

	if(uwWidth & 0xF) {
		printf("Width is not divisible by 16!\n");
		return;
	}

	pOut	= fopen(g_szOutputPath, "wb");
	if(!pOut) {
		printf("Can't write to file %s\n", g_szOutputPath);
		return;
	}

	ubPlaneCount = 1;
	for(i = 2; i < ubColorCount; i <<= 1)
		++ubPlaneCount;

	// Write .bm header
	writeByte(uwWidth >> 8, pOut);
	writeByte(uwWidth & 0xFF, pOut);
	writeByte(uwHeight >> 8, pOut);
	writeByte(uwHeight & 0xFF, pOut);
	writeByte(ubPlaneCount, pOut);
	writeByte(0, pOut); // Version
	writeByte(1, pOut); // Flags
	for(i = 0; i != 2; ++i)
		writeByte(0, pOut);

	// Write bitplanes - from LSB to MSB
	for(y = 0; y != uwHeight; ++y) {
		for(ubPlane = 0; ubPlane != ubPlaneCount; ++ubPlane) {
			uwPixelBuffer = 0;
			for(x = 0; x != uwWidth; ++x) {
				// Determine color
				ulPos = (y*uwWidth + x) * 3;
				wColorIdx = findColor(&pImgData[ulPos], pPalette, ubColorCount);
				if(wColorIdx == -1) {
					if(pImgData[ulPos] != g_uwMaskR || pImgData[ulPos+1] != g_uwMaskG || pImgData[ulPos+2] != g_uwMaskB) {
						printf(
							"ERR: Unexpected color: %hhu, %hhu, %hhu @%u,%u\n",
							pImgData[ulPos], pImgData[ulPos+1],	pImgData[ulPos+2], x, y
						);
						return;
					}
					else
						wColorIdx = 0;
				}

				// Write to bitplane
				uwPixelBuffer <<= 1;
				if(wColorIdx & (1 << ubPlane))
					uwPixelBuffer |= 1;
				if((x & 0xF) == 0xF) {
					writeByte(uwPixelBuffer >> 8, pOut);
					writeByte(uwPixelBuffer & 0xFF, pOut);
				}
			}
		}
	}

	fclose(pOut);
}

void writePlanar(
	uint8_t *pImgData,
	uint16_t uwWidth, uint16_t uwHeight,
	tColor *pPalette, uint8_t ubColorCount
) {
	uint8_t i, ubPlaneCount, ubPlane;
	uint16_t x, y, uwPixelBuffer;
	uint32_t ulPos;
	int16_t wColorIdx;
	FILE *pOut;

	if(uwWidth & 0xF) {
		printf("Width is not divisible by 16!\n");
		return;
	}

	pOut	= fopen(g_szOutputPath, "wb");
	if(!pOut) {
		printf("Can't write to file %s\n", g_szOutputPath);
		return;
	}

	ubPlaneCount = 1;
	for(i = 2; i < ubColorCount; i <<= 1)
		++ubPlaneCount;

	// Write .bm header
	writeByte(uwWidth >> 8, pOut);
	writeByte(uwWidth & 0xFF, pOut);
	writeByte(uwHeight >> 8, pOut);
	writeByte(uwHeight & 0xFF, pOut);
	writeByte(ubPlaneCount, pOut);
	writeByte(0, pOut); // Version
	writeByte(0, pOut); // Flags
	for(i = 0; i != 2; ++i)
		writeByte(0, pOut);

	// Write bitplanes - from LSB to MSB
	for(ubPlane = 0; ubPlane != ubPlaneCount; ++ubPlane) {
		for(y = 0; y != uwHeight; ++y) {
			uwPixelBuffer = 0;
			for(x = 0; x != uwWidth; ++x) {
				// Determine color
				ulPos = (y*uwWidth + x) * 3;
				wColorIdx = findColor(&pImgData[ulPos], pPalette, ubColorCount);
				if(wColorIdx == -1) {
					if(pImgData[ulPos] != g_uwMaskR || pImgData[ulPos+1] != g_uwMaskG || pImgData[ulPos+2] != g_uwMaskB) {
						printf(
							"ERR: Unexpected color: %hhu, %hhu, %hhu @%u,%u\n",
							pImgData[ulPos], pImgData[ulPos+1],	pImgData[ulPos+2], x, y
						);
						return;
					}
					else
						wColorIdx = 0;
				}

				// Write to bitplane
				uwPixelBuffer <<= 1;
				if(wColorIdx & (1 << ubPlane))
					uwPixelBuffer |= 1;
				if((x & 0xF) == 0xF) {
					writeByte(uwPixelBuffer >> 8, pOut);
					writeByte(uwPixelBuffer & 0xFF, pOut);
				}
			}
		}
	}

	fclose(pOut);
}

void writeMask(
	uint8_t *pImgData,
	uint16_t uwWidth, uint16_t uwHeight
) {
	uint16_t x,y, uwPixelBuffer;
	uint32_t ulPos;
	FILE *pOut;
	pOut = fopen(g_szMaskOutputPath, "wb");
	if(!pOut) {
		printf("Can't write to file %s\n", g_szMaskOutputPath);
		return;
	}
	// Write mask header
	writeByte(uwWidth >> 8, pOut);
	writeByte(uwWidth & 0xFF, pOut);
	writeByte(uwHeight >> 8, pOut);
	writeByte(uwHeight & 0xFF, pOut);

	// Write mask data
	for(y = 0; y != uwHeight; ++y) {
		uwPixelBuffer = 0;
		for(x = 0; x != uwWidth; ++x) {
			ulPos = y*uwWidth*3 + x*3;
			
			uwPixelBuffer <<= 1;
			if(pImgData[ulPos] != g_uwMaskR || pImgData[ulPos+1] != g_uwMaskG || pImgData[ulPos+2] != g_uwMaskB)
				uwPixelBuffer |= 1;
			if((x & 0xF) == 0xF) {
				writeByte(uwPixelBuffer >> 8, pOut);
				writeByte(uwPixelBuffer & 0xFF, pOut);
			}
		}
	}
	
	fclose(pOut);
}

void writeMaskInterleaved(
	uint8_t *pImgData,
	uint16_t uwWidth, uint16_t uwHeight, uint16_t uwPaletteCount
) {
	uint16_t x,y, uwPixelBuffer;
	uint32_t ulPos;
	uint8_t ubPlane, ubBpp, i;
	FILE *pOut;
	
	ubBpp = 1;
	for(i = 2; i < uwPaletteCount; i <<= 1)
		++ubBpp;
	
	pOut = fopen(g_szMaskOutputPath, "wb");
	if(!pOut) {
		printf("Can't write to file %s\n", g_szMaskOutputPath);
		return;
	}
	// Write mask header
	writeByte(uwWidth >> 8, pOut);
	writeByte(uwWidth & 0xFF, pOut);
	writeByte(uwHeight >> 8, pOut);
	writeByte(uwHeight & 0xFF, pOut);

	// Write mask data
	for(y = 0; y != uwHeight; ++y) {
		for(ubPlane = 0; ubPlane != ubBpp; ++ubPlane) {
			uwPixelBuffer = 0;
			for(x = 0; x != uwWidth; ++x) {
				ulPos = y*uwWidth*3 + x*3;
				
				uwPixelBuffer <<= 1;
				if(pImgData[ulPos] != g_uwMaskR || pImgData[ulPos+1] != g_uwMaskG || pImgData[ulPos+2] != g_uwMaskB)
					uwPixelBuffer |= 1;
				if((x & 0xF) == 0xF) {
					writeByte(uwPixelBuffer >> 8, pOut);
					writeByte(uwPixelBuffer & 0xFF, pOut);
				}
			}
		}
	}
	
	fclose(pOut);
}

uint8_t paletteLoad(char *szPath, tColor *pPalette) {
	FILE *pIn;
	uint8_t uwPaletteCount, i, ubXR, ubGB;

	pIn = fopen(szPath, "rb");
	if(!pIn) {
		printf("Can't open file %s\n", szPath);
		return 0;
	}
	fread(&uwPaletteCount, 1, 1, pIn);
	printf("Palette color count: %hhu\n", uwPaletteCount);
	for(i = 0; i != uwPaletteCount; ++i) {
		fread(&ubXR, 1, 1, pIn);
		fread(&ubGB, 1, 1, pIn);
		pPalette[i].ubR = ((ubXR & 0x0F) << 4) | (ubXR & 0x0F);
		pPalette[i].ubG = ((ubGB & 0xF0) >> 4) | (ubGB & 0xF0);
		pPalette[i].ubB = ((ubGB & 0x0F) << 4) | (ubGB & 0x0F);
	}
	fclose(pIn);
	return uwPaletteCount;
}

int inExtArray(char **pExts, char *szValue) {
	int i;

	i = 0;
	while(*pExts[i]) {
		if(!strcmp(pExts[i], szValue))
			return i;
		++i;
	}
	return -1;
}

int determineArgs(int argc, char *argv[]) {
	char *szExt;
	uint8_t i;
	uint16_t uwPathLen;

	// Default values
	g_szOutputPath[0] = 0;
	g_szMaskOutputPath[0] = 0;
	g_uwMaskR = 0xFFFF;
	ubMode = 0;
	
	if(argc < 3) {
		printf("ERR: too few arguments\n");
		return 0;
	}
	
	// First one must be a path to known palette type
	szExt = strrchr(argv[1], '.');
	if(!szExt) {
		printf("ERR: Input palette has no extension\n");
		return 0;
	}
	g_bPaletteExt = inExtArray(g_pKnownPaletteExts, szExt);
	if(g_bPaletteExt == -1) {
		printf("ERR: Unknown palette extension %s\n", szExt);
		return 0;
	}
	g_szPalettePath = argv[1];

	// Second one must be a path to known image type
	szExt = strrchr(argv[2], '.');
	if(!szExt) {
		printf("ERR: Input bitmap has no extension\n");
		return 0;
	}
	g_bBitmapExt = inExtArray(g_pKnownBitmapExts, szExt);
	if(g_bBitmapExt == -1) {
		printf("ERR: Unknown bitmap extension %s\n", szExt);
		return 0;
	}
	g_szInputPath = argv[2];

	// Then there goes switches, must start with "-"
	// After them there must be at least 1 arg as switch value - not for all (e.g. interleaved)
	for(i = 3; i < argc; ++i) {
		// Check if switch starts with "-"
		if(argv[i][0] != '-') {
			printf("ERR: Did you mean: -%s ?\n", argv[i]);
			return 0;
		}

		if(!strcmp(argv[i], "-i")) {
			// Interleaved mode
			ubMode |= MODE_INTERLEAVED;
		}
		else if(!strcmp(argv[i], "-o")) {
			// Output file
			if(argv[i+1][0] == '-') {
				printf("ERR: No file path after -o switch\n");
				return 0;
			}
			strcpy(g_szOutputPath, argv[i+1]);
			++i;
		}
		else if(!strcmp(argv[i], "-mc")) {
			// Mask color
			if(argv[i+1][0] == '-') {
				printf("ERR: No color code after -mc switch\n");
				return 0;
			}
			if(argv[i+1][0] != '#') {
				printf("ERR: Unknown color spec: %s\n", argv[i+1]);
				return 0;
			}
			unsigned int r, g, b;
			sscanf(argv[i+1], "#%2x%2x%2x", &r, &g, &b);
			g_uwMaskR = r;
			g_uwMaskG = g;
			g_uwMaskB = b;
			++i;
		}
		else if(!strcmp(argv[i], "-mo")) {
			// Mask output file
			if(argv[i+1][0] == '-') {
				printf("ERR: No file path after -mo switch\n");
				return 0;
			}
			strcpy(g_szMaskOutputPath, argv[i+1]);
			++i;
		}
	}

	// Default parameter values
	// Bitmap output path
	uwPathLen = strrchr(g_szInputPath, '.') - g_szInputPath;
	if(!strlen(g_szOutputPath)) {
		memcpy(g_szOutputPath, g_szInputPath, uwPathLen);
		g_szOutputPath[uwPathLen] = '\0';
		strcat(g_szOutputPath, ".bm");
	}

	// Bitmap mask output path
	if(!strlen(g_szMaskOutputPath) && g_uwMaskR != 0xFFFF) {
		memcpy(g_szMaskOutputPath, g_szInputPath, uwPathLen);
		g_szMaskOutputPath[uwPathLen] = '\0';
		strcat(g_szMaskOutputPath, ".msk");
	}
	
	return 1;
}

int main(int argc, char *argv[]) {
	char *szExt;
	uint8_t *pImgIn;            ///< Format is: 0xRR 0xGG 0xBB 0xRR 0xGG 0xBB...
	uint16_t uwPaletteCount;    ///< Number of colors in palette
	uint16_t uwWidth, uwHeight; ///< Image width & height
	tColor pPalette[256];

	if(!determineArgs(argc, argv)) {
		printUsage(argv[0]);
		return 0;
	}

	// Load palette
	printf("Loading palette from %s...\n", g_szPalettePath);
	uwPaletteCount = paletteLoad(g_szPalettePath, pPalette);
	if(!uwPaletteCount) {
		printf("No palette colors read, aborting.\n");
		return 0;
	}

	// Load image
	printf("Loading image from %s...\n", g_szInputPath);
	szExt = strrchr(g_szInputPath, '.');
	if(!strcmp(szExt, ".png")) {
		unsigned uError, uWidth, uHeight;
		uError = lodepng_decode24_file(&pImgIn, &uWidth, &uHeight, g_szInputPath);
		if(uError) {
			printf("ERR: loading %s\n", g_szInputPath);
			free(pImgIn);
			return 0;
		}
		uwWidth = uWidth;
		uwHeight = uHeight;
	}
	// TODO: other extensions
	else {
		printf("Unknown input bitmap extension type: %s\n", szExt);
		printUsage(argv[0]);
		return 0;
	}

	// Write as planar
	if(ubMode & MODE_INTERLEAVED) {
		printf("Writing interleaved bitmap to %s...\n", g_szOutputPath);
		writePlanarInterleaved(pImgIn, uwWidth, uwHeight, pPalette, uwPaletteCount);
	}
	else {
		printf("Writing bitmap to %s...\n", g_szOutputPath);
		writePlanar(pImgIn, uwWidth, uwHeight, pPalette, uwPaletteCount);
	}

	if(*g_szMaskOutputPath) {
		printf("Mask color: #%02x%02x%02x\n", g_uwMaskR, g_uwMaskG, g_uwMaskB);
		if(ubMode & MODE_INTERLEAVED) {
			printf("Writing interleaved bitmap mask to %s...\n", g_szMaskOutputPath);
			writeMaskInterleaved(pImgIn, uwWidth, uwHeight, uwPaletteCount);
		}
		else {
			printf("Writing bitmap mask to %s...\n", g_szMaskOutputPath);
			writeMask(pImgIn, uwWidth, uwHeight);			
		}
	}

	printf("All done!");
	free(pImgIn);

	return 0;
}

/**
 * This implementation is painfully slow, but it eats as little memory as possible
 */
 /*
void writePlanes(FILE *pOut, uint8_t *pData, uint16_t uwWidth, uint16_t uwHeight, uint16_t *pPalette, uint8_t ubBpp) {
	uint8_t ubPlane, x, y, ubCheckBit;
	uint16_t uwOutBfr;

	// Write bitplanes
	for(ubPlane = 0; ubPlane != ubBpp; ++ubPlane) {
		ubCheckBit = 1 << ubPlane;
		// uwOutBfr = 0; // Needed?
		for(y = 0; y != uwHeight; ++y) {
			for(x = 0; x != uwWidth; ++x) {
				uwOutBfr <<= 1;
				ubColor = getColorIdx(pPalette, ubBpp, pData[y*uwWidth + x]);
				if(ubColor & ubCheckBit)
					uwOutBfr |= 1;
				if(x & 0xF == 0xF) {
					// Write filled word buffer - watch out for endians!
					uint8_t ubByteBfr;
					ubByteBfr = uwOutBfr>>8;
					fwrite(&ubByteBfr, 1, 1, pOut);
					ubByteBfr = uwOutBfr&0xFF;
					fwrite(&ubByteBfr, 1, 1, pOut);
					// uwOutBfr = 0; // Needed?
				}
			}
		}
	}
}
*/


/**
 * This implementation is painfully slow, but it eats as little memory as possible
 */
 /*
void sth(FILE *pOut, uint8_t *pData, uint16_t uwWidth, uint16_t uwHeight, uint16_t *pPalette, uint8_t ubBpp, uint8_t ubMaskColor) {
	uint8_t ubPlane, x, y, ubCheckBit;
	uint16_t uwOutBfr;

	writePlanes(pOut, pData, uwWidth, uwHeight, pPalette, ubBpp);

	// Write mask
	for(y = 0; y != uwHeight; ++y) {
		for(x = 0; x != uwWidth; ++x) {
			uwOutBfr <<= 1;
			ubColor = getColorIdx(pPalette, ubBpp, pData[y*uwWidth + x]);
			if(ubColor == ubMaskColor)
				uwOutBfr |= 1;
			if(x & 0xF == 0xF) {
				// Write filled word buffer - watch out for endians!
				uint8_t ubByteBfr;
				ubByteBfr = uwOutBfr>>8;
				fwrite(&ubByteBfr, 1, 1, pOut);
				ubByteBfr = uwOutBfr&0xFF;
				fwrite(&ubByteBfr, 1, 1, pOut);
				// uwOutBfr = 0; // Needed?
			}
		}
	}
}
*/