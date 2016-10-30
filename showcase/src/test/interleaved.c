#include "test/interleaved.h"

#include <ace/config.h>
#include <ace/utils/extview.h>
#include <ace/utils/palette.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include "menu/menu.h"

static tView *s_pTestInterleavedView;
static tVPort *s_pTestInterleavedVPort;
static tSimpleBufferManager *s_pTestInterleavedBfr;

void gsTestInterleavedCreate(void) {
	s_pTestInterleavedView = viewCreate(V_GLOBAL_CLUT);
	s_pTestInterleavedVPort = vPortCreate(s_pTestInterleavedView, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT, WINDOW_SCREEN_BPP, 0);
	s_pTestInterleavedBfr = simpleBufferCreate(s_pTestInterleavedVPort, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT, BMF_CLEAR | BMF_INTERLEAVED);
	
	paletteLoad("data/amidb32.plt", s_pTestInterleavedVPort->pPalette, 1 << WINDOW_SCREEN_BPP);
	bitmapLoadFromFile(s_pTestInterleavedBfr->pBuffer, "data/32c_pal_interleaved.bm", 0, 0);
	// bitmapSaveBMP(s_pTestInterleavedBfr->pBuffer, s_pTestInterleavedVPort->pPalette, "dump.bmp");
	
	viewLoad(s_pTestInterleavedView);
}

void gsTestInterleavedLoop(void) {
	if (keyUse(KEY_ESCAPE)) {
		gameChangeState(gsMenuCreate, gsMenuLoop, gsMenuDestroy);
		return;
	}
}

void gsTestInterleavedDestroy(void) {
	viewDestroy(s_pTestInterleavedView);
}
