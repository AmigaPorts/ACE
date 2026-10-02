/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "examples/shooter_vertical/logic.h"
#include <string.h>

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
	pEnemy->wX = (int16_t)(40 + ubIndex * 40);
	pEnemy->wY = SHOOTER_ENEMY_FIRST_Y + ubIndex * SHOOTER_ENEMY_ROW_GAP;
	pEnemy->bDirection = (ubIndex & 1) ? -1 : 1;
	pEnemy->ubActive = 1;
	pEnemy->uwRespawn = 0;
}

void shooterVertInit(tShooterState *pState) {
	memset(pState, 0, sizeof(*pState));
	pState->wPlayerX = (SHOOTER_SCREEN_WIDTH - SHOOTER_PLAYER_WIDTH) / 2;
	pState->wPlayerY = SHOOTER_PLAYER_Y_MAX;
	pState->ubLives = SHOOTER_INITIAL_LIVES;
	pState->ubEnemyFireTimer = SHOOTER_ENEMY_FIRE_GRACE;
	for(uint8_t ubIndex = 0; ubIndex < SHOOTER_ENEMY_COUNT; ++ubIndex) {
		spawnEnemy(&pState->pEnemies[ubIndex], ubIndex);
	}
}

/** Equal channel velocities preserve this gap until a bullet is removed. */
static uint8_t hasSafeGap(
	const tShooterBullet *pBullets, uint8_t ubCount, int16_t wY
) {
	for(uint8_t ubIndex = 0; ubIndex < ubCount; ++ubIndex) {
		int16_t wGap = pBullets[ubIndex].wY - wY;
		if(pBullets[ubIndex].ubActive &&
			wGap > -SHOOTER_BULLET_MIN_GAP && wGap < SHOOTER_BULLET_MIN_GAP) {
			return 0;
		}
	}
	return 1;
}

