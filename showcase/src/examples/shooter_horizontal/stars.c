/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "examples/shooter_horizontal/stars.h"

/** Rebuild the pixel cache for initialization or an arbitrary phase jump. */
static void starCachePixel(tShooterStar *pStar) {
	pStar->ulByteOffset = pStar->ulRowOffset + (pStar->uwX >> 3);
	pStar->ubMask = 0x80 >> (pStar->uwX & 7);
}

/** Move left one pixel, touching the byte offset only at byte boundaries. */
static void starStepPixel(tShooterStar *pStar) {
	if(!pStar->uwX) {
		pStar->uwX = SHOOTER_STARS_WIDTH - 1;
		pStar->ulByteOffset += SHOOTER_STARS_WIDTH / 8 - 1;
		pStar->ubMask = 1;
	}
	else {
		--pStar->uwX;
		if(pStar->ubMask == 0x80) {
			--pStar->ulByteOffset;
			pStar->ubMask = 1;
		}
		else {
			pStar->ubMask <<= 1;
		}
	}
}

void shooterHorzStarsInit(tShooterStars *pStars, tBitMap *pBitmap) {
	ULONG ulRandom = SHOOTER_STARS_SEED;
	pStars->ulLastFrame = 0;
	for(UBYTE ubLayer = 0; ubLayer < SHOOTER_STARS_LAYERS; ++ubLayer) {
		pStars->pOffsets[ubLayer] = 0;
		for(UBYTE ubIndex = 0; ubIndex < SHOOTER_STARS_COUNT; ++ubIndex) {
			tShooterStar *pStar = &pStars->pStars[ubLayer][ubIndex];
			ulRandom = ulRandom * 1664525UL + 1013904223UL;
			pStar->uwInitialX = (ulRandom >> 16) % SHOOTER_STARS_WIDTH;
			pStar->uwX = pStar->uwInitialX;
			ulRandom = ulRandom * 1664525UL + 1013904223UL;
			UWORD uwY = SHOOTER_STARS_Y_MIN +
				((ulRandom >> 16) % SHOOTER_STARS_Y_COUNT);
			pStar->ulRowOffset = (ULONG)uwY * pBitmap->BytesPerRow;
			starCachePixel(pStar);
			((UBYTE *)pBitmap->Planes[ubLayer])[pStar->ulByteOffset] |=
				pStar->ubMask;
		}
	}
}

void shooterHorzStarsRender(
	tShooterStars *pStars, tBitMap *pBitmap, ULONG ulLifetimeFrame
) {
	UWORD pOffsets[SHOOTER_STARS_LAYERS];
	UBYTE pSteps[SHOOTER_STARS_LAYERS];
	UBYTE ubMoved = 0;
	if(ulLifetimeFrame == pStars->ulLastFrame) {
		return;
	}
	/* Each double-buffered bitmap advances twice; rollover is not a step. */
	UBYTE ubSequential = ulLifetimeFrame > pStars->ulLastFrame &&
		ulLifetimeFrame - pStars->ulLastFrame <= 2UL;
	for(UBYTE ubLayer = 0; ubLayer < SHOOTER_STARS_LAYERS; ++ubLayer) {
		if(ubSequential) {
			pSteps[ubLayer] = (ulLifetimeFrame >> (2 - ubLayer)) -
				(pStars->ulLastFrame >> (2 - ubLayer));
			pOffsets[ubLayer] = pStars->pOffsets[ubLayer] + pSteps[ubLayer];
			if(pOffsets[ubLayer] >= SHOOTER_STARS_WIDTH) {
				pOffsets[ubLayer] -= SHOOTER_STARS_WIDTH;
			}
		}
		else {
			pOffsets[ubLayer] =
				(ulLifetimeFrame >> (2 - ubLayer)) % SHOOTER_STARS_WIDTH;
		}
		if(pOffsets[ubLayer] == pStars->pOffsets[ubLayer]) {
			continue;
		}
		ubMoved |= 1 << ubLayer;
		UBYTE *pPlane = (UBYTE *)pBitmap->Planes[ubLayer];
		for(UBYTE ubIndex = 0; ubIndex < SHOOTER_STARS_COUNT; ++ubIndex) {
			tShooterStar *pStar = &pStars->pStars[ubLayer][ubIndex];
			pPlane[pStar->ulByteOffset] &= (UBYTE)~pStar->ubMask;
		}
	}
	for(UBYTE ubLayer = 0; ubLayer < SHOOTER_STARS_LAYERS; ++ubLayer) {
		if(!(ubMoved & (1 << ubLayer))) {
			continue;
		}
		pStars->pOffsets[ubLayer] = pOffsets[ubLayer];
		UBYTE *pPlane = (UBYTE *)pBitmap->Planes[ubLayer];
		for(UBYTE ubIndex = 0; ubIndex < SHOOTER_STARS_COUNT; ++ubIndex) {
			tShooterStar *pStar = &pStars->pStars[ubLayer][ubIndex];
			if(ubSequential) {
				starStepPixel(pStar);
				if(pSteps[ubLayer] == 2) {
					starStepPixel(pStar);
				}
			}
			else {
				WORD wX = pStar->uwInitialX - pOffsets[ubLayer];
				pStar->uwX = wX < 0 ? wX + SHOOTER_STARS_WIDTH : wX;
				starCachePixel(pStar);
			}
			pPlane[pStar->ulByteOffset] |= pStar->ubMask;
		}
	}
	pStars->ulLastFrame = ulLifetimeFrame;
}
