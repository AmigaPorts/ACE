/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "examples/shooter_horizontal/shooter.h"
#include <string.h>
#include <ace/managers/advancedmultiplexedsprite.h>
#include <ace/managers/blit.h>
#include <ace/managers/joy.h>
#include <ace/managers/key.h>
#include <ace/managers/memory.h>
#include <ace/managers/sprite.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/custom.h>
#include "game.h"
#include "examples/shooter_horizontal/art.h"
#include "examples/shooter_horizontal/logic.h"
#include "examples/shooter_horizontal/stars.h"

#define SHOOTER_BPP 4
#define SHOOTER_SPRITE_COP_OFFSET 0
#define SHOOTER_BUFFER_COP_OFFSET 16
#define SHOOTER_SHOT_CHANNEL_COUNT 4

static const UBYTE s_pShotChannels[SHOOTER_SHOT_CHANNEL_COUNT] = {2, 6, 7, 3};
static const UBYTE s_pShotCapacities[SHOOTER_SHOT_CHANNEL_COUNT] = {
	SHOOTER_RED_CHANNEL_CAPACITY, SHOOTER_RED_CHANNEL_CAPACITY,
	SHOOTER_RED_CHANNEL_CAPACITY, SHOOTER_ENEMY_BULLET_COUNT
};

typedef struct _tShooterHudCache {
	ULONG ulScore;
	UBYTE ubLives, isGameOver, isValid;
} tShooterHudCache;

typedef struct _tShooterGame {
	tView *pView;
	tVPort *pVPort;
	tSimpleBufferManager *pBuffer;
	ULONG *pBlankSprite;
	tSprite *pPlayer[2];
	tBitMap *pPlayerArt[2];
	tMultiplexedSprite *pShots[SHOOTER_SHOT_CHANNEL_COUNT];
	tBitMap *pShotArt[2];
	tAdvancedMultiplexedSprite *pEnemies;
	tShooterState sState;
	/* Fixed identities: ACE swaps pFront/pBack, never these array entries. */
	tBitMap *pBackgrounds[2];
	tShooterStars sStars[2];
	tShooterHudCache pHudCaches[2];
	UBYTE pShotCounts[SHOOTER_SHOT_CHANNEL_COUNT];
	ULONG ulFrame; /* Lifetime ticks: shooterHorzInit must never reset this. */
	UBYTE isManagersCreated, isSystemUnused, isLoaded;
} tShooterGame;

static tShooterGame s_sGame;

/** One sprite pointer table precedes the only playfield WAIT/setup region. */
static UBYTE createDisplay(void) {
	UWORD uwCopperCount = SHOOTER_BUFFER_COP_OFFSET +
		simpleBufferGetRawCopperlistInstructionCount(SHOOTER_BPP);
	s_sGame.pView = viewCreate(0,
		TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
		TAG_VIEW_COPLIST_RAW_COUNT, uwCopperCount,
		TAG_VIEW_GLOBAL_PALETTE, 1,
		TAG_VIEW_WINDOW_WIDTH, SHOOTER_SCREEN_WIDTH,
		TAG_VIEW_WINDOW_HEIGHT, SHOOTER_SCREEN_HEIGHT, TAG_END
	);
	if(!s_sGame.pView) {
		return 0;
	}
	s_sGame.pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_sGame.pView, TAG_VPORT_BPP, SHOOTER_BPP,
		TAG_VPORT_WIDTH, SHOOTER_SCREEN_WIDTH,
		TAG_VPORT_HEIGHT, SHOOTER_SCREEN_HEIGHT, TAG_END
	);
	if(!s_sGame.pVPort) {
		return 0;
	}
	s_sGame.pBuffer = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_sGame.pVPort,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, SHOOTER_BUFFER_COP_OFFSET,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
		TAG_SIMPLEBUFFER_USE_X_SCROLLING, 0, TAG_END
	);
	if(!s_sGame.pBuffer) {
		return 0;
	}
	shooterHorzArtPalette(s_sGame.pVPort->pPalette);
	s_sGame.pBackgrounds[0] = s_sGame.pBuffer->pFront;
	s_sGame.pBackgrounds[1] = s_sGame.pBuffer->pBack;
	for(UBYTE ubBuffer = 0; ubBuffer < 2; ++ubBuffer) {
		shooterHorzArtBackground(s_sGame.pBackgrounds[ubBuffer]);
		shooterHorzStarsInit(&s_sGame.sStars[ubBuffer],
			s_sGame.pBackgrounds[ubBuffer]);
	}
	return 1;
}

