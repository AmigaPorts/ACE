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

tAdvancedSprite *advancedSpriteAdd(UBYTE ubChannelIndex, UWORD uwSpriteHeight,tBitMap *pSpriteVerticalStripBitmap1, tBitMap *pSpriteVerticalStripBitmap2 ) {
    tAdvancedSprite *pAdvancedSprite = memAllocFastClear(sizeof(*pAdvancedSprite));
    pAdvancedSprite->ubChannelIndex = ubChannelIndex;
    pAdvancedSprite->isEnabled = 1;
    pAdvancedSprite->uwHeight = uwSpriteHeight;


    if(!(pSpriteVerticalStripBitmap1->Flags & BMF_INTERLEAVED) || (pSpriteVerticalStripBitmap1->Depth != 2  && (pSpriteVerticalStripBitmap1->Depth != 4 || (pAdvancedSprite->ubChannelIndex & 1) == 1))) {
        logWrite(
            "ERR: Sprite channel %hhu bitmap %p isn't interleaved 2BPP(for any channel) or 4BPP(for an even channel)\n",
            pAdvancedSprite->ubChannelIndex , pSpriteVerticalStripBitmap1
        );
        //return ;
    }

    pAdvancedSprite->ubByteWidth = bitmapGetByteWidth(pSpriteVerticalStripBitmap1);
    if(pAdvancedSprite->ubByteWidth != 2 && pAdvancedSprite->ubByteWidth != 4) {
        logWrite(
            "ERR: Unsupported sprite width: %hhu, expected 16 or 32\n",
            pAdvancedSprite->ubByteWidth * 8
        );
        //return;
    }

    UWORD bStripe1NbAnim = pSpriteVerticalStripBitmap1->Rows / uwSpriteHeight;

    UWORD bStripe2NbAnim = 0;
    if (pSpriteVerticalStripBitmap2 != NULL) {
        bStripe2NbAnim = pSpriteVerticalStripBitmap2->Rows / uwSpriteHeight;
    }

    pAdvancedSprite->uwAnimCount = bStripe1NbAnim + bStripe2NbAnim;

    if(pSpriteVerticalStripBitmap1->Depth == 4) {
        pAdvancedSprite->is4PP = 1;
    } else {
        pAdvancedSprite->is4PP = 0;
    }

    if(pAdvancedSprite->ubByteWidth == 2) {
        if(pSpriteVerticalStripBitmap1->Depth == 2) {
            pAdvancedSprite->ubSpriteCount=1;
        } else if (pSpriteVerticalStripBitmap1->Depth == 4) {
            pAdvancedSprite->ubSpriteCount=2;
        }
    }
    if(pAdvancedSprite->ubByteWidth == 4) {
        if(pSpriteVerticalStripBitmap1->Depth == 2) {
            pAdvancedSprite->ubSpriteCount=2;
        } else if (pSpriteVerticalStripBitmap1->Depth == 4) {
            pAdvancedSprite->ubSpriteCount=4;
        }
    }

    UWORD nbBitmap=pAdvancedSprite->uwAnimCount*pAdvancedSprite->ubSpriteCount;

    UWORD nbBitmapLimit1=bStripe1NbAnim*pAdvancedSprite->ubSpriteCount;

    pAdvancedSprite->pAnimBitmap= (tBitMap **)memAllocFastClear(nbBitmap*sizeof(tBitMap));

    
    tBitMap *tmpBitmap = bitmapCreate(
        SPRITE_WIDTH, pAdvancedSprite->uwHeight,
        4, BMF_CLEAR | BMF_INTERLEAVED
    );
    tBitMap *pointers_low;
    tBitMap *pointers_high;

    pointers_high = bitmapCreate(
        TWOBPP_BYTEWIDTH*8, pAdvancedSprite->uwHeight,
        2, BMF_CLEAR | BMF_INTERLEAVED);

    pointers_low = bitmapCreate(
        TWOBPP_BYTEWIDTH*8, pAdvancedSprite->uwHeight,
        2, BMF_CLEAR | BMF_INTERLEAVED);

    tBitMap *pSpriteVerticalStripBitmap;
    pSpriteVerticalStripBitmap = pSpriteVerticalStripBitmap1;    
    UWORD k = 0;
    for (UWORD i = 0; i < nbBitmap;) {
        if (i == nbBitmapLimit1) {
            k=0;
            pSpriteVerticalStripBitmap = pSpriteVerticalStripBitmap2;
        }
        // One time if 16 pixel wide, Two times if 32 pixel wide
        for(unsigned j = 0; j < pAdvancedSprite->ubByteWidth/2; j++) {
            if (pAdvancedSprite->is4PP) {
                // Convert the 4bpp bitmap to 2bpp.
                blitCopy(
                    pSpriteVerticalStripBitmap, 0+j*SPRITE_WIDTH, k * pAdvancedSprite->uwHeight,
                    tmpBitmap,
                    0,0, // first line will be for sprite control data
                    SPRITE_WIDTH, pAdvancedSprite->uwHeight,
                    MINTERM_COOKIE
                );
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
    bitmapDestroy(pointers_low);
    bitmapDestroy(pointers_high);
    bitmapDestroy(tmpBitmap);

    pAdvancedSprite->pSprites = (tSprite **)memAllocFastClear(sizeof(tSprite*) * pAdvancedSprite->ubSpriteCount);

    for (UWORD i = 0; i < pAdvancedSprite->ubSpriteCount; i++) {
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
    for (UWORD i = 0; i < pAdvancedSprite->ubSpriteCount; i++) {
        spriteRemove(pAdvancedSprite->pSprites[i]);
    }
    for (UWORD i = 0; i < pAdvancedSprite->uwAnimCount; i++) {
        bitmapDestroy(pAdvancedSprite->pAnimBitmap[i]);
    }
    memFree(pAdvancedSprite, sizeof(*pAdvancedSprite));
    systemUnuse();
}

void advancedSpriteSetEnabled(tAdvancedSprite *pAdvancedSprite, UBYTE isEnabled) {
    for (UWORD i = 0; i < pAdvancedSprite->ubSpriteCount; i++) {
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
    for (UWORD i = 0; i < pAdvancedSprite->ubSpriteCount; i++) {
        spriteSetBitmap(pAdvancedSprite->pSprites[i], pAdvancedSprite->pAnimBitmap[animIndex+i]);
        pAdvancedSprite->isHeaderToBeUpdated = 1; // To force header rewrite
    }    
}

void advancedSpriteProcessChannel(tAdvancedSprite *pAdvancedSprite) {
    for (UWORD i = 0; i < pAdvancedSprite->ubSpriteCount; i++) {
        spriteProcessChannel(pAdvancedSprite->ubChannelIndex + i);
    }
}


UWORD addAttachedX(tAdvancedSprite *pAdvancedSprite, UBYTE spriteindex) {
    if (( pAdvancedSprite->ubByteWidth==4) && (( (spriteindex>1) && (pAdvancedSprite->is4PP== 1) ) || ( (spriteindex==1) && (pAdvancedSprite->is4PP==0) )))
    {
        return SPRITE_WIDTH;
    }
    return 0;
}

void advancedSpriteProcess(tAdvancedSprite *pAdvancedSprite) {
    if(!pAdvancedSprite->isHeaderToBeUpdated) {
        return;
    }
    for (UWORD i = 0; i < pAdvancedSprite->ubSpriteCount; i++) {
        pAdvancedSprite->pSprites[i]->wX = pAdvancedSprite->wX + addAttachedX(pAdvancedSprite, i);

        pAdvancedSprite->pSprites[i]->wY = pAdvancedSprite->wY;

        pAdvancedSprite->pSprites[i]->isEnabled = pAdvancedSprite->isEnabled;

        pAdvancedSprite->pSprites[i]->isHeaderToBeUpdated = 1;

        spriteProcess(pAdvancedSprite->pSprites[i]);
    }
    pAdvancedSprite->isHeaderToBeUpdated=0;
}
