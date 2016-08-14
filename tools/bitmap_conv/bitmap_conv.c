#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lodepng.h"

void writeByte(uint8_t b, FILE *f) {
	fwrite(&b, 1, 1, f);
}

typedef struct _tColor {
	uint8_t ubR;
	uint8_t ubG;
	uint8_t ubB;
} tColor;

void printSupportedExtensions(void) {
	printf("Supported types:\n");
	printf("\tPNG (.png)");	
}

int16_t findColor(uint8_t *pRGB, tColor *pPalette, uint8_t ubColorCount) {
	while(ubColorCount--)
		if(pPalette[ubColorCount].ubR == pRGB[0] && pPalette[ubColorCount].ubG == pRGB[1] && pPalette[ubColorCount].ubB == pRGB[2])
			return ubColorCount;
	return -1;
}

void writePlanar(char *szOutPath, uint8_t *pImgData, uint16_t uwWidth, uint16_t uwHeight, tColor *pPalette, uint8_t ubColorCount) {
	uint8_t i, ubPlaneCount, ubPlane;
	uint16_t x, y, uwPixelBuffer;
	int16_t wColorIdx;
	FILE *pOut;
	
	if(uwWidth & 0xF) {
		printf("Width is not divisible by 16!\n");
		return;
	}

	pOut	= fopen(szOutPath, "wb");
	if(!pOut) {
		printf("Can't write to file %s\n", szOutPath);
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
	printf("Width: %u, height: %u\n", uwWidth, uwHeight);
	for(i = 0; i != 4; ++i)
		writeByte(0, pOut);
	// Write bitplanes - from LSB to MSB
	for(ubPlane = 0; ubPlane != ubPlaneCount; ++ubPlane) {
		for(y = 0; y != uwHeight; ++y) {
			uwPixelBuffer = 0;
			for(x = 0; x != uwWidth; ++x) {
				// Determine color
				wColorIdx = findColor(&pImgData[y*uwWidth*3 + x*3], pPalette, ubColorCount);
				if(wColorIdx == -1) {
					printf("Color not found in palette: %hhu, %hhu, %hhu @%u,%u\n", pImgData[y*uwWidth*3 + x*3], pImgData[y*uwWidth*3 + x*3 + 1], pImgData[y*uwWidth*3 + x*3 + 2], x, y);
					return;
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

uint8_t paletteLoad(char *szPath, tColor *pPalette) {
	FILE *pIn;
	uint8_t ubPaletteCount, i, ubXR, ubGB;
	
	pIn = fopen(szPath, "rb");
	if(!pIn) {
		printf("Can't open file %s\n", szPath);
		return 0;
	}
	fread(&ubPaletteCount, 1, 1, pIn);
	printf("Palette color count: %hhu\n", ubPaletteCount);
	for(i = 0; i != ubPaletteCount; ++i) {
		fread(&ubXR, 1, 1, pIn);
		fread(&ubGB, 1, 1, pIn);
		pPalette[i].ubR = (ubXR & 0x0F) << 4;
		pPalette[i].ubG = (ubGB & 0xF0);
		pPalette[i].ubB = (ubGB & 0x0F) << 4;
	}
	fclose(pIn);
	return ubPaletteCount;
}

int main(int argc, char *argv[]) {
	char *szExt, *szOutPath;
	uint8_t *pImgIn;            /// Format is: 0xRR 0xGG 0xBB 0xRR 0xGG 0xBB...
	uint8_t ubPaletteCount;
	uint16_t uwWidth, uwHeight; /// Image width & height
	tColor pPalette[256];
	
	if(argc < 3) {
		printf("Usage: %s palette_path img_input_path [img_output_path]\n", argv[0]);
		printSupportedExtensions();
		return 0;
	}
	
	// Load palette
	printf("Loading palette from %s...\n", argv[1]);
	ubPaletteCount = paletteLoad(argv[1], pPalette);
	if(!ubPaletteCount) {
		printf("No palette colors read, aborting.\n");
		return 0;
	}
	
	// Load image
	printf("Loading image from %s...\n", argv[1]);
	szExt = strrchr(argv[2], '.');
	if(!strcmp(szExt, ".png")) {
		unsigned uError, uWidth, uHeight;
		uError = lodepng_decode24_file(&pImgIn, &uWidth, &uHeight, argv[2]);
		if(uError) {
			printf("Erorr loading %s\n", argv[2]);
			free(pImgIn);
			return;
		}
		uwWidth = uWidth;
		uwHeight = uHeight;
	}
	// TODO: other extensions
	else {
		printf("Unknown extension type: %s\n", szExt);
		printSupportedExtensions();
		return 0;
	}
	
	if(argc >= 4)
		szOutPath = argv[3];
	else {
		szOutPath = malloc(szExt - argv[2] + 3 + 1); // path + ".bm" + '\0'
		memcpy(szOutPath, argv[2], szExt - argv[2]);
		szOutPath[szExt - argv[2]] = '\0';
		strcat(szOutPath, ".bm");
	}
	
	// Write as planar
	printf("Writing to %s...\n", szOutPath);
	writePlanar(szOutPath, pImgIn, uwWidth, uwHeight, pPalette, ubPaletteCount);
	
	if(argc >= 4) {
		free(szOutPath);
	}
	free(pImgIn);
	
	return 0;
}
