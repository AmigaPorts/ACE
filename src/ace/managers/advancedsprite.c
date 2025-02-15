/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/advancedsprite.h>
#include <ace/utils/bitmap.h>
#include <ace/managers/blit.h>
#include <ace/utils/sprite.h>
#include <ace/managers/system.h>


#define SPRITE_WIDTH 16
#define TWOBPP_BYTEWIDTH 2

void advancedSpriteManagerCreate(const tView *pView, UWORD uwRawCopPos) {
    spriteManagerCreate(pView, uwRawCopPos);
}

void advancedSpriteManagerDestroy(void) {
    // TODO: Should do the job, check in testing.
    spriteManagerDestroy();
}

tAdvancedSprite *advancedSpriteAdd(UBYTE ubChannelIndex, tBitMap *pSpriteVerticalStripBitmap, UWORD uwSpriteHeight) {
	tAdvancedSprite *pAdvancedSprite = memAllocFastClear(sizeof(*pAdvancedSprite));
	pAdvancedSprite->ubChannelIndex = ubChannelIndex;
	pAdvancedSprite->isEnabled = 1;
    pAdvancedSprite->uwHeight = uwSpriteHeight;


    if(!(pSpriteVerticalStripBitmap->Flags & BMF_INTERLEAVED) || (pSpriteVerticalStripBitmap->Depth != 2  && (pSpriteVerticalStripBitmap->Depth != 4 || (pAdvancedSprite->ubChannelIndex & 1) == 1))) {
		logWrite(
			"ERR: Sprite channel %hhu bitmap %p isn't interleaved 2BPP(for any channel) or 4BPP(for an even channel)\n",
			pAdvancedSprite->ubChannelIndex , pSpriteVerticalStripBitmap
		);
		//return ;
	}

    pAdvancedSprite->ubByteWidth = bitmapGetByteWidth(pSpriteVerticalStripBitmap);
	if(pAdvancedSprite->ubByteWidth != 2 || pAdvancedSprite->ubByteWidth != 4) {
		logWrite(
			"ERR: Unsupported sprite width: %hhu, expected 16 or 32\n",
			pAdvancedSprite->ubByteWidth * 8
		);
		//return;
	}

    pAdvancedSprite->uwAnimCount = pSpriteVerticalStripBitmap->Rows / uwSpriteHeight;

    if(pSpriteVerticalStripBitmap->Depth == 4) {
        pAdvancedSprite->is4PP = 1;
    } else {
        pAdvancedSprite->is4PP = 0;
    }

    if(pAdvancedSprite->ubByteWidth == 2) {
        if(pSpriteVerticalStripBitmap->Depth == 2) {
            pAdvancedSprite->ubSpriteCount=1;
        } else if (pSpriteVerticalStripBitmap->Depth == 4) {
            pAdvancedSprite->ubSpriteCount=2;
        }
    }
    if(pAdvancedSprite->ubByteWidth == 4) {
        if(pSpriteVerticalStripBitmap->Depth == 2) {
            pAdvancedSprite->ubSpriteCount=2;
        } else if (pSpriteVerticalStripBitmap->Depth == 4) {
            pAdvancedSprite->ubSpriteCount=4;
        }
    }

    unsigned int nbBitmap=pAdvancedSprite->uwAnimCount*pAdvancedSprite->ubSpriteCount*(1+pAdvancedSprite->is4PP);

    pAdvancedSprite->pAnimBitmap= (tBitMap **)memAllocFastClear(nbBitmap*sizeof(tBitMap));

    unsigned int k = 0;
    for (unsigned int i = 0; i < nbBitmap;) {
        // One time if 16 pixel wide, Two times if 32 pixel wide
        for(unsigned j = 0; j < pAdvancedSprite->ubByteWidth/2; j++) {
            if (pAdvancedSprite->is4PP) {
                // Convert the 4bpp bitmap to 2bpp.
                tBitMap *tmpBitmap =bitmapCreate(
                    SPRITE_WIDTH, pAdvancedSprite->uwHeight,
                    4, BMF_CLEAR | BMF_INTERLEAVED
                );;
                blitCopy(
                    pSpriteVerticalStripBitmap, 0+j*SPRITE_WIDTH, k * pAdvancedSprite->uwHeight,
                    tmpBitmap,
                    0,0, // first line will be for sprite control data
                    SPRITE_WIDTH, pAdvancedSprite->uwHeight,
                    MINTERM_COOKIE
                );

                tBitMap *pointers_low;
		        tBitMap *pointers_high;

                pointers_high = bitmapCreate(
                    TWOBPP_BYTEWIDTH*8, pAdvancedSprite->uwHeight,
                    2, BMF_CLEAR | BMF_INTERLEAVED);

                pointers_low = bitmapCreate(
                    TWOBPP_BYTEWIDTH*8, pAdvancedSprite->uwHeight,
                    2, BMF_CLEAR | BMF_INTERLEAVED);
                
                
                for (UWORD r = 0; r < tmpBitmap->Rows; r++)
                {
                    UWORD offetSrc = r * tmpBitmap->BytesPerRow;
                    UWORD offetDst = r * pointers_low->BytesPerRow;
                    memcpy(pointers_low->Planes[0] + offetDst, tmpBitmap->Planes[0] + offetSrc, TWOBPP_BYTEWIDTH);
                    memcpy(pointers_low->Planes[1] + offetDst, tmpBitmap->Planes[1] + offetSrc, TWOBPP_BYTEWIDTH);
                    memcpy(pointers_high->Planes[0] + offetDst, tmpBitmap->Planes[2] + offetSrc, TWOBPP_BYTEWIDTH);
                    memcpy(pointers_high->Planes[1] + offetDst, tmpBitmap->Planes[3] + offetSrc, TWOBPP_BYTEWIDTH);
                }
                // Init +2 on height for sprite management data
                pAdvancedSprite->pAnimBitmap[i] = bitmapCreate(
                    SPRITE_WIDTH, pAdvancedSprite->uwHeight+2,
                    2, BMF_CLEAR | BMF_INTERLEAVED
                );
                blitCopy(
                    pointers_low, 0, 0,
                    pAdvancedSprite->pAnimBitmap[i],
                    0,1, // first line will be for sprite control data
                    SPRITE_WIDTH, pAdvancedSprite->uwHeight,
                    MINTERM_COOKIE
                );
                i++;
                // Init +2 on height for sprite management data
                pAdvancedSprite->pAnimBitmap[i] = bitmapCreate(
                    SPRITE_WIDTH, pAdvancedSprite->uwHeight+2,
                    2, BMF_CLEAR | BMF_INTERLEAVED
                );
                blitCopy(
                    pointers_high, 0, 0,
                    pAdvancedSprite->pAnimBitmap[i],
                    0,1, // first line will be for sprite control data
                    SPRITE_WIDTH, pAdvancedSprite->uwHeight,
                    MINTERM_COOKIE
                );
                i++;
                bitmapDestroy(pointers_low);
                bitmapDestroy(pointers_high);
                bitmapDestroy(tmpBitmap);
            } else {
                // Init +2 on height for sprite management data
                pAdvancedSprite->pAnimBitmap[i] = bitmapCreate(
                    SPRITE_WIDTH, pAdvancedSprite->uwHeight+2,
                    pSpriteVerticalStripBitmap->Depth, BMF_CLEAR | BMF_INTERLEAVED
                );
                // Copy bitmap
                blitCopy(
                    pSpriteVerticalStripBitmap, 0+j*SPRITE_WIDTH, k * pAdvancedSprite->uwHeight,
                    pAdvancedSprite->pAnimBitmap[i],
                    0,1, // first line will be for sprite control data
                    SPRITE_WIDTH, pAdvancedSprite->uwHeight,
                    MINTERM_COOKIE
                ); 
                i++;
            }
        }
        k++;
    }

    pAdvancedSprite->pSprites = (tSprite **)memAllocFastClear(sizeof(tSprite*) * pAdvancedSprite->ubSpriteCount);

    for (int i = 0; i < pAdvancedSprite->ubSpriteCount; i++) {
        if (pAdvancedSprite->is4PP) {
            // 2 channels for 4bpp sprites
            pAdvancedSprite->pSprites[i] = spriteAdd(ubChannelIndex+i, pAdvancedSprite->pAnimBitmap[i]);
            i++;
            //attached sprite
            pAdvancedSprite->pSprites[i] = spriteAdd(ubChannelIndex+i, pAdvancedSprite->pAnimBitmap[i]);
            spriteSetAttached(pAdvancedSprite->pSprites[i],1);
        } else {
            pAdvancedSprite->pSprites[i] = spriteAdd(ubChannelIndex+i, pAdvancedSprite->pAnimBitmap[i]);
        }      
    }
	return pAdvancedSprite;
}

