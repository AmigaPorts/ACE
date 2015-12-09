#include "palette.h"

void paletteLoad(char *szFileName, UWORD *pPalette) {
	FILE *pFile;
	UBYTE ubPaletteLength;
	
	logBlockBegin("paletteLoad(szFileName: %s, pPalette: %p)", szFileName, pPalette);
		
	pFile = fopen(szFileName, "r");
	fread(&ubPaletteLength, 1, 1, pFile);
	logWrite(" Color count: %u...", ubPaletteLength);
	fread(pPalette, sizeof(UWORD), ubPaletteLength, pFile);
	fclose(pFile);
	
	logBlockEnd("paletteLoad()");
}

// void paletteSetVPColor(tExtVPort *pVPort, UBYTE ubIdx, UWORD uwColor) {
	// pVPort->pPalette[ubIdx] = uwColor;
	// SetRGB4(&pVPort->sVPort, ubIdx, (uwColor >> 8) & 0xF, (uwColor >> 4) & 0xF, (uwColor >> 0) & 0xF);
// }

// void paletteCopy(tExtVPort *pVPortSrc, tExtVPort *pVPortDest) {
	// CopyMem(pVPortSrc->pPalette, pVPortDest->pPalette, 32*sizeof(UWORD));
// }
