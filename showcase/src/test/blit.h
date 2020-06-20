/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _SHOWCASE_TEST_BLIT_H_
#define _SHOWCASE_TEST_BLIT_H_

//---------------------------------------------------------------------- DEFINES

#define TYPE_RECT 0
#define TYPE_AUTO 128
#define TYPE_RAPID 64
#define TYPE_SAVEBG 32

//------------------------------------------------------------------------ TYPES

//---------------------------------------------------------------------- GLOBALS

//-------------------------------------------------------------------- FUNCTIONS

void gsTestBlitCreate(void);
void gsTestBlitLoop(void);
void gsTestBlitDestroy(void);

//---------------------------------------------------------------------- INLINES

//----------------------------------------------------------------------- MACROS

#endif // _SHOWCASE_TEST_BLIT_H_
