/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GUARD_ACE_MANAGER_LOG_H
#define GUARD_ACE_MANAGER_LOG_H

#include <string.h> // strlen etc
#include <stdarg.h> // va_list etc
#include <ace/types.h>
#include <ace/managers/timer.h>
#include <ace/utils/file.h>

/* Types */

typedef struct _tAvg {
	UWORD uwAllocCount;
	UWORD uwUsedCount;
	UWORD uwCurrDelta;
	ULONG ulMin;
	ULONG ulMax;
	ULONG ulStartTime;
	ULONG *pDeltas;
	char *szName;
} tAvg;


typedef struct _tLogManager {
	tFile *pFile;
	UBYTE ubIndent;
	UBYTE wasLastInline;
	ULONG pTimeStack[256];
	char szTimeBfr[255];
	UBYTE ubBlockEmpty;
	UBYTE ubShutUp;
} tLogManager;

#ifdef ACE_DEBUG
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

#define logOpen() _logOpen()
#define logClose() _logClose()
#define logPushIndent() _logPushIndent()
#define logPopIndent() _logPopIndent()
#define logWrite(...) _logWrite(__VA_ARGS__)

#define logBlockBegin(...) _logBlockBegin(__VA_ARGS__)
#define logBlockEnd(szBlockName) _logBlockEnd(szBlockName)

#define logAvgCreate(szName, wCount) _logAvgCreate(szName, wCount)
#define logAvgDestroy(pAvg) _logAvgDestroy(pAvg)
#define logAvgBegin(pAvg) _logAvgBegin(pAvg)
#define logAvgEnd(pAvg) _logAvgEnd(pAvg)
#define logAvgWrite(pAvg) _logAvgWrite(pAvg)

#else
#define logOpen()
#define logClose()
#define logPushIndent()
#define logPopIndent()
#define logWrite(...)

#define logBlockBegin(...)
#define logBlockEnd(szBlockName)

#define logAvgCreate(szName, wCount) 0
#define logAvgDestroy(pAvg)
#define logAvgBegin(pAvg)
#define logAvgEnd(pAvg)
#define logAvgWrite(pAvg)
#endif // ACE_DEBUG

#endif // GUARD_ACE_MANAGER_LOG_H