/** Both constructors clear all eight pointers: run both before any Add. */
static UBYTE createManagers(void) {
	s_sGame.pBlankSprite = memAllocChipClear(sizeof(*s_sGame.pBlankSprite));
	if(!s_sGame.pBlankSprite) {
		return 0;
	}
	spriteManagerCreate(s_sGame.pView,
		SHOOTER_SPRITE_COP_OFFSET, s_sGame.pBlankSprite);
	multiplexedSpriteManagerCreate(s_sGame.pView,
		SHOOTER_SPRITE_COP_OFFSET, s_sGame.pBlankSprite);
	s_sGame.isManagersCreated = 1;
	return 1;
}

/** Basic sprites borrow separate low/high 2bpp images with writable headers. */
static UBYTE createPlayer(void) {
	for(UBYTE ubHalf = 0; ubHalf < 2; ++ubHalf) {
		s_sGame.pPlayerArt[ubHalf] = shooterHorzArtPlayer(ubHalf);
		if(!s_sGame.pPlayerArt[ubHalf]) {
			return 0;
		}
		s_sGame.pPlayer[ubHalf] = spriteAdd(ubHalf, s_sGame.pPlayerArt[ubHalf]);
		if(!s_sGame.pPlayer[ubHalf]) {
			return 0;
		}
	}
	spriteSetAttached(s_sGame.pPlayer[1], 1);
	return 1;
}

/** Two sources remain borrowed until all four ordinary channels are removed. */
static UBYTE createShots(void) {
	for(UBYTE ubSide = 0; ubSide < 2; ++ubSide) {
		s_sGame.pShotArt[ubSide] = shooterHorzArtShot(ubSide + 1);
		if(!s_sGame.pShotArt[ubSide]) {
			return 0;
		}
	}
	for(UBYTE ubStream = 0; ubStream < SHOOTER_SHOT_CHANNEL_COUNT; ++ubStream) {
		UBYTE ubCount = s_pShotCapacities[ubStream];
		s_sGame.pShots[ubStream] = multiplexedSpriteAdd(
			s_pShotChannels[ubStream], SHOOTER_BULLET_HEIGHT, ubCount);
		if(!s_sGame.pShots[ubStream]) {
			return 0;
		}
		for(UBYTE ubIndex = 0; ubIndex < ubCount; ++ubIndex) {
			multiplexedSpriteSetBitmap(s_sGame.pShots[ubStream], ubIndex,
				s_sGame.pShotArt[ubStream == 3]);
		}
	}
	return 1;
}

/** Advanced Add copies both animation frames; the input strip is disposable. */
static UBYTE createEnemies(void) {
	tBitMap *pStrip = shooterHorzArtEnemies();
	if(!pStrip) {
		return 0;
	}
	s_sGame.pEnemies = advancedMultiplexedSpriteAdd(
		4, pStrip, 0, SHOOTER_ENEMY_HEIGHT, SHOOTER_ENEMY_COUNT);
	blitWait();
	bitmapDestroy(pStrip);
	return s_sGame.pEnemies != 0;
}

/** Keyboard or joystick in port 2 (ACE JOY1).
 * The pure logic owns cooldowns and fire-edge restart. */
static tShooterInput readInput(void) {
	tShooterInput ubInput = 0;
	if(keyCheck(KEY_LEFT) || joyCheck(JOY1_LEFT)) {
		ubInput |= SHOOTER_INPUT_LEFT;
	}
	if(keyCheck(KEY_RIGHT) || joyCheck(JOY1_RIGHT)) {
		ubInput |= SHOOTER_INPUT_RIGHT;
	}
	if(keyCheck(KEY_UP) || joyCheck(JOY1_UP)) {
		ubInput |= SHOOTER_INPUT_UP;
	}
	if(keyCheck(KEY_DOWN) || joyCheck(JOY1_DOWN)) {
		ubInput |= SHOOTER_INPUT_DOWN;
	}
	if(keyCheck(KEY_SPACE) || joyCheck(JOY1_FIRE)) {
		ubInput |= SHOOTER_INPUT_FIRE;
	}
	if(keyCheck(KEY_R)) {
		ubInput |= SHOOTER_INPUT_RESTART;
	}
	return ubInput;
}

