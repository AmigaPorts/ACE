# Using advanced multiplexed sprites

## About advanced multiplexed sprites

The [advanced multiplexed sprite manager](../../include/ace/managers/advancedmultiplexedsprite.h)
builds on the [multiplexed sprite manager](multiplexedsprites.md). It adds:

- animation frames;
- 32px-wide sprites;
- 15-color sprites (4bpp, attached channel pairs).

One object holds several logical elements. Each has its own position,
animation frame and enabled state, and they share the same hardware channels.

## Channels used

| Width | Depth | Channels | Valid first channel |
|-------|-------|----------|---------------------|
| 16px  | 2bpp  | 1        | 0..7                |
| 32px  | 2bpp  | 2        | 0..6                |
| 16px  | 4bpp  | 2        | 0, 2, 4, 6          |
| 32px  | 4bpp  | 4        | 0, 2, 4             |

A 4bpp image uses an even channel for bitplanes 0/1 and the next odd channel,
with ATTACH set, for bitplanes 2/3. It uses colors 17..31. A 32px image is two
16px columns, the second one at X + 16:

| Piece of a 32px/4bpp image | Channel | Position  | Bitplanes |
|----------------------------|---------|-----------|-----------|
| Left, low                  | +0      | X, Y      | 0/1       |
| Left, high (attached)      | +1      | X, Y      | 2/3       |
| Right, low                 | +2      | X + 16, Y | 0/1       |
| Right, high (attached)     | +3      | X + 16, Y | 2/3       |

A 32px/2bpp object starting on an odd channel uses two different palette
groups for its columns. Set both groups if they should look the same.

## Limits

All limits of the [multiplexed sprite manager](multiplexedsprites.md#limits)
apply:

- elements on the same channels can't overlap vertically;
- no clipping: a 16px column whose hardware position is out of range is
  skipped (an attached pair stays together);
- OCS/ECS 16px sprite fetch only. 32px here means two 16px columns, not AGA
  wide sprites;
- mixing with `sprite.h` only works in raw copper mode.

## Initialization

There's no separate manager. Create the multiplexed sprite manager, then add
objects:

```c
#include <ace/managers/advancedmultiplexedsprite.h>

multiplexedSpriteManagerCreate(s_pView, uwRawCopPos, 0);
s_pEnemies = advancedMultiplexedSpriteAdd(4, pStrip, 0, 12, 6);
```

```c
tAdvancedMultiplexedSprite *advancedMultiplexedSpriteAdd(
  UBYTE ubChannelIndex, tBitMap *pStrip1, tBitMap *pStrip2,
  UBYTE ubSpriteHeight, UBYTE ubElementCount
);
```

- `ubChannelIndex`: first channel. All the channels needed must be free.
- `pStrip1`: animation strip. `pStrip2`: optional second strip with the same
  width and depth, or 0.
- `ubSpriteHeight`: height of one frame, 1..255.
- `ubElementCount`: number of logical elements, 1..255.

It returns 0 on invalid input, a used channel or allocation failure.

Strips are vertical, interleaved (`BMF_INTERLEAVED`), 16 or 32px wide and 2 or
4bpp. Their height must be a multiple of the frame height. They contain pixel
rows only, without control words. `bitmapCreate(width, height * frameCount,
depth, BMF_CLEAR | BMF_INTERLEAVED)` gives the right layout.

Frames are numbered from 0, from top to bottom of strip 1, then strip 2.

**The frames are copied on Add**, so you can free the strips right after.

All elements start **enabled**, at (0,0), on frame 0. Move or disable unused
elements before the first frame, or they will all compete at Y=0.

## API

```c
void advancedMultiplexedSpriteSetPos(tAdvancedMultiplexedSprite *pSprite,
  UBYTE ubIndex, WORD wX, WORD wY);
void advancedMultiplexedSpriteSetFrame(tAdvancedMultiplexedSprite *pSprite,
  UBYTE ubIndex, UWORD uwFrame);
void advancedMultiplexedSpriteSetEnabled(tAdvancedMultiplexedSprite *pSprite,
  UBYTE ubIndex, UBYTE isEnabled);
void advancedMultiplexedSpriteProcess(tAdvancedMultiplexedSprite *pSprite);
void advancedMultiplexedSpriteProcessChannel(
  tAdvancedMultiplexedSprite *pSprite);
void advancedMultiplexedSpriteRemove(tAdvancedMultiplexedSprite *pSprite);
```

- Positions are relative to the view's top-left corner.
- Frames don't advance by themselves: call `SetFrame` for each element.
- `pSprite->isEnabled` shows or hides the whole object.
- `ProcessChannel` takes the object, not a channel number, and updates all its
  channels.
- 0 sprite pointers and invalid indices or frames are ignored.

## Every frame

```c
advancedMultiplexedSpriteProcess(s_pEnemies);
advancedMultiplexedSpriteProcessChannel(s_pEnemies);
// ...other advanced objects and ordinary channels...
viewProcessManagers(s_pView);
copProcessBlocks();
vPortWaitForEnd(s_pLastVPort);
```

As with ordinary multiplexed sprites, process every object every frame, then
swap the copper list once.

## Example

Two 32x16, 4bpp elements with two frames, on channels 0..3:

```c
static tAdvancedMultiplexedSprite *s_pSprites;

static void spritesCreate(void) {
  multiplexedSpriteManagerCreate(s_pView, 0, 0);
  tBitMap *pStrip = bitmapCreate(32, 32, 4, BMF_CLEAR | BMF_INTERLEAVED);
  blitRect(pStrip, 0, 0, 32, 16, 1);   // Frame 0
  blitRect(pStrip, 0, 16, 32, 16, 15); // Frame 1
  blitWait();
  s_pSprites = advancedMultiplexedSpriteAdd(0, pStrip, 0, 16, 2);
  bitmapDestroy(pStrip); // Frames were copied
  advancedMultiplexedSpriteSetPos(s_pSprites, 0, 40, 40);
  advancedMultiplexedSpriteSetPos(s_pSprites, 1, 80, 64);
  advancedMultiplexedSpriteSetFrame(s_pSprites, 1, 1);
  s_pVPort->pPalette[17] = 0xFFF;
  s_pVPort->pPalette[31] = 0xF80;
  viewLoad(s_pView);
  systemSetDmaBit(DMAB_SPRITE, 1);
}

static void spritesProcess(void) {
  advancedMultiplexedSpriteProcess(s_pSprites);
  advancedMultiplexedSpriteProcessChannel(s_pSprites);
  viewProcessManagers(s_pView);
  copProcessBlocks();
  vPortWaitForEnd(s_pVPort);
}

static void spritesDestroy(void) {
  systemSetDmaBit(DMAB_SPRITE, 0);
  viewLoad(0);
  advancedMultiplexedSpriteRemove(s_pSprites);
  multiplexedSpriteManagerDestroy();
  viewDestroy(s_pView);
}
```

Check the results of `bitmapCreate()` and `advancedMultiplexedSpriteAdd()` in
real code.

For a full game using both managers together with `sprite.h`, see the shooter
examples in the showcase (`showcase/src/examples/shooter_horizontal` and
`shooter_vertical`).

## Removing

Remove advanced objects **before** `multiplexedSpriteManagerDestroy()`: the
manager can't free them for you. Apart from that, follow the
[ordinary teardown rules](multiplexedsprites.md#removing-sprites).

## Performance notes

- Frames are split into 16px/2bpp pieces once, on Add, without blits.
- Selecting the current frame again does nothing.
- Each channel keeps its sort order and only copies changed pixels, like the
  ordinary manager. Processing doesn't allocate or log.
