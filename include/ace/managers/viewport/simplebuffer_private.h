#ifndef _ACE_MANAGERS_VIEWPORT_SIMPLEBUFFER_PRIVATE_H_
#define _ACE_MANAGERS_VIEWPORT_SIMPLEBUFFER_PRIVATE_H_

#include <ace/managers/viewport/simplebuffer.h>

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

static inline void simpleBufferSetBack(tSimpleBufferManager *pManager, tBitMap *pBack) {
#if defined(ACE_DEBUG)
	if(pManager->pBack && pManager->pBack->Depth != pBack->Depth) {
		logWrite("ERR: buffer bitmaps differ in BPP!\n");
		return;
	}
#endif
	pManager->pBack = pBack;
}

static inline void simpleBufferSetFront(tSimpleBufferManager *pManager, tBitMap *pFront) {
	logBlockBegin(
		"simpleBufferSetFront(pManager: %p, pFront: %p)",
		pManager, pFront
	);
#if defined(ACE_DEBUG)
	if(pManager->pFront && pManager->pFront->Depth != pFront->Depth) {
		logWrite("ERR: buffer bitmaps differ in BPP!\n");
		return;
	}
#endif
	pManager->pFront = pFront;
	logBlockEnd("simplebufferSetFront()");
}


#endif // _ACE_MANAGERS_VIEWPORT_SIMPLEBUFFER_PRIVATE_H_
