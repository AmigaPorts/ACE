# Blitting with mask

So far, you've blitted solid rectangles and maskless backgrounds, as well as used maskless blits to copy stuff to background restore buffers.
Now it's time to do masked blits.

## How masked hardware works

TODO: move this to separate article on how blitter works and elaborate it A LOT.

In a nutshell, blitter operates by mixing up to three source channels (bitmaps A, B and C) into one output channel (D).

- To copy data with width equal to a multiple of 16, only single input channel is required and blit can be logically described as `D=A`, `D=B` or `D=C`.
- In order to get rid of some vertical lines from edges of your blit, you need to mask them out - typically channel A acts as mask, and with B being a source bitmap this can be described as `D=A*B`.
- However, this will make masked-out edges filled with zeros, so to counter it one needs to also feed blitter with background data in channel C, and use cookie-cut operation: `D=A*B + !A*C`.

The main takeaway is that for masked blits you need to specify three bitmaps for blitter and use all four channels:

- source for blitted object's mask (channel A),
- source for blitted object (channel B),
- source and destination for background data (channels C and D).

Since blitter operates on bitplane-by-bitplane manner, you can't just ask it to treat specific color index as mask - it expects you to pass in additional bitplane with mask information.
The upside of this is you don't need to reserve one of the colors for alpha channel  since you pass this data separately.
The downside is that you need to store mask separately, which results in a bit more data in memory.

## Converting images with mask data for ACE

The example uses following bitmap for paddles and ball - copy it to your `res` folder.

![Paddles and ball](../../showcase/res/pong_paddles.png)

To generate ACE's .bm as well as its mask counterpart, add another `convertBitmaps()` call in your `CMakeLists.txt` file.
Note that `MASK_COLOR` specifies which color should be used as a source for mask data - it can be, and even should be, the color outside of your project's regular palette.

```cmake
convertBitmaps(
  TARGET ${GAME_LINKED} PALETTE ${RES_DIR}/pong.gpl
  MASK_COLOR "#FF00FF"
  SOURCES
    ${RES_DIR}/pong_paddles.png
  DESTINATIONS
    ${DATA_DIR}/pong_paddles.bm
  MASKS
    ${DATA_DIR}/pong_paddles_mask.bm
)
```

Observe that after building your game, two files appeared:

- pong_paddles.bm is a 4BPP bitmap with all `MASK_COLOR` pixels converted to color zero,
- pong_paddles_mask.bm is a 1BPP bitmap with zeros set where `MASK_COLOR` was and with other pixels encoded as ones.

## Using masked images with ACE

What you need to do is:

- Load the bitmap from file like any other one.
- Load the extra mask bitmap - it's good practice to keep its variable name similar to the bitmap which will use the mask.
- Use `blitCopyMask()` function to draw masked bitmap.
  - You need to pass the mask as a `UWORD` pointer directly to where the data resides - typically it's start of bitmap's first bitplane (`pBmMask->Planes[0]`).
  - During blit, the function will auto-use the same X and Y offset for object's source as well as mask bitmaps.
- Be sure to clean up the bitmaps after they are no longer needed - in this case in `gameGsDestroy()`.

The example code looks as follows:

```c
// ...old defines here...
#define BALL_BG_BUFFER_WIDTH CEIL_TO_FACTOR(BALL_WIDTH, 16)
//-------------------------------------------------------------- NEW STUFF START
#define PADDLE_LEFT_BITMAP_OFFSET_Y 0
#define PADDLE_RIGHT_BITMAP_OFFSET_Y PADDLE_HEIGHT
#define BALL_BITMAP_OFFSET_Y (PADDLE_RIGHT_BITMAP_OFFSET_Y + PADDLE_HEIGHT)
//---------------------------------------------------------------- NEW STUFF END

// ...old static vars here...
static UBYTE s_hasBackgroundToRestore = 0;
//-------------------------------------------------------------- NEW STUFF START
static tBitMap *s_pBmObjects;
static tBitMap *s_pBmObjectsMask;
//---------------------------------------------------------------- NEW STUFF END

void gameGsCreate(void) {
  // ...old code...
  s_pBmPaddleLeftBg = bitmapCreate(PADDLE_BG_BUFFER_WIDTH, PADDLE_HEIGHT, 4, 0);
  s_pBmPaddleRightBg = bitmapCreate(PADDLE_BG_BUFFER_WIDTH, PADDLE_HEIGHT, 4, 0);
  s_pBmPaddleBallBg = bitmapCreate(BALL_BG_BUFFER_WIDTH, BALL_WIDTH, 4, 0);
//-------------------------------------------------------------- NEW STUFF START
  s_pBmObjects = bitmapCreateFromPath("data/pong_paddles.bm", 0);
  s_pBmObjectsMask = bitmapCreateFromPath("data/pong_paddles_mask.bm", 1);
//---------------------------------------------------------------- NEW STUFF END
  // ...old code...
}

void gameGsLoop(void) {
  // ...old code here...
//-------------------------------------------------------------- NEW STUFF START
  // Draw left paddle
  blitCopyMask(
    s_pBmObjects, 0, PADDLE_LEFT_BITMAP_OFFSET_Y,
    s_pMainBuffer->pBack, 0, uwPaddleLeftPosY,
    PADDLE_WIDTH, PADDLE_HEIGHT, s_pBmObjectsMask->Planes[0]
  );

  // Draw right paddle
  blitCopyMask(
    s_pBmObjects, 0, PADDLE_RIGHT_BITMAP_OFFSET_Y,
    s_pMainBuffer->pBack, uwPaddleRightPosX, uwPaddleRightPosY,
    PADDLE_WIDTH, PADDLE_HEIGHT, s_pBmObjectsMask->Planes[0]
  );

  // Draw ball
  blitCopyMask(
    s_pBmObjects, 0, BALL_BITMAP_OFFSET_Y,
    s_pMainBuffer->pBack,
    // x center: half of screen width minus half of ball
    (s_pVpMain->uwWidth - BALL_WIDTH) / 2,
    // y center: half of screen height minus half of ball
    (s_pVpMain->uwHeight - BALL_WIDTH) / 2,
    BALL_WIDTH, BALL_WIDTH, s_pBmObjectsMask->Planes[0]
  );
//---------------------------------------------------------------- NEW STUFF END
  vPortWaitForEnd(s_pVpMain);
}

void gameGsDestroy(void) {
  // ...old code here...
  bitmapDestroy(s_pBmPaddleBallBg);
//-------------------------------------------------------------- NEW STUFF START
  bitmapDestroy(s_pBmObjects);
  bitmapDestroy(s_pBmObjectsMask);
//---------------------------------------------------------------- NEW STUFF END

  // This will also destroy all associated viewports and viewport managers
  viewDestroy(s_pView);
}
```

Now your paddles and ball have dedicated graphics!
All that remains to have a working game is to add some motion to the ball and display the score.
