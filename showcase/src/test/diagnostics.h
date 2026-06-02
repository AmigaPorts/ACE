/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _DIAGNOSTICS_H_
#define _DIAGNOSTICS_H_

#include <ace/types.h>
#include <ace/managers/state.h>
#include <ace/utils/font.h>

void gsTestDiagSimpleBufferCreate(void);
void gsTestDiagSimpleBufferLoop(void);
void gsTestDiagSimpleBufferDestroy(void);

void gsTestDiagScrollTileBufferCreate(void);
void gsTestDiagScrollTileBufferLoop(void);
void gsTestDiagScrollTileBufferDestroy(void);

void diagnosticsChangeTo(UBYTE ubTestIndex);
void diagnosticsNextTest(void);
void diagnosticsSelectSimpleBufferBpp(UBYTE ubBpp);
void diagnosticsToggleSimpleBufferEhb(void);
void diagnosticsSelectSimpleBufferFmode(UBYTE ubFmode);
void diagnosticsShowMenu(void);
void diagnosticsShowSimpleBuffer(void);
void diagnosticsShowScrollTileBuffer(void);

UBYTE diagnosticsGetCurrentBpp(void);
UBYTE diagnosticsGetCurrentFmode(void);
UBYTE diagnosticsIsCurrentEhb(void);
UBYTE diagnosticsIsCurrentAga(void);
const char *diagnosticsGetCurrentName(void);
tFont *diagnosticsGetFont(void);
tTextBitMap *diagnosticsGetTextBitMap(void);

#endif // _DIAGNOSTICS_H_
