# Using multiplexed sprites

## About multiplexed sprites

The Amiga has eight hardware sprite channels. A channel can show several
images per frame if they don't overlap vertically: once a sprite ends, the
hardware reads the next control words and starts another one lower down.

The [multiplexed sprite manager](../../include/ace/managers/multiplexedsprite.h)
does this for you. You set the position, height and bitmap of each element.
Every frame the manager sorts the elements by Y, builds the DMA stream and
updates the channel's copper pointer.

- Elements are 16px wide and 2bpp: three colors plus transparent.
- Channels 0/1 use colors 17..19, 2/3 use 21..23, 4/5 use 25..27 and 6/7 use
  29..31.
- Multiplexing raises the number of sprites per frame, not per scanline.

For animations, 32px-wide or 15-color (attached) sprites, use the
[advanced multiplexed sprite manager](advancedmultiplexedsprites.md) on top of
this one.

## Limits

- **No vertical overlap on a channel.** An element is shown only if it starts
  at least one line after the previous one ends (`VSTART >= previous VSTOP + 1`),
  because the VSTOP line reloads the control words. With height 16 at Y=40,
  the next element can start at Y=57. Overlapping elements are skipped for
  that frame. Elements are not moved to another channel.
- **No clipping.** An element is skipped if its hardware position is out of
  range: HSTART 0..511, VSTART >= 26, VSTOP <= 511. The VSTART limit is the PAL
  first sprite line, also used on NTSC.
- **OCS/ECS sprite format only.** Data is laid out for the 16px sprite fetch.
  With `ACE_USE_AGA_FEATURES`, keep the sprite fetch width at 16px (FMODE bits
  2-3 cleared) wherever these sprites are shown. The bitplane fetch mode
  doesn't matter. AGA wide sprites, fine X positioning and sprite palette
  banks aren't supported. Not tested on AGA.
