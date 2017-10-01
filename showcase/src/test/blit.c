#include "test/blit.h"

#include <ace/utils/extview.h>
#include <ace/managers/game.h>
#include <ace/managers/blit.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/viewport/simplebuffer.h>

#include "menu/menu.h"

static tView *s_pTestBlitView;
static tVPort *s_pTestBlitVPort;
static tSimpleBufferManager *s_pTestBlitBfr;

static UWORD s_uwX, s_uwY;
static UBYTE s_ubType;
static UBYTE (*s_fnKeyPoll)(UBYTE ubKeyCode);

void gsTestBlitCreate(void) {
	// Prepare view & viewport
	s_pTestBlitView = viewCreate(0,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_DONE
	);
	s_pTestBlitVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pTestBlitView,
		TAG_VPORT_BPP, WINDOW_SCREEN_BPP,
		TAG_DONE
	);
	s_pTestBlitBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pTestBlitVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
		TAG_DONE
	);
	s_pTestBlitVPort->pPalette[0] = 0x000;
	s_pTestBlitVPort->pPalette[1] = 0xAAA;
	s_pTestBlitVPort->pPalette[2] = 0x666;
	s_pTestBlitVPort->pPalette[3] = 0xFFF;
	s_pTestBlitVPort->pPalette[4] = 0x111;
	
	// Loop vars
	s_uwX = WINDOW_SCREEN_WIDTH>>1;
	s_uwY = WINDOW_SCREEN_HEIGHT>>1;
	s_ubType = TYPE_RECT;
	s_fnKeyPoll = keyUse;
	
	// Display view with its viewports
	viewLoad(s_pTestBlitView);
}

void gsTestBlitLoop(void) {
	static BYTE bSpeedX = 0, bSpeedY = 0;
	
	if (keyUse(KEY_ESCAPE)) {
		gameChangeState(gsMenuCreate, gsMenuLoop, gsMenuDestroy);
		return;
	}
	
	// Erase previous blit using old type & coords
	if(s_ubType & TYPE_SAVEBG) {
		//TODO: Restore BG
	}
	else
		blitRect(s_pTestBlitBfr->pBuffer, s_uwX, s_uwY, 16, 16, 0);
	
	// Update type
	
	// Rapid movement
	if(keyUse(KEY_R)) {
		s_ubType ^= TYPE_RAPID;
		if(s_ubType & TYPE_RAPID)
			s_fnKeyPoll = keyCheck;
		else
			s_fnKeyPoll = keyUse;
	}
	
	// Auto movement
	if(keyUse(KEY_A)) {
		s_ubType ^= TYPE_AUTO;
		if(s_ubType & TYPE_AUTO) {
			bSpeedX = 1;
			bSpeedY = 1;
		}
	}
	
	// Save BG
	if(keyUse(KEY_B)) {
		s_ubType ^= TYPE_SAVEBG;
		if(s_ubType & TYPE_SAVEBG) {
			// TODO: Draw whole BG
		}
	}
	
	// Rect mode
	if(keyUse(KEY_1))
		s_ubType = TYPE_RECT;
	
	if(s_ubType & TYPE_AUTO) {
		if(bSpeedX > 0)
			if(s_uwX < 320-16)
				++s_uwX;
			else
				bSpeedX = -1;
		else if(bSpeedX < 0)
			if(s_uwX)
				--s_uwX;
			else
				bSpeedX = 1;
			
		if(bSpeedY > 0)
			if(s_uwY < 256-16)
				++s_uwY;
			else
				bSpeedY = -1;
		else if(bSpeedY < 0)
			if(s_uwY)
				--s_uwY;
			else
				bSpeedY = 1;
	}
	else {
		if(s_fnKeyPoll(KEY_UP) && s_uwY)
			--s_uwY;
		if(s_fnKeyPoll(KEY_DOWN) && s_uwY < 256-16)
			++s_uwY;
		if(s_fnKeyPoll(KEY_LEFT) && s_uwX)
			--s_uwX;
		if(s_fnKeyPoll(KEY_RIGHT) && s_uwX < 320-16)
			++s_uwX;
	}
	
	// Reblit using new type & coords
	if(s_ubType & TYPE_SAVEBG) {
		//TODO: Save BG beneath new bob
	}
	// if(s_ubType & TYPE_RECT)
		blitRect(s_pTestBlitBfr->pBuffer, s_uwX, s_uwY, 16, 16, 3);
	WaitTOF();
	
}

void gsTestBlitDestroy(void) {
	// Destroy buffer, view & viewport
	viewDestroy(s_pTestBlitView);
}