/** Run first after logic, while the previous end-of-viewport wait is fresh.
 * Unlike multiplexed streams these headers are not double buffered. */
static void renderPlayer(void) {
	const tShooterState *pState = &s_sGame.sState;
	UBYTE isVisible = !pState->ubGameOver &&
		(!pState->uwInvulnerability || !(pState->uwInvulnerability & 4));
	for(UBYTE ubHalf = 0; ubHalf < 2; ++ubHalf) {
		tSprite *pSprite = s_sGame.pPlayer[ubHalf];
		pSprite->wX = pState->wPlayerX;
		pSprite->wY = pState->wPlayerY;
		spriteRequestMetadataUpdate(pSprite);
		spriteSetEnabled(pSprite, isVisible);
		spriteProcess(pSprite);
		spriteProcessChannel(ubHalf);
	}
}

/** Red slots can remap: all borrow the same immutable 2px-high bitmap,
 * so ACE's per-slot pixel cache stays valid. Green retains owner indices. */
static void renderShots(void) {
	for(UBYTE ubStream = 0; ubStream < SHOOTER_SHOT_CHANNEL_COUNT; ++ubStream) {
		UBYTE ubChannel = s_pShotChannels[ubStream];
		const tShooterBullet *pBullets = ubStream == 3 ?
			s_sGame.sState.pEnemyBullets : s_sGame.sState.pPlayerBullets;
		UBYTE ubCount = ubStream == 3 ? SHOOTER_ENEMY_BULLET_COUNT :
			SHOOTER_PLAYER_BULLET_COUNT;
		s_sGame.pShotCounts[ubStream] = 0;
		for(UBYTE ubIndex = 0; ubIndex < ubCount; ++ubIndex) {
			const tShooterBullet *pBullet = &pBullets[ubIndex];
			UBYTE isEnabled = pBullet->ubActive &&
				pBullet->ubChannel == ubChannel;
			UBYTE ubSlot = ubStream == 3 ? ubIndex :
				s_sGame.pShotCounts[ubStream];
			if(ubStream == 3 || isEnabled) {
				multiplexedSpriteSetEnabled(s_sGame.pShots[ubStream], ubSlot,
					isEnabled);
			}
			if(isEnabled) {
				multiplexedSpriteSetPos(s_sGame.pShots[ubStream], ubSlot,
					pBullet->wX, pBullet->wY);
				++s_sGame.pShotCounts[ubStream];
			}
		}
		if(ubStream != 3) {
			for(UBYTE ubSlot = s_sGame.pShotCounts[ubStream];
				ubSlot < SHOOTER_RED_CHANNEL_CAPACITY; ++ubSlot) {
				multiplexedSpriteSetEnabled(s_sGame.pShots[ubStream], ubSlot, 0);
			}
		}
		multiplexedSpriteProcess(s_sGame.pShots[ubStream]);
		multiplexedSpriteProcessChannel(ubChannel);
	}
}

/** Death only disables a logical element; respawn reuses its allocated slot. */
static void renderEnemies(void) {
	for(UBYTE ubIndex = 0; ubIndex < SHOOTER_ENEMY_COUNT; ++ubIndex) {
		const tShooterEnemy *pEnemy = &s_sGame.sState.pEnemies[ubIndex];
		advancedMultiplexedSpriteSetEnabled(s_sGame.pEnemies, ubIndex,
			pEnemy->ubActive);
		advancedMultiplexedSpriteSetPos(s_sGame.pEnemies, ubIndex,
			pEnemy->wX, pEnemy->wY);
		advancedMultiplexedSpriteSetFrame(s_sGame.pEnemies, ubIndex,
			((s_sGame.sState.ulFrame >> 3) + ubIndex) & 1);
	}
	advancedMultiplexedSpriteProcess(s_sGame.pEnemies);
	advancedMultiplexedSpriteProcessChannel(s_sGame.pEnemies);
}