void advancedSpriteRemove(tAdvancedSprite *pAdvancedSprite) {
	systemUse();
    for (int i = 0; i < pAdvancedSprite->ubSpriteCount; i++) {
        spriteRemove(pAdvancedSprite->pSprites[i]);
    }
    for (int i = 0; i < pAdvancedSprite->uwAnimCount; i++) {
        bitmapDestroy(pAdvancedSprite->pAnimBitmap[i]);
    }
	memFree(pAdvancedSprite, sizeof(*pAdvancedSprite));
	systemUnuse();
}

void advancedSpriteSetEnabled(tAdvancedSprite *pAdvancedSprite, UBYTE isEnabled) {
	for (int i = 0; i < pAdvancedSprite->ubSpriteCount; i++) {
        spriteSetEnabled(pAdvancedSprite->pSprites[i], isEnabled);
    }
}

void advancedSpriteSetPos(tAdvancedSprite *pAdvancedSprite,WORD wX, WORD wY) {
	pAdvancedSprite->wX = wX;
	pAdvancedSprite->wY = wY;
	pAdvancedSprite->isHeaderToBeUpdated = 1;
}

void advancedSpriteSetPosX(tAdvancedSprite *pAdvancedSprite,WORD wX) {
	pAdvancedSprite->wX = wX;
	pAdvancedSprite->isHeaderToBeUpdated = 1;
}