- **Using it with `sprite.h`** only works in raw copper mode. See
  [Using it with the sprite manager](#using-it-with-the-sprite-manager).

## Initialization

```c
#include <ace/managers/multiplexedsprite.h>

multiplexedSpriteManagerCreate(s_pView, uwRawCopPos, 0);
```

Call this once, before adding sprites and before `viewLoad()`. It blanks all
eight sprite pointers.

- `pView` must stay alive until `multiplexedSpriteManagerDestroy()`.
- `pBlankSprite`: pass 0 and the manager allocates one, or pass a zeroed CHIP
  `ULONG` that you keep until the manager is destroyed.
- If creation fails, `multiplexedSpriteAdd()` returns 0.

**Block copper mode:** `uwRawCopPos` is ignored. The manager creates its own
copper blocks.

**Raw copper mode:** reserve 16 MOVE slots starting at `uwRawCopPos`, two per
channel (channel `n` uses `uwRawCopPos + 2 * n` and the next one). They must
run before sprite DMA starts, so put them at the start of the list. The
manager doesn't check the size of your list.

## Adding sprites

```c
tMultiplexedSprite *multiplexedSpriteAdd(
  UBYTE ubChannelIndex, UBYTE ubSpriteHeight, UBYTE ubElementCount
);
```

This reserves channel `ubChannelIndex` (0..7) with `ubElementCount` elements,
each at most `ubSpriteHeight` pixels tall (both 1..255). It returns 0 on bad
arguments, if the channel is in use, or on allocation failure.

All elements start **disabled**, at (0,0), with no bitmap. The channel itself
is enabled (`pSprite->isEnabled`).

Element bitmaps are borrowed and never written:

- 16px wide, 2bpp, `BMF_INTERLEAVED`;
- pixel rows only, **without** the control words and terminator that
  `sprite.h` bitmaps contain;
- at most `ubSpriteHeight` rows.

Keep a bitmap alive while an element uses it, even while that element is
disabled. Several elements can share one bitmap. After editing its pixels,
call `multiplexedSpriteSetBitmap()` again with the same pointer.

Each channel uses two CHIP DMA buffers of `(ubSpriteHeight + 1) *
ubElementCount + 1` rows of 4 bytes.

## Element API

```c
void multiplexedSpriteSetBitmap(tMultiplexedSprite *pSprite, UBYTE ubIndex,
  tBitMap *pBitmap);
void multiplexedSpriteSetPos(tMultiplexedSprite *pSprite, UBYTE ubIndex,
  WORD wX, WORD wY);
void multiplexedSpriteSetEnabled(tMultiplexedSprite *pSprite, UBYTE ubIndex,
  UBYTE isEnabled);
void multiplexedSpriteSetHeight(tMultiplexedSprite *pSprite, UBYTE ubIndex,
  UWORD uwHeight);
void multiplexedSpriteSetAttached(tMultiplexedSprite *pSprite, UBYTE ubIndex,
  UBYTE isAttached);
void multiplexedSpriteSetElement(tMultiplexedSprite *pSprite, UBYTE ubIndex,
  UWORD uwHeight, UBYTE isEnabled, UBYTE isAttached);
```

- Positions are relative to the **view**'s top-left corner, not a viewport.
- `SetBitmap` also sets the height to the bitmap's height.
- `SetHeight` accepts 1..maximum height, and no more than the bitmap's height.
  It shows the first rows of the bitmap.
- `SetAttached` only works on odd channels. You have to keep the even channel
  in sync yourself; the advanced manager does that for you.
- 0 sprite pointers, invalid indices and invalid values are ignored.

## Every frame

```c
multiplexedSpriteProcess(pSprite);           // Build the DMA stream
multiplexedSpriteProcessChannel(ubChannel);  // Update the copper pointer
// ...same for every other channel...
viewProcessManagers(s_pView);
copProcessBlocks();                          // One swap per frame
vPortWaitForEnd(s_pLastVPort);
```

Do this for **every channel, every frame**, even if nothing changed. The
manager has one DMA buffer per copper buffer, so a skipped channel would show
the previous frame's buffer.

None of these functions wait for the beam. Before a buffer is rewritten, the
frame showing it must be finished. Waiting with `vPortWaitForEnd()` on the
last viewport is enough if your sprites don't go below it.

## Example

```c
static tMultiplexedSprite *s_pSprites;
static tBitMap *s_pPixels;

static void spritesCreate(void) {
  multiplexedSpriteManagerCreate(s_pView, 0, 0);
  s_pSprites = multiplexedSpriteAdd(0, 16, 2);
  s_pPixels = bitmapCreate(16, 16, 2, BMF_CLEAR | BMF_INTERLEAVED);
  blitRect(s_pPixels, 0, 0, 16, 16, 1);
  blitWait();
  for(UBYTE i = 0; i < 2; ++i) {
    multiplexedSpriteSetBitmap(s_pSprites, i, s_pPixels);
    multiplexedSpriteSetPos(s_pSprites, i, 40, 40 + 24 * i);
    multiplexedSpriteSetEnabled(s_pSprites, i, 1);
  }
  s_pVPort->pPalette[17] = 0xFFF;
  viewLoad(s_pView);
  systemSetDmaBit(DMAB_SPRITE, 1);
}

static void spritesProcess(void) {
  multiplexedSpriteProcess(s_pSprites);
  multiplexedSpriteProcessChannel(0);
  viewProcessManagers(s_pView);
  copProcessBlocks();
  vPortWaitForEnd(s_pVPort);
}

static void spritesDestroy(void) {
  systemSetDmaBit(DMAB_SPRITE, 0);
  viewLoad(0);
  multiplexedSpriteRemove(s_pSprites);
  bitmapDestroy(s_pPixels);
  multiplexedSpriteManagerDestroy();
  viewDestroy(s_pView);
}
```

Check the results of `multiplexedSpriteAdd()` and `bitmapCreate()` in real
code.

## Using it with the sprite manager

Both managers blank all eight sprite pointers when created, so they must not
overwrite each other. The tested setup:

- raw copper mode;
- the same 16-slot `uwRawCopPos` and the same blank sprite for both;
- both created before any sprite is added;
- each channel owned by only one manager.

```c
s_pBlankSprite = memAllocChipClear(sizeof(ULONG));
spriteManagerCreate(s_pView, 0, s_pBlankSprite);
multiplexedSpriteManagerCreate(s_pView, 0, s_pBlankSprite);
s_pPlayer = spriteAdd(0, s_pPlayerBitmap);     // sprite.h owns channel 0
s_pBullets = multiplexedSpriteAdd(2, 2, 12);   // multiplexer owns channel 2
```

Each manager then only writes the slots of its own channels. Free the blank
sprite after both managers are destroyed. Block copper mode isn't supported
for this: each manager creates a block that blanks all eight channels.

See the two shooter examples in the showcase, under
`showcase/src/examples/shooter_horizontal` and `shooter_vertical`.

## Removing sprites

```c
void multiplexedSpriteRemove(tMultiplexedSprite *pSprite);
void multiplexedSpriteManagerDestroy(void);
```

Final teardown order: wait for the blitter, disable sprite DMA,
`viewLoad(0)`, remove advanced multiplexed sprites, remove ordinary ones,
destroy the manager, then destroy the view. The manager destroy frees any
ordinary sprites still left.

To remove a sprite while the view stays loaded, disable sprite DMA first.
Keep it disabled until both copper buffers have been rebuilt: in raw mode
call `multiplexedSpriteProcessChannel()` for the removed channel and swap the
copper list twice. Disabling the sprite alone doesn't make it safe to free.

## Performance notes

- Elements are only re-sorted when a Y position changes. Insertion sort is
  used because the order rarely changes much between frames.
- Pixels are only copied when an element's bitmap or its place in the DMA
  stream changes, tracked separately for each buffer.
- Processing doesn't allocate, log or start blits. It waits for the blitter
  before reading source pixels.
