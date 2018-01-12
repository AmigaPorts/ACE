#ifndef GUARD_SHOWCASE_MENU_MENULIST_H
#define GUARD_SHOWCASE_MENU_MENULIST_H

#include <ace/types.h>
#include <ace/types.h>
#include <ace/utils/font.h>

/* ****************************************************************** DEFINES */

#define MENULIST_HIDDEN 0
#define MENULIST_DISABLED 1
#define MENULIST_ENABLED 2

/* ******************************************************************** TYPES */

typedef void (*tMenuActivateCb)(void);

typedef struct _tMenuEntry {
	char *szText;                    /// Displayed text
	UBYTE ubDisplay;                 /// 0 to disable
	tTextBitMap *pBitMap;
} tMenuEntry;

typedef struct _tMenuList {
	tUwCoordYX sCoord;            /// Top-left point of list,
	                              /// centering must be done by ubFontFlags
	tMenuEntry *pEntries;         /// Entry array
	tBitMap *pDestBitMap;         /// BitMap to redraw on
	tFont *pFont;                 /// Font for drawing list
	UBYTE ubFontFlags;            /// Font flags for drawing list
	UBYTE ubColor;                /// Color idx for drawing list
	UBYTE ubColorDisabled;        /// Ditto, disabled pos
	UBYTE ubColorSelected;        /// Ditto, selected pos
	UBYTE ubSpacing;              /// Y-space between positions
	UBYTE ubCount;                /// Entry count on list
	UBYTE ubSelected;             /// Currently selected entry
} tMenuList;

typedef void (*tMenuSelectCb)(
	IN struct _tMenuList *pList,
	IN UBYTE ubPosIdx
);

/* ****************************************************************** GLOBALS */

/* **************************************************************** FUNCTIONS */

tMenuList *menuListCreate(
	IN UWORD uwX,
	IN UWORD uwY,
	IN UBYTE ubCount,
	IN UBYTE ubSpacing,
	IN tFont *pFont,
	IN UBYTE ubFontFlags,
	IN UBYTE ubColor,
	IN UBYTE ubColorDisabled,
	IN UBYTE ubColorSelected,
	IN tBitMap *pDestBitMap
);

void menuListDestroy(
	IN tMenuList *pList
);

void menuListSetEntry(
	IN tMenuList *pList,
	IN UBYTE ubIdx,
	IN UBYTE ubDisplay,
	IN char *szText
);

void menuListDrawPos(
	IN tMenuList *pList,
	IN UBYTE ubIdx
);

void menuListDraw(
	IN tMenuList *pList
);

void menuListMove(
	IN tMenuList *pList,
	IN BYTE bMoveDir
);

void menuListResetEntries(tMenuList *pList, UBYTE ubCount);

/* ****************************************************************** INLINES */

/* ******************************************************************* MACROS */

#endif
