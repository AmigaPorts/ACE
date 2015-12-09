#ifndef GUARD_ACE_UTIL_FONT_H
#define GUARD_ACE_UTIL_FONT_H

#include <clib/exec_protos.h> // Amiga typedefs

#include "config.h"
#include "managers/blit.h"
#include "utils/bitmap.h"

/* Types */
#define FONT_LEFT    0
#define FONT_RIGHT   1
#define FONT_HCENTER 2
#define FONT_TOP     0
#define FONT_BOTTOM  4
#define FONT_VCENTER 8
#define FONT_SHADOW  16
#define FONT_COOKIE  32
#define FONT_LAZY    64
#define FONT_CENTER (FONT_HCENTER|FONT_VCENTER)

typedef struct {
	UWORD uwWidth;
	UWORD uwHeight;
	UBYTE ubChars;
	UWORD *pCharOffsets;
	tBitMap *pRawData;
} tFont;

typedef struct {
	tBitMap *pBitMap;
	UWORD uwActualWidth;
} tTextBitMap;

/* Globals */

/* Functions */
tFont *fontCreate(
	IN char *szFontName
);

void fontDestroy(
	IN tFont *pFont
);

tTextBitMap *fontCreateTextBitMap(
	IN tFont *pFont,
	IN char *szText
);

void fontDestroyTextBitMap(
	IN tTextBitMap *pTextBitMap
);

void fontDrawTextBitMap(
	IN tBitMap *pDest,
	IN tTextBitMap *pTextBitMap,
	IN UWORD uwX,
	IN UWORD uwY,
	IN UBYTE ubColor,
	IN UBYTE ubFlags
);

void fontDrawTextBitMapCookie(
	IN tBitMap *pDest,
	IN tTextBitMap *pTextBitMap,
	IN UWORD uwX,
	IN UWORD uwY,
	IN UBYTE ubColor
);

void fontDrawTextBitMapRaw(
	IN tBitMap *pDest,
	IN tTextBitMap *pTextBitMap,
	IN UWORD uwX,
	IN UWORD uwY,
	IN UBYTE ubColor
);

void fontDrawStr(
	IN tBitMap *pDest,
	IN tFont *pFont,
	IN UWORD uwX,
	IN UWORD uwY,
	IN char *szText,
	IN UBYTE ubColor,
	IN UBYTE ubFlags
);

#endif