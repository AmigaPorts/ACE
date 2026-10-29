/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "examples/shooter_horizontal/logic.h"
#include <string.h>

static const uint8_t s_pPlayerChannels[SHOOTER_PLAYER_BULLET_CHANNEL_COUNT] = {
	2, 6, 7
};

/** Clamp actor origins, rather than sprite right/bottom edges. */
static int16_t clamp(int16_t wValue, int16_t wMin, int16_t wMax) {
	if(wValue < wMin) {
		return wMin;
	}
	if(wValue > wMax) {
		return wMax;
	}
	return wValue;
}

/** Recreate only the enemy: its bullet belongs to the persistent slot. */
static void spawnEnemy(tShooterEnemy *pEnemy, uint8_t ubIndex) {
	pEnemy->wX = SHOOTER_SCREEN_WIDTH - SHOOTER_ENEMY_WIDTH;
	pEnemy->wY = SHOOTER_ENEMY_FIRST_Y + ubIndex * SHOOTER_ENEMY_ROW_GAP;
	pEnemy->bDirection = -1;
	pEnemy->ubActive = 1;
	pEnemy->uwRespawn = 0;
}

void shooterHorzInit(tShooterState *pState) {
	memset(pState, 0, sizeof(*pState));
	pState->wPlayerX = 32;
	pState->wPlayerY = 132;
	pState->ubLives = SHOOTER_INITIAL_LIVES;
	pState->ubEnemyFireTimer = SHOOTER_ENEMY_FIRE_GRACE;
	for(uint8_t ubIndex = 0; ubIndex < SHOOTER_ENEMY_COUNT; ++ubIndex) {
		spawnEnemy(&pState->pEnemies[ubIndex], ubIndex);
		pState->pEnemies[ubIndex].wX = 220 + 28 * (ubIndex % 3);
	}
}

/** Admit only within the channel's renderer capacity and DMA separation. */
static uint8_t hasRoomAndSafeGap(
	const tShooterBullet *pBullets, uint8_t ubCount, int16_t wY,
	uint8_t ubChannel, uint8_t ubCapacity
) {
	uint8_t ubLiveCount = 0;
	for(uint8_t ubIndex = 0; ubIndex < ubCount; ++ubIndex) {
		if(!pBullets[ubIndex].ubActive ||
			pBullets[ubIndex].ubChannel != ubChannel) {
			continue;
		}
		int16_t wGap = pBullets[ubIndex].wY - wY;
		if(++ubLiveCount >= ubCapacity ||
			(wGap > -SHOOTER_BULLET_MIN_GAP && wGap < SHOOTER_BULLET_MIN_GAP)) {
			return 0;
		}
	}
	return 1;
}

/** Bullets leave at the screen edge; no clipped, collidable remnants. */
static void moveBullets(
	tShooterBullet *pBullets, uint8_t ubCount, int16_t wDx
) {
	for(uint8_t ubIndex = 0; ubIndex < ubCount; ++ubIndex) {
		tShooterBullet *pBullet = &pBullets[ubIndex];
		if(pBullet->ubActive) {
			pBullet->wX += wDx;
			if(pBullet->wX < 0 ||
				pBullet->wX > SHOOTER_SCREEN_WIDTH - SHOOTER_BULLET_WIDTH) {
				pBullet->ubActive = 0;
			}
		}
	}
}

/** Opposing held directions cancel without an input-priority bias. */
static void movePlayer(tShooterState *pState, tShooterInput ubInput) {
	int16_t wDx = !!(ubInput & SHOOTER_INPUT_RIGHT) -
		!!(ubInput & SHOOTER_INPUT_LEFT);
	int16_t wDy = !!(ubInput & SHOOTER_INPUT_DOWN) -
		!!(ubInput & SHOOTER_INPUT_UP);
	pState->wPlayerX = clamp(pState->wPlayerX + wDx * SHOOTER_PLAYER_SPEED,
		SHOOTER_X_MIN, SHOOTER_X_MAX);
	pState->wPlayerY = clamp(pState->wPlayerY + wDy * SHOOTER_PLAYER_SPEED,
		SHOOTER_PLAYER_Y_MIN, SHOOTER_PLAYER_Y_MAX);
}

