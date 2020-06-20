/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _SHOWCASE_MENU_MENULIST_H_
#define _SHOWCASE_MENU_MENULIST_H_

#include <ace/types.h>
#include <ace/types.h>
#include <ace/utils/font.h>

//---------------------------------------------------------------------- DEFINES

#define MENULIST_HIDDEN 0
#define MENULIST_DISABLED 1
#define MENULIST_ENABLED 2

//------------------------------------------------------------------------ TYPES

typedef void (*tMenuActivateCb)(void);

typedef struct _tMenuEntry {
	char *szText;        ///< Displayed text
	UBYTE ubDisplayMode; ///< see MENULIST_* defines
	tTextBitMap *pBitMap;
} tMenuEntry;

typedef struct _tMenuList {
	tUwCoordYX sCoord;     ///< Top-left point of list,
	                       ///< centering must be done by ubFontFlags
	tMenuEntry *pEntries;  ///< Entry array
	tBitMap *pDestBitMap;  ///< BitMap to redraw on
	tFont *pFont;          ///< Font for drawing list
	UBYTE ubFontFlags;     ///< Font flags for drawing list
	UBYTE ubColor;         ///< Color idx for drawing list
	UBYTE ubColorDisabled; ///< Ditto, disabled pos
	UBYTE ubColorSelected; ///< Ditto, selected pos
	UBYTE ubSpacing;       ///< Y-space between positions
	UBYTE ubCount;         ///< Entry count on list
	UBYTE ubMaxCount;
	UBYTE ubSelected;      ///< Currently selected entry
} tMenuList;

typedef void (*tMenuSelectCb)(struct _tMenuList *pList, UBYTE ubPosIdx);

//---------------------------------------------------------------------- GLOBALS

//-------------------------------------------------------------------- FUNCTIONS

tMenuList *menuListCreate(
	UWORD uwX, UWORD uwY, UBYTE ubCount, UBYTE ubSpacing,
	tFont *pFont, UBYTE ubFontFlags,
	UBYTE ubColor, UBYTE ubColorDisabled, UBYTE ubColorSelected,
	tBitMap *pDestBitMap
);

void menuListDestroy(tMenuList *pList);

void menuListSetEntry(
	tMenuList *pList, UBYTE ubIdx, UBYTE ubDisplay, char *szText
);

void menuListDrawPos(tMenuList *pList, UBYTE ubIdx);

void menuListDraw(tMenuList *pList);

void menuListMove(tMenuList *pList, BYTE bMoveDir);

void menuListResetCount(tMenuList *pList, UBYTE ubCount);

//---------------------------------------------------------------------- INLINES

//----------------------------------------------------------------------- MACROS

#endif // _SHOWCASE_MENU_MENULIST_H_
