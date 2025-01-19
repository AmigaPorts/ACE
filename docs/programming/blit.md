# Blitter

We still don't have anything except black screen - in this step we're going
to use some colors. This tutorial will at first aim to recreate classic Pong
game, so we need to be able to draw some rectangles and lines.

To use blit functions, you need to include `<ace/managers/blit.h>`.

## Blitting rectangles

For this task you can use `blitRect()` function, syntax is as follows:

``` c
UBYTE blitRect(
  tBitMap *pDst, WORD wDstX, WORD wDstY,
  WORD wWidth, WORD wHeight, UBYTE ubColor
);
```

Where `wDstX` and `wDstY` are coords on viewport manager's bitmap where
you want to start drawing. We'll use it to draw paddles and ball.

## Blitting lines

Line drawing is done by `blitLine()` function, which syntax is as follows:

``` c
void blitLine(
  tBitMap *pDst, WORD x1, WORD y1, WORD x2, WORD y2,
  UBYTE ubColor, UWORD uwPattern, UBYTE isOneDot
);
```

Bear in mind that `blitLine()` uses blitter's line mode, which makes
line drawing very DMA-consuming, so if you have to draw horizontal / vertical
lines you can most likely do them faster using `blitRect()` function by
specifying width or height equal to one.

Apart from slowdown, blitter's line drawing mode allows you to draw them using
any 16-bit pattern: for each 1 blitter puts a pixel, whereas for each 0 it does
nothing. Some examples are shown below:

``` plain
solid line (0xFFFF):
1111 1111 1111 1111 1111 1111 1111 1111

dotted line (0xAAAA):
1010 1010 1010 1010 1010 1010 1010 1010

dashed line (0xE4E4):
1110 0100 1110 0100
```

Last parameter is related to blitter fill mode - it expects filled region's
borders to be only 1px wide. If you set `isOneDot` parameter to 1, blitter will
ensure to plot only one pixel in a row.

### Faster demo-ish line drawing

Some demos speed up line drawing by doing them only on a single bitplane. This
function directly doesn't support it but you can trick it into such usage.
All you need to do is to create `tBitMap` struct with same width and height
as target bitmap, depth set to 1 and first bitplane pointer to the desired
bitplane. This way you will create two "entangled" bitmaps, one of which can be
used for general operations, second one for line drawing.

## Tutorial code

We're all set, so let's use those fns to finally draw something on screen.
Below you can see added `#include` for blit manager, some defines to make later
porion of code more readable, and then blits.

In next page of tutorial we'll add basic game logic, adding animations in the
process.

``` c
#include "game.h"
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
//-------------------------------------------------------------- NEW STUFF START
#include <ace/managers/blit.h> // Blitting fns

// Let's make code more readable by giving names to numbers
// It is a good practice to name constant stuff using uppercase
#define BALL_WIDTH 8
#define BALL_COLOR 1
#define PADDLE_WIDTH 8
#define PADDLE_HEIGHT 32
#define PADDLE_LEFT_COLOR 2
#define PADDLE_RIGHT_COLOR 3
#define SCORE_COLOR 1
#define WALL_HEIGHT 1
#define WALL_COLOR 1
//---------------------------------------------------------------- NEW STUFF END

static tView *s_pView; // View containing all the viewports
static tVPort *s_pVpScore; // Viewport for score
static tSimpleBufferManager *s_pScoreBuffer;
static tVPort *s_pVpMain; // Viewport for playfield
static tSimpleBufferManager *s_pMainBuffer;

void gameGsCreate(void) {
  s_pView = viewCreate(0, TAG_END);

  // Viewport for score bar - on top of screen
  s_pVpScore = vPortCreate(0,
    TAG_VPORT_VIEW, s_pView,
    TAG_VPORT_BPP, 4,
    TAG_VPORT_HEIGHT, 32,
  TAG_END);
  s_pScoreBuffer = simpleBufferCreate(0,
    TAG_SIMPLEBUFFER_VPORT, s_pVpScore,
    TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
  TAG_END);

  // Now let's do the same for main playfield
  s_pVpMain = vPortCreate(0,
    TAG_VPORT_VIEW, s_pView,
    TAG_VPORT_BPP, 4,
  TAG_END);
  s_pMainBuffer = simpleBufferCreate(0,
    TAG_SIMPLEBUFFER_VPORT, s_pVpMain,
    TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
  TAG_END);

  s_pVpScore->pPalette[0] = 0x0000; // First color is also border color
  s_pVpScore->pPalette[1] = 0x0888; // Gray
  s_pVpScore->pPalette[2] = 0x0800; // Red - not max, a bit dark
  s_pVpScore->pPalette[3] = 0x0008; // Blue - same brightness as red

//-------------------------------------------------------------- NEW STUFF START
  // Draw line separating score VPort and main VPort, leave one line blank after it
  blitLine(
    s_pScoreBuffer->pBack,
    0, s_pVpScore->uwHeight - 2,
    s_pVpScore->uwWidth - 1, s_pVpScore->uwHeight - 2,
    SCORE_COLOR, 0xFFFF, 0 // Try patterns 0xAAAA, 0xEEEE, etc.
  );

  // Draw wall on the bottom of main VPort
  blitRect(
    s_pMainBuffer->pBack,
    0, s_pVpMain->uwHeight - WALL_HEIGHT,
    s_pVpMain->uwWidth, WALL_HEIGHT, WALL_COLOR
  );
//---------------------------------------------------------------- NEW STUFF END

  systemUnuse();

  // Load the view
  viewLoad(s_pView);
}

void gameGsLoop(void) {
  // This will loop every frame
  if(keyCheck(KEY_ESCAPE)) {
    gameExit();
  }
  else {
//-------------------------------------------------------------- NEW STUFF START
    // Draw first paddle
    blitRect(
      s_pMainBuffer->pBack, 0, 0,
      PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_LEFT_COLOR
    );

    // Draw second paddle
    blitRect(
      s_pMainBuffer->pBack, s_pVpMain->uwWidth - PADDLE_WIDTH, 0,
      PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_RIGHT_COLOR
    );

    // Draw ball
    blitRect(
      s_pMainBuffer->pBack,
      // x center: half of screen width minus half of ball
      (s_pVpMain->uwWidth - BALL_WIDTH) / 2,
      // y center: half of screen height minus half of ball
      (s_pVpMain->uwHeight - BALL_WIDTH) / 2,
      BALL_WIDTH, BALL_WIDTH, BALL_COLOR
    );
    vPortWaitForEnd(s_pVpMain); // Wait for end of frame
//---------------------------------------------------------------- NEW STUFF END
  }
}

void gameGsDestroy(void) {
  systemUse();

  // This will also destroy all associated viewports and viewport managers
  viewDestroy(s_pView);
}
```