/** Return a bitmask of teleported enemies to exclude from this tick's hits. */
static uint8_t moveEnemies(tShooterState *pState) {
	uint8_t ubTeleported = 0;
	for(uint8_t ubIndex = 0; ubIndex < SHOOTER_ENEMY_COUNT; ++ubIndex) {
		tShooterEnemy *pEnemy = &pState->pEnemies[ubIndex];
		if(!pEnemy->ubActive) {
			if(pEnemy->uwRespawn && --pEnemy->uwRespawn == 0) {
				spawnEnemy(pEnemy, ubIndex);
				ubTeleported |= (uint8_t)(1u << ubIndex);
			}
			continue;
		}
		pEnemy->wX -= SHOOTER_ENEMY_SPEED;
		if(pEnemy->wX <= -SHOOTER_ENEMY_WIDTH) {
			spawnEnemy(pEnemy, ubIndex);
			ubTeleported |= (uint8_t)(1u << ubIndex);
		}
	}
	return ubTeleported;
}

/** Half-open AABBs: merely touching edges does not count as a hit. */
static uint8_t bulletHits(
	const tShooterBullet *pBullet, int16_t wX, int16_t wY,
	int16_t wWidth, int16_t wHeight
) {
	return pBullet->wX < wX + wWidth &&
		pBullet->wX + SHOOTER_BULLET_WIDTH > wX &&
		pBullet->wY < wY + wHeight &&
		pBullet->wY + SHOOTER_BULLET_HEIGHT > wY;
}

/** Deactivating immediately prevents double scoring within the same tick. */
static void hitEnemies(tShooterState *pState, uint8_t ubTeleported) {
	for(uint8_t ubShot = 0; ubShot < SHOOTER_PLAYER_BULLET_COUNT; ++ubShot) {
		tShooterBullet *pBullet = &pState->pPlayerBullets[ubShot];
		if(!pBullet->ubActive) {
			continue;
		}
		for(uint8_t ubIndex = 0; ubIndex < SHOOTER_ENEMY_COUNT; ++ubIndex) {
			tShooterEnemy *pEnemy = &pState->pEnemies[ubIndex];
			if(pEnemy->ubActive && !(ubTeleported & (1u << ubIndex)) &&
				bulletHits(pBullet, pEnemy->wX,
				pEnemy->wY, SHOOTER_ENEMY_WIDTH, SHOOTER_ENEMY_HEIGHT)) {
				pBullet->ubActive = 0;
				pEnemy->ubActive = 0;
				pEnemy->uwRespawn = SHOOTER_RESPAWN_FRAMES;
				++pState->ulKills;
				pState->ulScore += SHOOTER_KILL_SCORE;
				break;
			}
		}
	}
}

/** All damage sources share one invulnerability window. */
static void damagePlayer(tShooterState *pState) {
	if(pState->uwInvulnerability || !pState->ubLives) {
		return;
	}
	--pState->ubLives;
	++pState->ulHits;
	pState->uwInvulnerability = SHOOTER_INVULNERABILITY_FRAMES;
	if(!pState->ubLives) {
		pState->ubGameOver = 1;
	}
}

/** Invulnerable contacts consume bullets but never consume another life. */
static void hitPlayer(tShooterState *pState) {
	for(uint8_t ubIndex = 0; ubIndex < SHOOTER_ENEMY_BULLET_COUNT; ++ubIndex) {
		tShooterBullet *pBullet = &pState->pEnemyBullets[ubIndex];
		if(!pBullet->ubActive || !bulletHits(pBullet,
			pState->wPlayerX, pState->wPlayerY,
			SHOOTER_PLAYER_WIDTH, SHOOTER_PLAYER_HEIGHT)) {
			continue;
		}
		pBullet->ubActive = 0;
		damagePlayer(pState);
	}
}