/** Bullets leave at the screen edge; no clipped, collidable remnants. */
static void moveBullets(
	tShooterBullet *pBullets, uint8_t ubCount, int16_t wDy
) {
	for(uint8_t ubIndex = 0; ubIndex < ubCount; ++ubIndex) {
		tShooterBullet *pBullet = &pBullets[ubIndex];
		if(pBullet->ubActive) {
			pBullet->wY += wDy;
			if(pBullet->wY < 0 ||
				pBullet->wY > SHOOTER_SCREEN_HEIGHT - SHOOTER_BULLET_HEIGHT) {
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

/** Respawn ticks count down only during gameplay. */
static void moveEnemies(tShooterState *pState) {
	for(uint8_t ubIndex = 0; ubIndex < SHOOTER_ENEMY_COUNT; ++ubIndex) {
		tShooterEnemy *pEnemy = &pState->pEnemies[ubIndex];
		if(!pEnemy->ubActive) {
			if(pEnemy->uwRespawn && --pEnemy->uwRespawn == 0) {
				spawnEnemy(pEnemy, ubIndex);
			}
			continue;
		}
		pEnemy->wX += pEnemy->bDirection * SHOOTER_ENEMY_SPEED;
		if(pEnemy->wX <= SHOOTER_X_MIN || pEnemy->wX >= SHOOTER_X_MAX) {
			pEnemy->wX = clamp(pEnemy->wX, SHOOTER_X_MIN, SHOOTER_X_MAX);
			pEnemy->bDirection = (int8_t)-pEnemy->bDirection;
		}
	}
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
static void hitEnemies(tShooterState *pState) {
	for(uint8_t ubShot = 0; ubShot < SHOOTER_PLAYER_BULLET_COUNT; ++ubShot) {
		tShooterBullet *pBullet = &pState->pPlayerBullets[ubShot];
		if(!pBullet->ubActive) {
			continue;
		}
		for(uint8_t ubIndex = 0; ubIndex < SHOOTER_ENEMY_COUNT; ++ubIndex) {
			tShooterEnemy *pEnemy = &pState->pEnemies[ubIndex];
			if(pEnemy->ubActive && bulletHits(pBullet, pEnemy->wX,
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
		if(!pState->uwInvulnerability) {
			--pState->ubLives;
			++pState->ulHits;
			pState->uwInvulnerability = SHOOTER_INVULNERABILITY_FRAMES;
			if(!pState->ubLives) {
				pState->ubGameOver = 1;
				return;
			}
		}
	}
}

/** Cooldown stays at zero on rejection: there is no accumulated fire debt. */
static void firePlayer(tShooterState *pState, tShooterInput ubInput) {
	int16_t wY = pState->wPlayerY - SHOOTER_BULLET_HEIGHT;
	if(!(ubInput & SHOOTER_INPUT_FIRE) || pState->ubFireCooldown ||
		!hasSafeGap(pState->pPlayerBullets, SHOOTER_PLAYER_BULLET_COUNT, wY)) {
		return;
	}
	for(uint8_t ubIndex = 0; ubIndex < SHOOTER_PLAYER_BULLET_COUNT; ++ubIndex) {
		tShooterBullet *pBullet = &pState->pPlayerBullets[ubIndex];
		if(!pBullet->ubActive) {
			pBullet->wX = pState->wPlayerX +
				(SHOOTER_PLAYER_WIDTH - SHOOTER_BULLET_WIDTH) / 2;
			pBullet->wY = wY;
			pBullet->ubActive = 1;
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
	int16_t wY = pEnemy->wY + SHOOTER_ENEMY_HEIGHT;
	if(pState->ubEnemyFireTimer) {
		--pState->ubEnemyFireTimer;
		return;
	}
	pState->ubEnemyFireTimer = SHOOTER_ENEMY_FIRE_PERIOD - 1;
	pState->ubNextEnemy = (ubIndex + 1) % SHOOTER_ENEMY_COUNT;
	if(!pEnemy->ubActive || pBullet->ubActive ||
		!hasSafeGap(pState->pEnemyBullets, SHOOTER_ENEMY_BULLET_COUNT, wY)) {
		return;
	}
	pBullet->wX = pEnemy->wX +
		(SHOOTER_ENEMY_WIDTH - SHOOTER_BULLET_WIDTH) / 2;
	pBullet->wY = wY;
	pBullet->ubActive = 1;
	++pState->ulEnemyShots;
}

void shooterVertStep(tShooterState *pState, tShooterInput ubInput) {
	uint8_t ubFire = !!(ubInput & SHOOTER_INPUT_FIRE);
	if(pState->ubGameOver) {
		if((ubInput & SHOOTER_INPUT_RESTART) ||
			(ubFire && !pState->ubPreviousFire)) {
			shooterVertInit(pState);
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
	moveEnemies(pState);
	moveBullets(pState->pPlayerBullets, SHOOTER_PLAYER_BULLET_COUNT,
		SHOOTER_PLAYER_BULLET_DY);
	moveBullets(pState->pEnemyBullets, SHOOTER_ENEMY_BULLET_COUNT,
		SHOOTER_ENEMY_BULLET_DY);
	hitEnemies(pState);
	hitPlayer(pState);
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

uint8_t shooterVertActivePlayerBullets(const tShooterState *pState) {
	return countBullets(pState->pPlayerBullets, SHOOTER_PLAYER_BULLET_COUNT);
}

uint8_t shooterVertActiveEnemyBullets(const tShooterState *pState) {
	return countBullets(pState->pEnemyBullets, SHOOTER_ENEMY_BULLET_COUNT);
}

uint8_t shooterVertActiveEnemies(const tShooterState *pState) {
	uint8_t ubActive = 0;
	for(uint8_t ubIndex = 0; ubIndex < SHOOTER_ENEMY_COUNT; ++ubIndex) {
		ubActive += !!pState->pEnemies[ubIndex].ubActive;
	}
	return ubActive;
}
