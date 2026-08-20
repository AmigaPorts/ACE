# Using a sprite as a mouse pointer

Workbench draws the mouse with hardware sprite channel 0, and ACE games typically do the same.
A hardware sprite sits on top of the playfield without blitting, so you never have to undraw the pointer or punch a hole in your bitmap.

This tutorial assumes you already have a view, a viewport and a simple buffer, and that you have read [Using sprites](sprites.md).

A complete example lives in the showcase as the **Mouse sprite** test (`showcase/src/test/mouse_sprite.c`).

## Two managers, one pointer

ACE splits mouse **input** from mouse **graphics**:

- The [mouse manager](../../include/ace/managers/mouse.h) reads the hardware counters and buttons and keeps an X/Y position.
  It does not draw anything.
- The [sprite manager](../../include/ace/managers/sprite.h) displays a 2BPP interleaved bitmap on a sprite channel.
  It does not read the mouse.

Your job is to copy the mouse position onto a sprite each frame.

> [!NOTE]
> `spriteManagerCreate()` documents this split on purpose: the same sprite can be driven by mouse, joystick, or your own logic.

## Preparing the pointer bitmap

Hardware sprites are always 2BPP (four colors, color 0 transparent) and, on OCS/ECS, 16 pixels wide.
The sprite manager stores control words in the first and last rows of the bitmap, so those rows are **not displayed**.

Draw your pointer as:

- width 16 (OCS/ECS) — a multiple of 16 on AGA if you use wide sprites
- height equal to the visible cursor plus **two** empty rows (one header, one footer)

For a 16×16 visible arrow, the PNG is therefore 16×18.
The showcase cursors are 16×24 visible (`arrow` / `pencil` in [showcase/res/sprites/mouse](../../showcase/res/sprites/mouse)), stored as 16×26 cells.

Color 0 must be the transparent background.
The remaining three colors are the outline, fill, and any accent.

> [!CAUTION]
> If you convert a bitmap without the extra rows, `spriteSetBitmap()` will still steal the first and last lines for control words and you will lose two rows of graphics.

## Converting the pointer

Convert with [`sprite_conv`](../tools/sprite_conv.md) so the result is an interleaved 2BPP `.bm`.
From CMake, after `add_executable()`:

```cmake
set(RES_DIR ${CMAKE_CURRENT_LIST_DIR}/res)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)
file(MAKE_DIRECTORY ${DATA_DIR})

convertSprite(
  TARGET ${GAME_LINKED}
  PALETTE ${RES_DIR}/mouse.gpl
  SOURCE ${RES_DIR}/cursor.png
  DESTINATION ${DATA_DIR}/cursor.bm
)
```

`sprite_conv` matches PNG pixels against the palette you pass:

- indices `0..3` become sprite colors `0..3`
- indices `16..31` are also accepted and mapped down to sprite `0..3` (handy if your game palette already reserves `COLOR16..31` for sprites)

You can copy the showcase mouse art and `mouse.gpl` as a starting point.

## Sprite colors

Sprite pixels do **not** use playfield colors `1..3`.
Channel 0 (the usual mouse channel) is displayed with the upper half of the 32-color palette:

| Sprite pixel | 0 (transparent) | 1 | 2 | 3 |
|:------------:|:---------------:|:-:|:-:|:-:|
| Viewport color | 16 (unused) | 17 | 18 | 19 |

Set those slots on the viewport even if your playfield is only 4- or 5-bit:

```c
s_pVpMain->pPalette[17] = 0x000; // outline
s_pVpMain->pPalette[18] = 0xFFF; // fill
s_pVpMain->pPalette[19] = 0xF80; // accent
```

If you use `TAG_VIEW_GLOBAL_PALETTE` (the default), copy them onto the topmost viewport's palette.

> [!NOTE]
> `sprite_conv` also writes a `.plt` next to the `.bm`.
> That file is a copy of the **input** palette, not a 32-color display palette, so loading it into the viewport will not place colors at 17..19 for you.

## Setting up mouse and sprite

Create the mouse manager once — either in `genericCreate()` if the whole game uses a pointer, or in the gamestate that needs it.

```c
#include <ace/managers/mouse.h>
#include <ace/managers/sprite.h>
#include <ace/utils/custom.h>
#include <ace/generic/screen.h>
#include <hardware/dmabits.h>

static tSprite *s_pPtrSprite;
static tBitMap *s_pBmCursor;

void gameGsCreate(void) {
  // ... view / viewport / simplebuffer as usual ...

  s_pVpMain->pPalette[17] = 0x000;
  s_pVpMain->pPalette[18] = 0xFFF;
  s_pVpMain->pPalette[19] = 0xF80;

  mouseCreate(MOUSE_PORT_1);
  mouseSetBounds(
    MOUSE_PORT_1, 0, 0, SCREEN_PAL_WIDTH - 1, SCREEN_PAL_HEIGHT - 1
  );
  mouseSetPosition(
    MOUSE_PORT_1, SCREEN_PAL_WIDTH / 2, SCREEN_PAL_HEIGHT / 2
  );
#ifdef AMIGA
  // Seed hardware counters so the first mouseProcess() does not jump.
  {
    UWORD uwDat = g_pCustom->joy0dat;
    g_sMouseManager.pMice[MOUSE_PORT_1].ubPrevHwX = (UBYTE)(uwDat & 0xFF);
    g_sMouseManager.pMice[MOUSE_PORT_1].ubPrevHwY = (UBYTE)(uwDat >> 8);
  }
#endif

  s_pBmCursor = bitmapCreateFromPath("data/cursor.bm", 0);

  spriteManagerCreate(s_pView, 0, 0);
  systemSetDmaBit(DMAB_SPRITE, 1);
  s_pPtrSprite = spriteAdd(0, s_pBmCursor);
  s_pPtrSprite->wX = (WORD)mouseGetX(MOUSE_PORT_1);
  s_pPtrSprite->wY = (WORD)mouseGetY(MOUSE_PORT_1);
  spriteRequestMetadataUpdate(s_pPtrSprite);
  spriteProcess(s_pPtrSprite);
  spriteProcessChannel(0);

  viewLoad(s_pView);
  systemUnuse();
}
```