/** Recycle every rammed enemy, including contacts while invulnerable. */
static void hitBodies(tShooterState *pState, uint8_t ubTeleported) {
	for(uint8_t ubIndex = 0; ubIndex < SHOOTER_ENEMY_COUNT; ++ubIndex) {
		tShooterEnemy *pEnemy = &pState->pEnemies[ubIndex];
		if(pEnemy->ubActive && !(ubTeleported & (1u << ubIndex)) &&
			pEnemy->wX < pState->wPlayerX + SHOOTER_PLAYER_WIDTH &&
			pEnemy->wX + SHOOTER_ENEMY_WIDTH > pState->wPlayerX &&
			pEnemy->wY < pState->wPlayerY + SHOOTER_PLAYER_HEIGHT &&
			pEnemy->wY + SHOOTER_ENEMY_HEIGHT > pState->wPlayerY) {
			pEnemy->ubActive = 0;
			pEnemy->uwRespawn = SHOOTER_RESPAWN_FRAMES;
			damagePlayer(pState);
		}
	}
}

/** Reject partially off-screen spawns before assigning any slot or cooldown. */
static uint8_t bulletFits(int16_t wX, int16_t wY) {
	return wX >= 0 && wX <= SHOOTER_SCREEN_WIDTH - SHOOTER_BULLET_WIDTH &&
		wY >= 0 && wY <= SHOOTER_SCREEN_HEIGHT - SHOOTER_BULLET_HEIGHT;
}

/** Return a round-robin channel index, or CHANNEL_COUNT when all are busy. */
static uint8_t findPlayerChannel(const tShooterState *pState, int16_t wY) {
	for(uint8_t ubOffset = 0;
		ubOffset < SHOOTER_PLAYER_BULLET_CHANNEL_COUNT; ++ubOffset) {
		uint8_t ubIndex = (pState->ubNextPlayerChannel + ubOffset) %
			SHOOTER_PLAYER_BULLET_CHANNEL_COUNT;
		if(hasRoomAndSafeGap(pState->pPlayerBullets, SHOOTER_PLAYER_BULLET_COUNT,
			wY, s_pPlayerChannels[ubIndex], SHOOTER_RED_CHANNEL_CAPACITY)) {
			return ubIndex;
		}
	}
	return SHOOTER_PLAYER_BULLET_CHANNEL_COUNT;
}

/** Cooldown stays at zero on rejection: there is no accumulated fire debt. */
static void firePlayer(tShooterState *pState, tShooterInput ubInput) {
	int16_t wX = pState->wPlayerX + SHOOTER_PLAYER_WIDTH;
	int16_t wY = pState->wPlayerY + 7;
	if(!(ubInput & SHOOTER_INPUT_FIRE) || pState->ubFireCooldown ||
		!bulletFits(wX, wY)) {
		return;
	}
	uint8_t ubChannelIndex = findPlayerChannel(pState, wY);
	if(ubChannelIndex == SHOOTER_PLAYER_BULLET_CHANNEL_COUNT) {
		return;
	}
	for(uint8_t ubIndex = 0; ubIndex < SHOOTER_PLAYER_BULLET_COUNT; ++ubIndex) {
		tShooterBullet *pBullet = &pState->pPlayerBullets[ubIndex];
		if(!pBullet->ubActive) {
			pBullet->wX = wX;
			pBullet->wY = wY;
			pBullet->ubActive = 1;
			pBullet->ubChannel = s_pPlayerChannels[ubChannelIndex];
			pState->ubNextPlayerChannel = (ubChannelIndex + 1) %
				SHOOTER_PLAYER_BULLET_CHANNEL_COUNT;
			pState->ubFireCooldown = SHOOTER_FIRE_COOLDOWN;
			++pState->ulShotsFired;
			return;
		}
	}
}