void advancedSpriteSetPosY(tAdvancedSprite *pAdvancedSprite, WORD wY) {
	pAdvancedSprite->wY = wY;
	pAdvancedSprite->isHeaderToBeUpdated = 1;
}

void advancedSpriteSetFrame(tAdvancedSprite *pAdvancedSprite, UWORD animFrame) {
    if (animFrame >= pAdvancedSprite->uwAnimCount) {
        logWrite("ERR: Invalid animation index %hu\n", animFrame);
        return;
    }
    pAdvancedSprite->uwAnimFrame=animFrame;
    UWORD animIndex = animFrame << ((pAdvancedSprite->ubByteWidth == 4) + pAdvancedSprite->is4PP);
    for (int i = 0; i < pAdvancedSprite->ubSpriteCount; i++) {
        spriteSetBitmap(pAdvancedSprite->pSprites[i], pAdvancedSprite->pAnimBitmap[animIndex+i]);
        pAdvancedSprite->isHeaderToBeUpdated = 1; // To force header rewrite
    }    
}

void advancedSpriteProcessChannel(UBYTE ubChannelIndex, tAdvancedSprite *pAdvancedSprite) {
	for (unsigned int i = 0; i < pAdvancedSprite->ubSpriteCount; i++) {
        spriteProcessChannel(ubChannelIndex+i);
    }
}

void advancedSpriteProcess(tAdvancedSprite *pAdvancedSprite) {
    if(!pAdvancedSprite->isHeaderToBeUpdated) {
		return;
	}
    for (int i = 0; i < pAdvancedSprite->ubSpriteCount; i++) {
        if (( pAdvancedSprite->ubByteWidth==4) && (( (i>1) && (pAdvancedSprite->is4PP== 1) ) || ( (i==1) && (pAdvancedSprite->is4PP==0) )))
        {
            pAdvancedSprite->pSprites[i]->wX = pAdvancedSprite->wX+SPRITE_WIDTH;;
        } else {
            pAdvancedSprite->pSprites[i]->wX = pAdvancedSprite->wX;
        }
        pAdvancedSprite->pSprites[i]->wY = pAdvancedSprite->wY;

        pAdvancedSprite->pSprites[i]->isEnabled = pAdvancedSprite->isEnabled;

        pAdvancedSprite->pSprites[i]->isHeaderToBeUpdated = 1;

        spriteProcess(pAdvancedSprite->pSprites[i]);
    }
    pAdvancedSprite->isHeaderToBeUpdated=0;
}
