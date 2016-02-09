#ifndef GUARD_ACE_UTIL_PALETTE_H
#define GUARD_ACE_UTIL_PALETTE_H

#include <stdio.h> // FILE etc
#include <clib/exec_protos.h> // Amiga typedefs
// #include <clib/graphics_protos.h> // BitMap etc

#include "config.h"
#include "utils/extview.h"

/* Types */
#define BLIT_DEFAULT 0xC0, 0xFF, 0

/* Globals */

/* Functions */
void paletteLoad(
	IN char *szFileName,
	OUT UWORD *pPalette
);

// void paletteSetVPColor(
	// IN tExtVPort *pVPort,
	// IN UBYTE ubIdx,
	// IN UWORD uwColor
// );

// void paletteCopy(
	// IN tExtVPort *pVPortSrc,
	// IN tExtVPort *pVPortDest
// );

#endif