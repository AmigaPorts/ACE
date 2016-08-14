#include <ace/utils/palette.h>

void paletteLoad(char *szFileName, UWORD *pPalette, UBYTE ubMaxLength) {
	FILE *pFile;
	UBYTE ubPaletteLength;
	
	logBlockBegin("paletteLoad(szFileName: %s, pPalette: %p, ubMaxLength: %hu)", szFileName, pPalette, ubMaxLength);
		
	pFile = fopen(szFileName, "r");
	fread(&ubPaletteLength, 1, 1, pFile);
	logWrite(" Color count: %u\n", ubPaletteLength);
	if(ubPaletteLength > ubMaxLength)
		ubPaletteLength = ubMaxLength;
	fread(pPalette, sizeof(UWORD), ubPaletteLength, pFile);
	fclose(pFile);
	
	logBlockEnd("paletteLoad()");
}