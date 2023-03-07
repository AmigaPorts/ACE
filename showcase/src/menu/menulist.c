/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "menu/menulist.h"
#include <ace/managers/log.h>
#include <ace/utils/font.h>

tMenuList *menuListCreate(
	UWORD uwX, UWORD uwY, UBYTE ubMaxCount, UBYTE ubSpacing,
	tFont *pFont, UBYTE ubFontFlags,
	UBYTE ubColor, UBYTE ubColorDisabled, UBYTE ubColorSelected,
	tBitMap *pDestBitMap
) {
	tMenuList *pList;

	pList = memAllocFast(sizeof(tMenuList));
	pList->pEntries = memAllocFastClear(ubMaxCount*sizeof(tMenuEntry));
	pList->ubMaxCount = ubMaxCount;
	pList->ubCount = 0;
	pList->ubSelected = 0;

	pList->pDestBitMap = pDestBitMap;

	// Font & colors
	pList->pFont = pFont;
	pList->ubColor = ubColor;
	pList->ubColorDisabled = ubColorDisabled;
	pList->ubColorSelected = ubColorSelected;
	pList->ubFontFlags = ubFontFlags;

	// Positioning & spacing
	pList->sCoord.uwX = uwX;
	pList->sCoord.uwY = uwY;
	pList->ubSpacing = ubSpacing;

	for(UBYTE i = 0; i < ubMaxCount; ++i) {
		pList->pEntries[i].pBitMap = fontCreateTextBitMap(
			192, pList->pFont->uwHeight
		);
	}

	return pList;
}

void menuListDestroy(tMenuList *pList) {
	for(UBYTE i = 0; i < pList->ubMaxCount; ++i) {
		fontDestroyTextBitMap(pList->pEntries[i].pBitMap);
	}
	memFree(pList->pEntries, pList->ubMaxCount * sizeof(tMenuEntry));
	memFree(pList, sizeof(tMenuList));
}

void menuListSetEntry(tMenuList *pList, UBYTE ubIdx, UBYTE ubDisplay, char *szText) {
	tMenuEntry *pEntry;

	pEntry = &pList->pEntries[ubIdx];
	pEntry->ubDisplayMode = ubDisplay;
	pEntry->szText = szText;

	// Update text bitmap
	fontFillTextBitMap(pList->pFont, pEntry->pBitMap, szText);
}

void menuListDrawPos(tMenuList *pList, UBYTE ubIdx) {
	UBYTE ubColor;
	UWORD uwX, uwY;

	if(pList->pEntries[ubIdx].ubDisplayMode == MENULIST_HIDDEN) {
		return;
	}

	uwX = pList->sCoord.uwX;
	uwY = pList->sCoord.uwY + ubIdx*(pList->ubSpacing+pList->pFont->uwHeight);

	// Color
	if(pList->pEntries[ubIdx].ubDisplayMode == MENULIST_DISABLED) {
		ubColor = pList->ubColorDisabled;
	}
	else if (pList->ubSelected == ubIdx) {
		ubColor = pList->ubColorSelected;
	}
	else {
		ubColor = pList->ubColor;
	}

	// Drawing
	fontDrawTextBitMap(
		pList->pDestBitMap,
		pList->pEntries[ubIdx].pBitMap,
		uwX, uwY, ubColor, pList->ubFontFlags
	);
}

void menuListDraw(tMenuList *pList) {
	UBYTE i;

	for (i = 0; i != pList->ubCount; ++i) {
		menuListDrawPos(pList, i);
	}
}

void menuListMove(tMenuList *pList, BYTE bMoveDir) {
	UBYTE ubOldIdx;

	ubOldIdx = pList->ubSelected;
	do {
		// Move cursor according to direction
		if(!pList->ubSelected && bMoveDir == -1) {
			pList->ubSelected = pList->ubCount-1;
		}
		else if(pList->ubSelected == pList->ubCount-1 && bMoveDir == 1) {
			pList->ubSelected = 0;
		}
		else {
			pList->ubSelected += bMoveDir;
		}
	} while(pList->pEntries[pList->ubSelected].ubDisplayMode != MENULIST_ENABLED);
	// Redraw positions: old and new
	menuListDrawPos(pList, ubOldIdx);
	menuListDrawPos(pList, pList->ubSelected);
}

void menuListResetCount(tMenuList *pList, UBYTE ubCount) {
	pList->ubCount = ubCount;
	pList->ubSelected = 0;
}
