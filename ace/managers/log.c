#include <ace/managers/log.h>
#ifdef GAME_DEBUG
/* Globals */
tLogManager g_sLogManager;

/* Functions */

/**
 * Base debug functions
 */

void _logOpen() {
	g_sLogManager.pFile = fopen(LOG_FILE_NAME, "w");
	g_sLogManager.ubIndent = 0;
	g_sLogManager.ubIsLastWasInline = 0;
	g_sLogManager.ubBlockEmpty = 1;
	g_sLogManager.ubShutUp = 0;
}

void _logPushIndent() {
	++g_sLogManager.ubIndent;
}

void _logPopIndent() {
	--g_sLogManager.ubIndent;
}

void _logWrite(char *szFormat, ...) {
	if(g_sLogManager.ubShutUp)
		return;
	if (g_sLogManager.pFile) {
		g_sLogManager.ubBlockEmpty = 0;
		if (!g_sLogManager.ubIsLastWasInline) {
			UBYTE bLogIndent = g_sLogManager.ubIndent;
			while (bLogIndent--)
				fprintf(g_sLogManager.pFile, "\t");
		}

		g_sLogManager.ubIsLastWasInline = szFormat[strlen(szFormat) - 1] != '\n';
		
		va_list vaArgs;
		va_start(vaArgs, szFormat);
		vfprintf(g_sLogManager.pFile, szFormat, vaArgs);
		va_end(vaArgs);

		fflush(g_sLogManager.pFile);
	}
}

void _logClose() {
	if (g_sLogManager.pFile)
		fclose(g_sLogManager.pFile);
	g_sLogManager.pFile = 0;
}

/**
 * Extended debug functions
 */

// Log blocks
void _logBlockBegin(char *szBlockName, ...) {
	if(g_sLogManager.ubShutUp)
		return;
	char szFmtBfr[512];
	char szStrBfr[1024];
	// make format string
	strcpy(szFmtBfr, "Block begin: ");
	strcat(szFmtBfr, szBlockName);
	strcat(szFmtBfr, "\n");
	
	va_list vaArgs;
	va_start(vaArgs, szBlockName);
	vsprintf(szStrBfr,szFmtBfr,vaArgs);
	va_end(vaArgs);
	
	logWrite(szStrBfr);
	g_sLogManager.pTimeStack[g_sLogManager.ubIndent] = timerGetPrec();
	logPushIndent();
	g_sLogManager.ubBlockEmpty = 1;
}

void _logBlockEnd(char *szBlockName) {
	if(g_sLogManager.ubShutUp)
		return;
	logPopIndent();
	timerFormatPrec(
		g_sLogManager.szTimeBfr, 
		timerGetDelta(
			g_sLogManager.pTimeStack[g_sLogManager.ubIndent],
			timerGetPrec()
		)
	);
	if(g_sLogManager.ubBlockEmpty) {
		// empty block - collapse to single line
		g_sLogManager.ubIsLastWasInline = 1;
		fseek(g_sLogManager.pFile, -1, SEEK_CUR);
		logWrite("...OK, time: %s\n", g_sLogManager.szTimeBfr);
	}
	else
		logWrite("Block end: %s, time: %s\n", szBlockName, g_sLogManager.szTimeBfr);
	g_sLogManager.ubBlockEmpty = 0;
}

// Average logging
/**
 *
 */
tAvg *_logAvgCreate(char *szName, UWORD uwCount) {
	tAvg *pAvg = memAllocFast(sizeof(tAvg));
	pAvg->szName = szName;
	pAvg->uwCount = uwCount;
	pAvg->uwCurrDelta = 0;
	pAvg->pDeltas = memAllocFast(uwCount*sizeof(tAvg));
	pAvg->ulMin = 0xFFFFFFFF;
	pAvg->ulMax = 0;
	return pAvg;
}

/**
 *
 */
