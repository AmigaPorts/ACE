/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _SHOWCASE_EXAMPLES_SHOOTER_HORIZONTAL_LOGIC_H_
#define _SHOWCASE_EXAMPLES_SHOOTER_HORIZONTAL_LOGIC_H_
/** @file Portable, allocation-free deterministic 50 Hz shooter simulation. */
#include <stdint.h>

#define SHOOTER_HZ 50
#define SHOOTER_SCREEN_WIDTH 320
#define SHOOTER_SCREEN_HEIGHT 256
#define SHOOTER_X_MIN 16
#define SHOOTER_X_MAX 288
#define SHOOTER_PLAYER_WIDTH 16
#define SHOOTER_PLAYER_HEIGHT 16
#define SHOOTER_PLAYER_Y_MIN 32
#define SHOOTER_PLAYER_Y_MAX 232
#define SHOOTER_PLAYER_SPEED 2
#define SHOOTER_ENEMY_WIDTH 16
#define SHOOTER_ENEMY_HEIGHT 12
#define SHOOTER_ENEMY_COUNT 6
#define SHOOTER_ENEMY_FIRST_Y 34
#define SHOOTER_ENEMY_ROW_GAP 34
#define SHOOTER_ENEMY_SPEED 1
#define SHOOTER_BULLET_WIDTH 6
#define SHOOTER_BULLET_HEIGHT 2
#define SHOOTER_BULLET_MIN_GAP 3
#define SHOOTER_PLAYER_BULLET_COUNT 12
#define SHOOTER_RED_CHANNEL_CAPACITY 4
#define SHOOTER_ENEMY_BULLET_COUNT SHOOTER_ENEMY_COUNT
#define SHOOTER_PLAYER_BULLET_DX 6
#define SHOOTER_ENEMY_BULLET_DX (-3)
#define SHOOTER_PLAYER_BULLET_CHANNEL_COUNT 3
#define SHOOTER_ENEMY_BULLET_CHANNEL 3
#define SHOOTER_FIRE_COOLDOWN 4
#define SHOOTER_ENEMY_FIRE_GRACE 100
#define SHOOTER_ENEMY_FIRE_PERIOD 12
#define SHOOTER_INITIAL_LIVES 3
#define SHOOTER_INVULNERABILITY_FRAMES 50
#define SHOOTER_RESPAWN_FRAMES 120
#define SHOOTER_KILL_SCORE 100

typedef uint8_t tShooterInput;
#define SHOOTER_INPUT_LEFT    0x01u
#define SHOOTER_INPUT_RIGHT   0x02u
#define SHOOTER_INPUT_UP      0x04u
#define SHOOTER_INPUT_DOWN    0x08u
#define SHOOTER_INPUT_FIRE    0x10u
#define SHOOTER_INPUT_RESTART 0x20u

/** All coordinates are screen-space top-left pixels; rectangles are half-open.
 * Bullet art occupies WIDTH x HEIGHT at this origin (even in a 16px sprite).
 * Inactive slots must not be rendered. Slot indices are stable, not Y-sorted.
 * ubChannel is the actual hardware channel: red 2/6/7, green 3. Render each
 * Red channels each admit at most four bullets; render their active slots
 * compactly using channel-local indices, independent of logical slot indices.
 * Only bullets on the SAME channel require a >=3px top-left Y separation.
 */
typedef struct _tShooterBullet {
	int16_t wX, wY;
	uint8_t ubActive;
	uint8_t ubChannel;
} tShooterBullet;

typedef struct _tShooterEnemy {
	int16_t wX, wY;
	int8_t bDirection; /**< Retained facing metadata, always -1 (left). */
	uint8_t ubActive;
	uint16_t uwRespawn;
} tShooterEnemy;

/** Renderer reads this snapshot; only logic should mutate it in production.
 * Enemy bullet index is its owner index, independent of owner activity.
 * Counters are per run, uint32_t modulo counters. ulHits counts lives lost.
 * uwInvulnerability is remaining frames; ubGameOver implies ubLives == 0.
 */
typedef struct _tShooterState {
	int16_t wPlayerX, wPlayerY;
	tShooterEnemy pEnemies[SHOOTER_ENEMY_COUNT];
	tShooterBullet pPlayerBullets[SHOOTER_PLAYER_BULLET_COUNT];
	tShooterBullet pEnemyBullets[SHOOTER_ENEMY_BULLET_COUNT];
	uint32_t ulFrame, ulScore;
	uint32_t ulShotsFired, ulEnemyShots, ulHits, ulKills;
	uint16_t uwInvulnerability;
	uint8_t ubLives, ubGameOver;
	uint8_t ubFireCooldown, ubEnemyFireTimer, ubNextEnemy;
	uint8_t ubPreviousFire;
	uint8_t ubNextPlayerChannel; /**< Round-robin index into channels {2,6,7}. */
} tShooterState;

/** Initialize a fresh run: player (32,132), enemy X=220+28*(index%3),
 * enemy Y=34+34*index and direction=-1. Clear all bullets and counters.
 */
void shooterHorzInit(tShooterState *pState);

/** Advance exactly one 1/50-second tick with held input bits; no clock/RNG/I/O.
 * Opposing directions cancel; diagonal movement is 2px on each axis.
 * Order: timers, actors, existing bullets, red hits, green hits, body hits,
 * then new shots. Half-open AABBs; touching edges alone is not a collision.
 * Player origins clamp to X16..288/Y32..232. Enemies move left 1px/tick.
 * At X<=-16 enemies wrap to 304. Wrap/respawn ticks skip enemy collisions;
 * movement and collisions resume next tick. Fixed enemy Y never changes.
 * Wrapped/respawned enemies may fire on their scheduled turn that tick.
 * Existing red/green bullets move X by +6/-3, Y unchanged; deactivate before
 * collisions when X<0 or X>314. New red origin=(playerX+16,playerY+7),
 * green=(enemyX-6,enemyY+5); the entire 6x2 rectangle must fit the screen.
 * New bullets first move/collide next tick. Rejected shots are not counted.
 * Successful player shots are >=4 ticks (80ms) apart, with no burst fire.
 * Search channels 2/6/7 round-robin for room below the four-bullet channel
 * capacity and a >=3px Y gap, then use the first inactive global slot.
 * Rejection changes neither shot count, cooldown nor cursor.
 * Enemy attempts: ticks 101,113,...; owner advances even on rejection.
 * Dead enemies respawn 120 ticks later, retaining any existing owner bullet.
 * Respawn X=304. Body contacts recycle enemies for 120 ticks without scoring,
 * even during invulnerability. Bullet kills take priority over body contacts.
 * Hits at tick T protect through T+49; next damage is possible at T+50.
 * Game over freezes everything except fire-edge tracking. R (held) or a
 * rising fire edge resets only when already game over, consuming that tick.
 * Fire held across death cannot restart. Reset remembers current fire level.
 * Requires non-null, initialized state. Unknown input bits are ignored.
 */
void shooterHorzStep(tShooterState *pState, tShooterInput ubInput);

/** Return the number of active player bullet slots (0..12). */
uint8_t shooterHorzActivePlayerBullets(const tShooterState *pState);
/** Return the number of active enemy bullet slots (0..6). */
uint8_t shooterHorzActiveEnemyBullets(const tShooterState *pState);
/** Return the number of living enemies (0..6). */
uint8_t shooterHorzActiveEnemies(const tShooterState *pState);

#endif // _SHOWCASE_EXAMPLES_SHOOTER_HORIZONTAL_LOGIC_H_