Channel 0 is the front-most sprite.
ACE's default `BPLCON2` already puts sprites 0–3 in front of the playfield, so a channel-0 pointer shows up without extra priority setup.

Call `mouseProcess()` **once** per frame — typically next to `keyProcess()` in `genericProcess()`, or at the start of the gamestate loop if you created the mouse there.

```c
void genericProcess(void) {
  keyProcess();
  mouseProcess();
  stateProcess(g_pGameStateManager);
}
```

## Updating the pointer each frame

Write the mouse coordinates into the sprite, then let the sprite manager refresh metadata and the copper pointer:

```c
void gameGsLoop(void) {
  if(keyCheck(KEY_ESCAPE)) {
    gameExit();
    return;
  }

  s_pPtrSprite->wX = (WORD)mouseGetX(MOUSE_PORT_1);
  s_pPtrSprite->wY = (WORD)mouseGetY(MOUSE_PORT_1);
  spriteRequestMetadataUpdate(s_pPtrSprite);

  spriteProcess(s_pPtrSprite);
  spriteProcessChannel(0);
  copProcessBlocks();
  vPortWaitForEnd(s_pVpMain);
}
```

`spriteProcess()` is cheap when nothing changed, so calling it every frame is fine.
`copProcessBlocks()` is required: without it the copper never picks up the sprite pointer writes.

## Hotspot

`tSprite.wX` / `wY` is the top-left of the **visible** sprite, while `mouseGetX()` / `mouseGetY()` is the logical cursor point.
For an arrow that usually is `(0, 0)`.
For a pencil or crosshair, subtract the pixel that should sit under the mouse:

```c
#define CURSOR_HOT_X 0
#define CURSOR_HOT_Y 0

s_pPtrSprite->wX = (WORD)(mouseGetX(MOUSE_PORT_1) - CURSOR_HOT_X);
s_pPtrSprite->wY = (WORD)(mouseGetY(MOUSE_PORT_1) - CURSOR_HOT_Y);
spriteRequestMetadataUpdate(s_pPtrSprite);
```

Keep using the unadjusted mouse coordinates for hit-tests (`mouseInRect()`, clicking tiles, and so on).

## Cleanup

Tear down in the reverse order of creation:

```c
void gameGsDestroy(void) {
  viewLoad(0);
  systemUse();
  systemSetDmaBit(DMAB_SPRITE, 0);
  spriteManagerDestroy(); // also removes s_pPtrSprite
  bitmapDestroy(s_pBmCursor);
  mouseDestroy();
  viewDestroy(s_pView);
}
```

If the mouse manager was created in `genericCreate()`, destroy it in `genericDestroy()` instead.

## Buttons and swapping cursors

`mouseCheck()` is the held state, `mouseUse()` is an edge trigger (press once, then consumed).
You can swap the sprite bitmap when the button is down — the showcase switches arrow → pencil on LMB:

```c
if(mouseCheck(MOUSE_PORT_1, MOUSE_LMB)) {
  spriteSetBitmap(s_pPtrSprite, s_pBmPencil);
}
else {
  spriteSetBitmap(s_pPtrSprite, s_pBmArrow);
}
spriteRequestMetadataUpdate(s_pPtrSprite);
```

> [!NOTE]
> Do not share one bitmap across two sprites: the manager writes control words into the bitmap.
> Give each cursor its own `.bm`.

To paint or pick with the pointer, use the mouse position rather than the sprite position (so the hotspot stays correct):

```c
if(mouseCheck(MOUSE_PORT_1, MOUSE_LMB)) {
  blitRect(
    s_pMainBuffer->pBack,
    mouseGetX(MOUSE_PORT_1), mouseGetY(MOUSE_PORT_1),
    2, 2, 1
  );
}
```

## Common pitfalls

- **Forgotten sprite DMA.** `spriteManagerCreate()` does not enable it. Call `systemSetDmaBit(DMAB_SPRITE, 1)`.
- **Forgotten `copProcessBlocks()`.** Sprite copper updates will not appear.
- **First-frame jump.** `mouseCreate()` does not seed `ubPrevHwX` / `ubPrevHwY` from `joy0dat`. Copy those counters once after create, as in the setup snippet.
- **Wrong palette slots.** Channel 0 uses colors 17–19, not 1–3.
- **Missing header/footer rows.** Visible height is `bitmap->Rows - 2`.
- **Double `mouseProcess()`.** Calling it in both `genericProcess()` and the gamestate loop applies movement twice.

## Attached 16-color pointers

A single channel is four colors. For a 16-color cursor, attach channel 1 to channel 0: the even sprite supplies the low 2 bits, the odd sprite the high 2 bits.

Convert the PNG with [`sprite_conv -attached`](../tools/sprite_conv.md) (or `convertSprite(... ATTACHED LO ... HI ...)`), add both bitmaps, and mark the odd sprite as attached:

```c
s_pSprLo = spriteAdd(0, s_pBmCursorLo);
s_pSprHi = spriteAdd(1, s_pBmCursorHi);
spriteSetAttached(s_pSprHi, 1);
```

Keep both sprites on the same X/Y (including hotspot) and process both channels each frame. Attached pair 0+1 uses palette colors 17..31; color 16 stays transparent.

See [Using sprites](sprites.md) for how attached pairs map onto the other channels.
