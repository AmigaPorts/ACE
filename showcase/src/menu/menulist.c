#include "menu/menulist.h"
#include <ace/managers/log.h>
#include <ace/utils/font.h>

tMenuList *menuListCreate(
	UWORD uwX, UWORD uwY, UBYTE ubCount, UBYTE ubSpacing,
	tFont *pFont, UBYTE ubFontFlags,
	UBYTE ubColor, UBYTE ubColorDisabled, UBYTE ubColorSelected,
	tBitMap *pDestBitMap
) {
	tMenuList *pList;

	pList = memAllocFast(sizeof(tMenuList));
	pList->pEntries = memAllocFastClear(ubCount*sizeof(tMenuEntry));
	pList->ubCount = ubCount;
	pList->ubSelected = 0;
	
	pList->pDestBitMap = pDestBitMap;
	
	// Font & colors
	pList->pFont = pFont;
	pList->ubColor = ubColor;
	pList->ubColorDisabled = ubColorDisabled;
	pList->ubColorSelected = ubColorSelected;
	pList->ubFontFlags = ubFontFlags;
	
	// Positioning & spacing
	pList->sCoord.sUwCoord.uwX = uwX;
	pList->sCoord.sUwCoord.uwY = uwY;
	pList->ubSpacing = ubSpacing;
		
	return pList;
}

void menuListDestroy(tMenuList *pList) {
	menuListResetEntries(pList, 0);
	memFree(pList, sizeof(tMenuList));
}

void menuListSetEntry(tMenuList *pList, UBYTE ubIdx, UBYTE ubDisplay, char *szText) {
	tMenuEntry *pEntry;
	
	pEntry = &pList->pEntries[ubIdx];
	pEntry->ubDisplay = ubDisplay;
	pEntry->szText = szText;
	
	// Update text bitmap
	if(pEntry->pBitMap)
		fontDestroyTextBitMap(pEntry->pBitMap);
	pEntry->pBitMap = fontCreateTextBitMap(pList->pFont, szText);
}

void menuListDrawPos(tMenuList *pList, UBYTE ubIdx) {
	UBYTE ubColor;
	UWORD uwX, uwY;
	
	if(pList->pEntries[ubIdx].ubDisplay == MENULIST_HIDDEN)
		return;
	
	uwX = pList->sCoord.sUwCoord.uwX;
	uwY = pList->sCoord.sUwCoord.uwY + ubIdx*(pList->ubSpacing+pList->pFont->uwHeight);
	
	// Color
	if(pList->pEntries[ubIdx].ubDisplay == MENULIST_DISABLED)
		ubColor = pList->ubColorDisabled;
	else if (pList->ubSelected == ubIdx)
		ubColor = pList->ubColorSelected;
	else
		ubColor = pList->ubColor;
	
	// Drawing
	fontDrawTextBitMap(
		pList->pDestBitMap,
		pList->pEntries[ubIdx].pBitMap,
		uwX, uwY, ubColor, pList->ubFontFlags
	);
}

void menuListDraw(tMenuList *pList) {
	UBYTE i;

	for (i = 0; i != pList->ubCount; ++i)
		menuListDrawPos(pList, i);
}

void menuListMove(tMenuList *pList, BYTE bMoveDir) {
	UBYTE ubOldIdx;
	
	ubOldIdx = pList->ubSelected;
	do {
		// Move cursor according to direction
		if(!pList->ubSelected && bMoveDir == -1)
			pList->ubSelected = pList->ubCount-1;
		else if(pList->ubSelected == pList->ubCount-1 && bMoveDir == 1)
			pList->ubSelected = 0;
		else
			pList->ubSelected += bMoveDir;
	} while(pList->pEntries[pList->ubSelected].ubDisplay != MENULIST_ENABLED);
	// Redraw positions: old and new
	menuListDrawPos(pList, ubOldIdx);
	menuListDrawPos(pList, pList->ubSelected);
}

void menuListResetEntries(tMenuList *pList, UBYTE ubCount) {
	UBYTE i;
	
	for(i = pList->ubCount; i--;)
		if(pList->pEntries[i].pBitMap)
			fontDestroyTextBitMap(pList->pEntries[i].pBitMap);
	memFree(pList->pEntries, sizeof(tMenuEntry)*pList->ubCount);
	
	if(ubCount)
		pList->pEntries = memAllocFastClear(sizeof(tMenuEntry)*ubCount);
	pList->ubCount = ubCount;
	pList->ubSelected = 0;
}