/** Attempt exactly one owner, never scan ahead when its slot is occupied. */
static void fireEnemy(tShooterState *pState) {
	uint8_t ubIndex = pState->ubNextEnemy;
	tShooterEnemy *pEnemy = &pState->pEnemies[ubIndex];
	tShooterBullet *pBullet = &pState->pEnemyBullets[ubIndex];
	int16_t wX = pEnemy->wX - SHOOTER_BULLET_WIDTH;
	int16_t wY = pEnemy->wY + 5;
	if(pState->ubEnemyFireTimer) {
		--pState->ubEnemyFireTimer;
		return;
	}
	pState->ubEnemyFireTimer = SHOOTER_ENEMY_FIRE_PERIOD - 1;
	pState->ubNextEnemy = (ubIndex + 1) % SHOOTER_ENEMY_COUNT;
	if(!pEnemy->ubActive || pBullet->ubActive || !bulletFits(wX, wY) ||
		!hasRoomAndSafeGap(pState->pEnemyBullets, SHOOTER_ENEMY_BULLET_COUNT,
			wY, SHOOTER_ENEMY_BULLET_CHANNEL, SHOOTER_ENEMY_BULLET_COUNT)) {
		return;
	}
	pBullet->wX = wX;
	pBullet->wY = wY;
	pBullet->ubActive = 1;
	pBullet->ubChannel = SHOOTER_ENEMY_BULLET_CHANNEL;
	++pState->ulEnemyShots;
}

void shooterHorzStep(tShooterState *pState, tShooterInput ubInput) {
	uint8_t ubFire = !!(ubInput & SHOOTER_INPUT_FIRE);
	if(pState->ubGameOver) {
		if((ubInput & SHOOTER_INPUT_RESTART) ||
			(ubFire && !pState->ubPreviousFire)) {
			shooterHorzInit(pState);
		}
		pState->ubPreviousFire = ubFire;
		return;
	}
	pState->ubPreviousFire = ubFire;
	++pState->ulFrame;
	if(pState->ubFireCooldown) {
		--pState->ubFireCooldown;
	}
	if(pState->uwInvulnerability) {
		--pState->uwInvulnerability;
	}
	movePlayer(pState, ubInput);
	uint8_t ubTeleported = moveEnemies(pState);
	moveBullets(pState->pPlayerBullets, SHOOTER_PLAYER_BULLET_COUNT,
		SHOOTER_PLAYER_BULLET_DX);
	moveBullets(pState->pEnemyBullets, SHOOTER_ENEMY_BULLET_COUNT,
		SHOOTER_ENEMY_BULLET_DX);
	hitEnemies(pState, ubTeleported);
	hitPlayer(pState);
	hitBodies(pState, ubTeleported);
	if(!pState->ubGameOver) {
		firePlayer(pState, ubInput);
		fireEnemy(pState);
	}
}

/** Count only occupied slots; inactive coordinates have no meaning. */
static uint8_t countBullets(const tShooterBullet *pBullets, uint8_t ubCount) {
	uint8_t ubActive = 0;
	for(uint8_t ubIndex = 0; ubIndex < ubCount; ++ubIndex) {
		ubActive += !!pBullets[ubIndex].ubActive;
	}
	return ubActive;
}

uint8_t shooterHorzActivePlayerBullets(const tShooterState *pState) {
	return countBullets(pState->pPlayerBullets, SHOOTER_PLAYER_BULLET_COUNT);
}

uint8_t shooterHorzActiveEnemyBullets(const tShooterState *pState) {
	return countBullets(pState->pEnemyBullets, SHOOTER_ENEMY_BULLET_COUNT);
}

uint8_t shooterHorzActiveEnemies(const tShooterState *pState) {
	uint8_t ubActive = 0;
	for(uint8_t ubIndex = 0; ubIndex < SHOOTER_ENEMY_COUNT; ++ubIndex) {
		ubActive += !!pState->pEnemies[ubIndex].ubActive;
	}
	return ubActive;
}
