#ifndef GUARD_ACE_MANAGER_LOG_H
#define GUARD_ACE_MANAGER_LOG_H

#include <stdio.h> // fopen etc
#include <string.h> // strlen etc
#include <stdarg.h> // va_list etc
#include <clib/exec_protos.h> // Amiga typedefs
#include <clib/graphics_protos.h> // Amiga typedefs

#include "config.h"
#include "managers/timer.h"

#ifndef LOG_FILE_NAME
#define LOG_FILE_NAME "game.log"
#endif

/* Types */


typedef struct {
	UWORD uwCount;
	UWORD uwCurrDelta;
	ULONG ulMin;
	ULONG ulMax;
	ULONG ulStartTime;
	ULONG *pDeltas;
	char *szName;
} tAvg;


typedef struct {
	FILE *pFile;
	UBYTE ubIndent;
	UBYTE ubIsLastWasInline;
	ULONG pTimeStack[256];
	char szTimeBfr[255];
	UBYTE ubBlockEmpty;
	UBYTE ubShutUp;
} tLogManager;

#ifdef GAME_DEBUG
/* Globals */
extern tLogManager g_sLogManager;

/* Functions - general */

void _logOpen(void);
void _logClose(void);

void _logPushIndent(void);
void _logPopIndent(void);

void _logWrite(
	IN char *szFormat,
	IN ...
);

/* Functions - block logging */

void _logBlockBegin(
	IN char *szBlockName,
	IN ...
);
void _logBlockEnd(
	IN char *szBlockName
);

/* Functions - average block time */

tAvg *_logAvgCreate(char *szName, UWORD uwCount);
void _logAvgDestroy(tAvg *pAvg);
void _logAvgBegin(tAvg *pAvg);
void _logAvgEnd(tAvg *pAvg);
void _logAvgWrite(tAvg *pAvg);

/* Functions - struct dump */

void _logUCopList(
	IN struct UCopList *pUCopList
);
void _logBitMap(
	IN struct BitMap *pBitMap
);

# define logOpen() _logOpen()
# define logClose() _logClose()
# define logPushIndent() _logPushIndent()
# define logPopIndent() _logPopIndent()
# define logWrite(...) _logWrite(__VA_ARGS__)

# define logBlockBegin(...) _logBlockBegin(__VA_ARGS__)
# define logBlockEnd(szBlockName) _logBlockEnd(szBlockName)

# define logAvgCreate(szName, wCount) _logAvgCreate(szName, wCount)
# define logAvgDestroy(pAvg) _logAvgDestroy(pAvg)
# define logAvgBegin(pAvg) _logAvgBegin(pAvg)
# define logAvgEnd(pAvg) _logAvgEnd(pAvg)
# define logAvgWrite(pAvg) _logAvgWrite(pAvg)

# define logUCopList(pUCopList) _logUCopList(pUCopList)
# define logBitMap(pBitMap) _logBitMap(pBitMap)
#else
# define logOpen()
# define logClose()
# define logPushIndent()
# define logPopIndent()
# define logWrite(...)

# define logBlockBegin(...)
# define logBlockEnd(szBlockName)

# define logAvgCreate(szName, wCount) 0
# define logAvgDestroy(pAvg)
# define logAvgBegin(pAvg)
# define logAvgEnd(pAvg)
# define logAvgWrite(pAvg)

# define logUCopList(pUCopList)
# define logBitMap(pBitMap)
#endif

#endif