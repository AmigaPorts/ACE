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

void paletteDim(UWORD *pSource, UWORD *pDest, UBYTE ubColorCount, UBYTE ubLevel) {
	UBYTE i, c;
	UBYTE r,g,b;
	
	for(c = 0; c != ubColorCount; ++c) {
		// Extract channels
		r = (pSource[c] >> 8) & 0xF;
		g = (pSource[c] >> 4) & 0xF;
		b = (pSource[c])      & 0xF;
		
		// Dim color
		r = ((r * ubLevel)/15) & 0xF;
		g = ((g * ubLevel)/15) & 0xF;
		b = ((b * ubLevel)/15) & 0xF;
		
		// Output
		pDest[c] = (r << 8) | (g << 4) | b;
	}
}