/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "test/interleaved.h"
#include <ace/utils/extview.h>
#include <ace/utils/palette.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include "main.h"
#include "menu/menu.h"

static tView *s_pTestInterleavedView;
static tVPort *s_pTestInterleavedVPort;
static tSimpleBufferManager *s_pTestInterleavedBfr;

void gsTestInterleavedCreate(void) {
	s_pTestInterleavedView = viewCreate(0,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_DONE
	);
	s_pTestInterleavedVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pTestInterleavedView,
		TAG_VPORT_BPP, SHOWCASE_BPP,
		TAG_DONE
	);
	s_pTestInterleavedBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pTestInterleavedVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_DONE
	);
	paletteLoad(
		"data/amidb32.plt", s_pTestInterleavedVPort->pPalette, 1 << SHOWCASE_BPP
	);
	bitmapLoadFromFile(
		s_pTestInterleavedBfr->pBack, "data/32c_pal_interleaved.bm", 0, 0
	);

	systemUnuse();
	viewLoad(s_pTestInterleavedView);
}

void gsTestInterleavedLoop(void) {
	if (keyUse(KEY_ESCAPE)) {
		gameChangeState(gsMenuCreate, gsMenuLoop, gsMenuDestroy);
		return;
	}
}

void gsTestInterleavedDestroy(void) {
	systemUse();
	viewDestroy(s_pTestInterleavedView);
}
