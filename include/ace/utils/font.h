#ifndef GUARD_ACE_UTIL_FONT_H
#define GUARD_ACE_UTIL_FONT_H

/**
 *  Font & text drawing utils.
 *  @todo Consider changing prefix to 'txt' or splitting to two modules:
 *        'font' and 'txt'.
 */

#include <ace/types.h>
#include <ace/managers/blit.h>
#include <ace/utils/bitmap.h>

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

/**
 *  @brief The font structure.
 *  All font glyphs are stored in continuous 1bb bitmap. Its width is
 *  determined by glyph count and height by font size.
 *  Not all glyphs in codepage must be included - all missing glyphs share
 *  offset with next implemented glyph.
 */
typedef struct _tFont {
	UWORD uwWidth;       ///< Packed font bitmap width.
	UWORD uwHeight;      ///< Packed font bitmap height.
	UBYTE ubChars;       ///< Glyph count in font.
	UWORD *pCharOffsets; ///< Glyph offsets in packed bitmap.
	tBitMap *pRawData;   ///< Pointer to packed bitmap.
} tFont;

/**
 *  @brief The 1bpp bitmap buffer containing text assembled from glyps.
 *  Since text assembly from chars is time-consuming process, it is usually
 *  better to store once assembled text for future redraws.
 */
typedef struct _tTextBitMap {
	tBitMap *pBitMap;    ///< Word-aligned bitmap buffer with pre-drawn text.
	UWORD uwActualWidth; ///< Actual text width for precise blitting.
} tTextBitMap;

/* Globals */

/* Functions */

/**
 *  @brief Creates font instance from specified file.
 *
 *  @param szFontName Font file path to be loaded.
 *  @return pointer to loaded font.
 *
 *  @see fontDestroy()
 */
tFont *fontCreate(
	IN const char *szFontName
);

/**
 *  @brief Destroys given font instance.
 *
 *  @param pFont Font to be destroyed.
 *
 *  @see fontCreate()
 */
void fontDestroy(
	IN tFont *pFont
);

/**
 *  @brief Creates text bitmap with specified font, containing given text.
 *  Treat as cache - allows faster reblit of text without need
 *  of assembling it again.
 *
 *  @param pFont  Font to be used during text assembly.
 *  @param szText String to be printed on bitmap buffer.
 *  @return Newly-created text bitmap pointer.
 *
 *  @see fontDestroyTextbitMap()
 *  @see fontDrawTextBitMap()
 */
tTextBitMap *fontCreateTextBitMap(
	IN tFont *pFont,
	IN const char *szText
);

/**
 *  @brief Destroys specified buffered text bitmap.
 *
 *  @param pTextBitMap Text bitmap to be destroyed.
 *
 *  @see fontCreateTextBitMap()
 */
void fontDestroyTextBitMap(
	IN tTextBitMap *pTextBitMap
);

/**
 *  @brief Draws specified text bitmap at given position, color and flags.
 *
 *  @param pDest       Destination bitmap.
 *  @param pTextbitMap Source text bitmap.
 *  @param uwX         X position on destination bitmap.
 *  @param uwY         Y position on destination bitmap.
 *  @param ubColor     Desired text color.
 *  @param ubFlags     Text draw flags (FONT_*).
 *
 *  @see fontCreateTextBitMap()
 *  @see fontDrawStr()
 */
void fontDrawTextBitMap(
	IN tBitMap *pDest,
	IN tTextBitMap *pTextBitMap,
	IN UWORD uwX,
	IN UWORD uwY,
	IN UBYTE ubColor,
	IN UBYTE ubFlags
);

/**
 *  @brief Writes one-time texts on specified destination bitmap.
 *  This function should be used very carefully, as text assembling is
 *  time-consuming. If same text is going to be redrawn in game loop, its bitmap
 *  buffer should be stored and used for redraw.
 *
 *  @param pDest Destination bitmap.
 *  @param pFont   Font to be used for text assembly.
 *  @param uwX     X position on destination bitmap.
 *  @param uwY     Y position on destination bitmap.
 *  @param szText  String to be printed on destination bitmap.
 *  @param ubColor Desired text color.
 *  @param ubFlags Text draw flags (FONT_*).
 *
 *  @see fontDrawTextBitMap()
 */
void fontDrawStr(
	IN tBitMap *pDest,
	IN tFont *pFont,
	IN UWORD uwX,
	IN UWORD uwY,
	IN const char *szText,
	IN UBYTE ubColor,
	IN UBYTE ubFlags
);

#endif