void _logAvgDestroy(tAvg *pAvg) {
	logAvgWrite(pAvg);
	memFree(pAvg->pDeltas, pAvg->uwCount*sizeof(tAvg));
	memFree(pAvg, sizeof(tAvg));
}

/**
 *
 */
void _logAvgBegin(tAvg *pAvg) {
	pAvg->ulStartTime = timerGetPrec();
}

/**
 *
 */
void _logAvgEnd(tAvg *pAvg) {
	// Calculate timestamp
	pAvg->pDeltas[pAvg->uwCurrDelta] = timerGetDelta(pAvg->ulStartTime, timerGetPrec());
	// Update min/max
	if(pAvg->pDeltas[pAvg->uwCurrDelta] > pAvg->ulMax)
		pAvg->ulMax = pAvg->pDeltas[pAvg->uwCurrDelta];
	if(pAvg->pDeltas[pAvg->uwCurrDelta] < pAvg->ulMin)
		pAvg->ulMin = pAvg->pDeltas[pAvg->uwCurrDelta];
	++pAvg->uwCurrDelta;
	// Roll 
	if(pAvg->uwCurrDelta == pAvg->uwCount)
		pAvg->uwCurrDelta = 0;
}

/**
 *
 */
void _logAvgWrite(tAvg *pAvg) {
	UWORD i;
	ULONG ulAvg;
	char szAvg[15];
	char szMin[15];
	char szMax[15];
	
	// Calculate average time
	for(i = pAvg->uwCount; i--;)
		ulAvg += pAvg->pDeltas[i];
	ulAvg /= pAvg->uwCount;
	
	// Display info
	timerFormatPrec(szAvg, ulAvg);
	timerFormatPrec(szMin, pAvg->ulMin);
	timerFormatPrec(szMax, pAvg->ulMax);
	logWrite("Avg %s: %s, min: %s, max: %s\n", pAvg->szName, szAvg, szMin, szMax);
}

// Copperlist debug
void _logUCopList(struct UCopList *pUCopList) {
	logBlockBegin("logUCopList(pUCopList: %p)", pUCopList);
	logWrite("Next: %p\n", pUCopList->Next);
	logWrite("FirstCopList: %p\n", pUCopList->FirstCopList);
	logWrite("CopList: %p\n", pUCopList->CopList);
	
	logBlockBegin("pUCopList->CopList");
	logWrite("Next: %p\n", pUCopList->CopList->Next);
	logWrite("_CopList: %p\n", pUCopList->CopList->_CopList);
	logWrite("_ViewPort: %p\n", pUCopList->CopList->_ViewPort);
	logWrite("CopIns: %p\n", pUCopList->CopList->CopIns);
	logWrite("CopPtr: %p\n", pUCopList->CopList->CopPtr);
	logWrite("CopLStart: %p\n", pUCopList->CopList->CopLStart);
	logWrite("CopSStart: %p\n", pUCopList->CopList->CopSStart);
	logWrite("Count: %u\n", pUCopList->CopList->Count);
	logWrite("MaxCount: %u\n", pUCopList->CopList->MaxCount);
	logWrite("DyOffset: %u\n", pUCopList->CopList->DyOffset);
	logBlockEnd("pUCopList->CopList");
	
	logBlockEnd("logUCopList()");
}

void _logBitMap(struct BitMap *pBitMap) {
	UBYTE i;
	logBlockBegin("logBitMap(pBitMap: %p)", pBitMap);
	logWrite("BytesPerRow: %u\n", pBitMap->BytesPerRow);
	logWrite("Rows: %u\n", pBitMap->Rows);
	logWrite("Flags: %hu\n", pBitMap->Flags);
	logWrite("Depth: %hu\n", pBitMap->Depth);
	logWrite("pad: %u\n", pBitMap->pad);
	// since Planes is always 8-long, dump all its entries
	for(i = 0; i != 8; ++i)
		logWrite("Planes[%hu]: %p\n", i, pBitMap->Planes[i]);
	logBlockEnd("logBitMap");
}
#endif