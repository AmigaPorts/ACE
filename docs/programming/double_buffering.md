# Double buffering

When you draw directly on the bitmap that is being displayed, the beam may catch your drawing halfway through.
This shows up as tearing and flickering objects, especially when you erase and redraw moving things.
Double buffering solves this by using two bitmaps: one is displayed while you draw on the other one, and they swap roles every frame.

## Enabling double buffering

The simple buffer manager allocates the second bitmap for you when you pass `TAG_SIMPLEBUFFER_IS_DBLBUF`:

```c
s_pMainBuffer = simpleBufferCreate(0,
  TAG_SIMPLEBUFFER_VPORT, s_pVpMain,
  TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
  TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
TAG_DONE);
```

The manager then exposes two bitmaps:

- `pFront` is the bitmap that is currently displayed,
- `pBack` is the bitmap you should draw on.

## Swapping the buffers

The buffers are swapped by the simple buffer manager's process function, which is called by `viewProcessManagers()`.
It also writes the new bitplane pointers into the copperlist, which then needs to be refreshed with `copProcessBlocks()`.
The end of your gamestate loop should look like this:

```c
viewProcessManagers(s_pView);
copProcessBlocks();
vPortWaitForEnd(s_pVpMain);
```

> [!CAUTION]
> Both `viewProcessManagers()` and `copProcessBlocks()` need to be called every frame.
> If you skip `copProcessBlocks()`, the `pFront` and `pBack` pointers still swap, but the display keeps showing the same bitmap.
> As a result, every other frame you will be drawing on the displayed bitmap and the dynamic content will flicker.

## What to draw where

- Never draw on `pFront` during the gamestate loop - it is on screen and the beam will catch partial draws and erase operations.
- Draw everything that changes during the loop on `pBack`.
- Draw static content, such as backgrounds and HUD frames, on **both** `pFront` and `pBack` when creating the gamestate, since either of them may be displayed first.
- Redraw dynamic content, such as moving objects, counters and timers, on `pBack` **every frame**.

The last rule is the most common source of bugs when converting a single-buffered game.
If you redraw something only when it changes, it will be updated on one of the bitmaps only.
The other bitmap still holds the old content, so the display alternates between the old and the new state every frame.
If you really want to redraw only on change, keep redrawing it for two consecutive frames, so that both bitmaps receive the update.

## Undrawing objects

When you undraw objects manually (see [Handling blitted object undraw](blit_undraw.md)), the undraw bookkeeping needs to be tracked separately for each bitmap.
The object's previous position stored for `pBack` is the one from two frames ago, not from the previous frame - that one was drawn on the other bitmap.

To find out which bitmap you are currently drawing on, store both pointers when creating the gamestate and compare `pBack` against them:

```c
static tBitMap *s_pBuffers[2];
static tUwCoordYX s_pLastPos[2];

void gameGsCreate(void) {
  // ...
  s_pBuffers[0] = s_pMainBuffer->pFront;
  s_pBuffers[1] = s_pMainBuffer->pBack;
}

void gameGsLoop(void) {
  UBYTE ubBufferIndex = (s_pMainBuffer->pBack == s_pBuffers[0]) ? 0 : 1;
  // Undraw the object at s_pLastPos[ubBufferIndex], draw it at the new position
  // and store that position in s_pLastPos[ubBufferIndex].
}
```

> [!NOTE]
> Comparing `pBack` against `pFront` won't help you here - with double buffering enabled they always point to different bitmaps.

The [BOB manager](using_bobs.md) does this bookkeeping for you, as long as you pass it the current `pBack` in each `bobBegin()` call.
