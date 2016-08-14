#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct _tColor {
	uint8_t ubR;
	uint8_t ubG;
	uint8_t ubB;
} tColor;

void printSupportedExtensions(void) {
	printf("Supported palettes:\n");
	printf("\tGIMP Palette (.gpl)\n");
}

void trimEnd(char *str) {
	char *c;
	
	for(c = &(str[strlen(str)-1]); strchr("\n\r ", *c) && c >= str; --c)
		*c = '\0';
}

uint8_t paletteLoadFromGpl(FILE *pFile, tColor *pPalette) {
	char szBuffer[255];
	uint8_t ubCommentPos;
	char *pComment;
	tColor *pColor;
	
	fgets(szBuffer, 255, pFile);
	trimEnd(szBuffer);
	if(strcmp(szBuffer, "GIMP Palette")) {
		printf("Invalid .gpl file - palette mismatch\n");
		return 0;
	}
	pColor = pPalette;
	while(!feof(pFile)) {
		szBuffer[0] = 0;
		fgets(szBuffer, 255, pFile);
		trimEnd(szBuffer);
		pComment = strchr(szBuffer, '#');
		if(pComment)
			*pComment = '\0';
		if(!strlen(szBuffer))
			continue;
		if(szBuffer == strstr(szBuffer, "Name:") || szBuffer == strstr(szBuffer, "Columns:")) {
			// ignore
			continue;
		}
		if(sscanf(szBuffer, "%u %u %u", &pColor->ubR, &pColor->ubG, &pColor->ubB) == 3) {
			++pColor;
		}
	}
	return pColor - pPalette;
}

int main(int argc, char *argv[]) {
	char *szExt, *szOut;
	tColor pPalette[256];
	uint8_t ubColorCount, i, ubXR, ubGB;
	FILE *pFile;
	
	// No args?
	if(argc == 1) {
		printf("Usage: %s source_path [dest_path.pal]\n", argv[0]);
		printSupportedExtensions();
		return 0;
	}
	
	// Determine file extension
	szExt = strrchr(argv[1], '.');
	if(!szExt) {
		printf("No input file extension, aborting...\n");
		return 0;
	}

	// Read input file
	printf("Reading from %s...\n", argv[1]);
	pFile = fopen(argv[1], "r");
	if(!strcmp(szExt, ".gpl"))
		ubColorCount = paletteLoadFromGpl(pFile, pPalette);
	// TODO: other extensions
	else {
		fclose(pFile);
		printf("Unknown input file extension: %s\n", szExt);
		printSupportedExtensions();
		return 0;
	}
	fclose(pFile);
	
	if(!ubColorCount)
		printf("ERROR: read 0 colors\n");
	else
		printf("Read %hu colors\n", ubColorCount);
	
	// Determine output path
	if(argc == 3)
		szOut = argv[2];
	else {
		szOut = malloc((unsigned)(szExt - argv[1]) + 4+1); // filename + ".plt" + \0
		memcpy(szOut, argv[1], szExt - argv[1]);
		szOut[szExt - argv[1]] = '\0';
		strcat(szOut, ".plt");
	}
	
	// Write ACE palette
	printf("Writing to %s...\n", szOut);
	pFile = fopen(szOut, "wb");
	fwrite(&ubColorCount, 1, 1, pFile);
	for(i = 0; i != ubColorCount; ++i) {
		ubXR = (pPalette[i].ubR >> 4);
		ubGB = (pPalette[i].ubG & 0xF0) | (pPalette[i].ubB >> 4);
		fwrite(&ubXR, 1, 1, pFile);
		fwrite(&ubGB, 1, 1, pFile);
	}
	fflush(pFile);
	fclose(pFile);
	if(argc != 3) {
		free(szOut);
	}
	printf("Done.\n");
	return 0;
}