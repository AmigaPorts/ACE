/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_LOG_H_
#define _ACE_MANAGERS_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h> // strlen etc
#include <stdarg.h> // va_list etc
#include <ace/types.h>
#include <ace/managers/timer.h>
#include <ace/utils/file.h>

// Types

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
	WORD wInterruptDepth;
	UBYTE wasLastInline;
	ULONG pTimeStack[256];
	char szTimeBfr[255];
	UBYTE isBlockEmpty;
	UBYTE ubShutUp;
} tLogManager;

#ifdef ACE_DEBUG
// Globals
extern tLogManager g_sLogManager;

// Functions - general

void _logOpen(const char *szFilePath);
void _logClose(void);

void _logPushIndent(void);
void _logPopIndent(void);

void _logPushInt(void);
void _logPopInt(void);

void _logWrite(char *szFormat, ...) __attribute__ ((format (printf, 1, 2)));

void _logWriteVa(char *szFormat, va_list vaArgs);

// Functions - block logging

void _logBlockBegin(char *szBlockName, ...) __attribute__ ((format (printf, 1, 2)));
void _logBlockEnd(char *szBlockName);

// Functions - average block time

tAvg *_logAvgCreate(char *szName, UWORD uwCount);
void _logAvgDestroy(tAvg *pAvg);
void _logAvgBegin(tAvg *pAvg);
void _logAvgEnd(tAvg *pAvg);
void _logAvgWrite(tAvg *pAvg);

// Functions - general logging

#define logOpen(szFilePath) _logOpen(szFilePath)
#define logClose() _logClose()
#define logPushIndent() _logPushIndent()
#define logPopIndent() _logPopIndent()
#define logPushInt() _logPushInt()
#define logPopInt() _logPopInt()
#define logWrite(...) _logWrite(__VA_ARGS__)
#define logWriteVa(szFormat, vaArgs) _logWriteVa(szFormat, vaArgs)

#define logBlockBegin(...) _logBlockBegin(__VA_ARGS__)
#define logBlockEnd(szBlockName) _logBlockEnd(szBlockName)

#define logAvgCreate(szName, wCount) _logAvgCreate(szName, wCount)
#define logAvgDestroy(pAvg) _logAvgDestroy(pAvg)
#define logAvgBegin(pAvg) _logAvgBegin(pAvg)
#define logAvgEnd(pAvg) _logAvgEnd(pAvg)
#define logAvgWrite(pAvg) _logAvgWrite(pAvg)

#else
#define logOpen(szFilePath)
#define logClose()
#define logPushIndent()
#define logPopIndent()
#define logPushInt()
#define logPopInt()
#define logWrite(...)
#define logWriteVa(szFormat, vaArgs)

#define logBlockBegin(...)
#define logBlockEnd(szBlockName)

#define logAvgCreate(szName, wCount) 0
#define logAvgDestroy(pAvg)
#define logAvgBegin(pAvg)
#define logAvgEnd(pAvg)
#define logAvgWrite(pAvg)
#endif // ACE_DEBUG

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_LOG_H_
