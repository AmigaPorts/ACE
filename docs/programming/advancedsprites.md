# Using Advanced Aprites

## About Advanced Sprites

Advanced Sprites Manager is an overlay to the sprite manager.

It allows you to directly use:

- simple 4 colors & 16px wide sprites (same as the Sprites Manager).
- 4 colors & 32px wide sprites.
- 16 colors & 16px wide sprites.
- 16 colors & 32px wide sprites.

Be aware that it can take 2 or 4 channels on the 8 available on Amiga :

| Colors | Width | Start Channel | Number of Channels Taken |
|--------|-------|---------------|--------------------------|
| 4      | 16px  | Any           | 1                        |
| 4      | 32px  | Any           | 2                        |
| 16     | 16px  | Even (0,2,4,6)| 2                        |
| 16     | 32px  | Even (0,2,4,6)| 4                        |


## Main feature

- [X] Frames/animation
- [X] 32px sprites
- [X] 16 colors sprites
- [ ] Multiplexed sprites (not yet available)

## Initializing Advanced Sprites


Init your static variables :

```c
static tAdvancedSprite *s_pASprite;
```

Then init your manager in your *creation* part:
```c
spriteManagerCreate(s_pView, 0);
systemSetDmaBit(DMAB_SPRITE, 1);
```

## Adding sprites

Still in your *creation* part, first load/create your sprite stripe. It must be **vertical** stripe. It can be generated easily with tools like *Aseprite*. Not space between sprites, the same sprite height must be respected for the whole stripe.

```c
// Init your static variables
static tBitMap *s_pStripe32;
// [..]

// Init your bitmap in the creation part
s_pStripe32 = bitmapCreate(32, 32*10, 2, BMF_CLEAR|BMF_INTERLEAVED); // 16x32 2BPP
  for(int i=0; i<10; i++) {
    char msg[50];
    sprintf(msg, "%d",i);
	  fontDrawStr(s_pFont,  s_pStripe32, 0, i*32+0, msg, 1, FONT_LEFT | FONT_TOP | FONT_COOKIE, s_pTextBitMap);
    for(int j=0; j<4; j++) {
      blitRect(s_pStripe32,16*(j%2), i*32+8+8*((j-1>0)&1), randUwMinMax(g_sRand,8,16), randUwMinMax(g_sRand,4,8), j);
    }
  }
```

Then you inject your bitmap while creating your advanced sprites :

```c
// 2 is the channel number, 32 is the height
s_pASprite = advancedSpriteAdd(2, 32, s_pStripe32, NULL);
// Set Init position (x,y)
advancedSpriteSetPos(s_pASprite,180,100);
```

Please note, that you can destroy the stripe bitmap just after you've added the sprite with the bitmap.

```c
bitmapDestroy(s_pStripe32);
```

NULL can be replaced by a second stripe (bitmap are limited to 4096 height).

```c
s_pHeroSprite = advancedSpriteAdd(MAIN_SPRITE_CHANNEL, MAIN_SPRITE_HEIGHT, pLeftSprites, pRightSprites); 
bitmapDestroy(pRightSprites);
bitmapDestroy(pLeftSprites);
```


***Bonus :*** If your sprite is showing behind the view layer add this line after the `viewLoad` :
```c
// Reset blcon2 to put sprite in front of http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0159.html
  g_pCustom->bplcon2=0b00100000;
```

## Loop Management

At the end of of your **loop* part should add this 2 lines in order to :


```c
  advancedSpriteProcess(s_pASprite); // set sprites data
  advancedSpriteProcessChannel(2,s_pASprite);  // apply it memory and copper
```

Before that you can change the sprite position :

```c
advancedSpriteSetPos(s_pASprite,180,100); // ==> Set position
advancedSpriteSetPosX(s_pASprite,s_pASprite->wX-2); // ==> Move x by -2
advancedSpriteSetPosY(s_pASprite,s_pASprite->wY+2); // ==>  Move Y by +2
```

Do not use `s_pASprite->wX = ...` to change the value, use the available function. 


You can also change the current frame in order to manage animation or change of states :

```c
// 5 is the frame number starting at 0
advancedSpriteSetFrame(s_pASprite,5);
```

By default frame is 0 on `advancedSpriteAdd`. It's the top sprite of your stripe. Second one is 1, third is 2...
If you use Aseprite, it's the frame number minus 1 (1 -> 0, 2 --> 1, 3 -> 2, and so on).

Last function available is to enable/disable the sprite :

```c
advancedSpriteSetEnabled(s_pASprite,1); // enable
advancedSpriteSetEnabled(s_pASprite,0); // disable
```

By default it's enabled on `advancedSpriteAdd`. 


## Destroy Management

Don't forget to :

1 - Destroy advanced sprites
```c
advancedSpriteRemove(s_pASprite);
```

2 - destroy manager
```c
systemSetDmaBit(DMAB_SPRITE, 0); // Disable sprite DMA
spriteManagerDestroy();
```