/** Cache tracks bitmap contents, not the alternating front/back role. */
static void renderHud(UBYTE ubBuffer) {
	const tShooterState *pState = &s_sGame.sState;
	tShooterHudCache *pCache = &s_sGame.pHudCaches[ubBuffer];
	UBYTE isScoreDirty = !pCache->isValid ||
		pCache->ulScore != pState->ulScore;
	UBYTE isLivesDirty = !pCache->isValid ||
		pCache->ubLives != pState->ubLives;
	UBYTE isStatusDirty = !pCache->isValid ||
		pCache->isGameOver != pState->ubGameOver;
	if(isScoreDirty || isLivesDirty || isStatusDirty) {
		blitWait();
		shooterHorzArtHud(s_sGame.pBackgrounds[ubBuffer],
			pState->ulScore, pState->ubLives, pState->ubGameOver,
			isScoreDirty, isLivesDirty, isStatusDirty);
		pCache->ulScore = pState->ulScore;
		pCache->ubLives = pState->ubLives;
		pCache->isGameOver = pState->ubGameOver;
		pCache->isValid = 1;
	}
}

/** Finish both CPU drawing passes before ACE swaps/publishes this bitmap.
 * Each fixed bitmap normally advances two lifetime ticks per reuse. */
static void renderBackground(void) {
	tBitMap *pBack = s_sGame.pBuffer->pBack;
	UBYTE ubBuffer = pBack == s_sGame.pBackgrounds[0] ? 0 : 1;
	renderHud(ubBuffer);
	blitWait();
	shooterHorzStarsRender(&s_sGame.sStars[ubBuffer], pBack,
		s_sGame.ulFrame + 1);
}

void gsExampleShooterHorzCreate(void) {
	memset(&s_sGame, 0, sizeof(s_sGame));
	shooterHorzInit(&s_sGame.sState);
	if(!createDisplay() || !createManagers() || !createPlayer() ||
		!createShots() || !createEnemies()) {
		return;
	}
	/* Both initial images are complete before raster DMA can read either. */
	for(UBYTE ubBuffer = 0; ubBuffer < 2; ++ubBuffer) {
		renderHud(ubBuffer);
	}
	blitWait();
	systemUnuse();
	s_sGame.isSystemUnused = 1;
	systemSetDmaBit(DMAB_SPRITE, 1);
	viewLoad(s_sGame.pView);
	/* Put all sprite pairs ahead of the single playfield. */
	g_pCustom->bplcon2 = 0x0024;
	s_sGame.isLoaded = 1;
	/* First loop needs the same safe header-writing window as later loops. */
	vPortWaitForEnd(s_sGame.pVPort);
}

void gsExampleShooterHorzLoop(void) {
	if(!s_sGame.isLoaded) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}
	if(keyUse(KEY_ESCAPE)) {
		stateChange(g_pGameStateManager, &g_pTestStates[TEST_STATE_MENU]);
		return;
	}
	shooterHorzStep(&s_sGame.sState, readInput());
	renderPlayer();
	renderShots();
	renderEnemies();
	renderBackground();
	blitWait();
	viewProcessManagers(s_sGame.pView);
	copProcessBlocks();
	++s_sGame.ulFrame;
	vPortWaitForEnd(s_sGame.pVPort);
}

void gsExampleShooterHorzDestroy(void) {
	blitWait();
	if(s_sGame.isLoaded) {
		systemSetDmaBit(DMAB_SPRITE, 0);
		viewLoad(0);
	}
	if(s_sGame.isSystemUnused) {
		systemUse();
	}
	if(s_sGame.pEnemies) {
		advancedMultiplexedSpriteRemove(s_sGame.pEnemies);
	}
	if(s_sGame.isManagersCreated) {
		/* Manager teardown removes ordinary shots and basic player structs. */
		multiplexedSpriteManagerDestroy();
		spriteManagerDestroy();
	}
	for(UBYTE ubIndex = 0; ubIndex < 2; ++ubIndex) {
		if(s_sGame.pShotArt[ubIndex]) {
			bitmapDestroy(s_sGame.pShotArt[ubIndex]);
		}
		if(s_sGame.pPlayerArt[ubIndex]) {
			bitmapDestroy(s_sGame.pPlayerArt[ubIndex]);
		}
	}
	if(s_sGame.pView) {
		/* The simple-buffer manager owns both background bitmaps. */
		viewDestroy(s_sGame.pView);
	}
	if(s_sGame.pBlankSprite) {
		memFree(s_sGame.pBlankSprite, sizeof(*s_sGame.pBlankSprite));
	}
	memset(&s_sGame, 0, sizeof(s_sGame));
}
