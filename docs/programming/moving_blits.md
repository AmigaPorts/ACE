# Moving blitted objects

Last time, we've added static paddles and ball for pong-like game.
Now it's time to make things move.

The important thing to understand is that blitter draws objects directly on the bitmap which we'll then display.
The problem is, if you draw the paddle in new position, it will still be visible in old one.
To visualize this problem, slightly modify your game.c code and add ability to move the right paddle up and down with keyboard's arrow keys:

```c
// ... rest is without changes
#define WALL_HEIGHT 1
#define WALL_COLOR 1
//-------------------------------------------------------------- NEW STUFF START
#define PLAYFIELD_HEIGHT (256-32)
#define PADDLE_MAX_POS_Y (PLAYFIELD_HEIGHT - PADDLE_HEIGHT - 1)
#define PADDLE_SPEED 4
//---------------------------------------------------------------- NEW STUFF END

static tView *s_pView; // View containing all the viewports
static tVPort *s_pVpScore; // Viewport for score
static tSimpleBufferManager *s_pScoreBuffer;
static tVPort *s_pVpMain; // Viewport for playfield
static tSimpleBufferManager *s_pMainBuffer;

//-------------------------------------------------------------- NEW STUFF START
static UWORD uwPaddleLeftPosY = 0;
static UWORD uwPaddleRightPosY = 0;
//---------------------------------------------------------------- NEW STUFF END

// ... rest is without changes

void gameGsLoop(void) {
  // This will loop every frame
  if(keyCheck(KEY_ESCAPE)) {
    gameExit();
  }
  else {
//-------------------------------------------------------------- NEW STUFF START
  if(keyCheck(KEY_DOWN)) {
    uwPaddleRightPosY = MIN(uwPaddleRightPosY + PADDLE_SPEED, PADDLE_MAX_POS_Y);
  }
  else if(keyCheck(KEY_UP)) {
    uwPaddleRightPosY = MAX(uwPaddleRightPosY - PADDLE_SPEED, 0);
  }
//---------------------------------------------------------------- NEW STUFF END

    // Draw first paddle
    // NEW: uwPaddleLeftPosY
    blitRect(
      s_pMainBuffer->pBack, 0, uwPaddleLeftPosY,
      PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_LEFT_COLOR
    );

    // Draw second paddle
    // NEW: uwPaddleRightPosY
    blitRect(
      s_pMainBuffer->pBack, s_pVpMain->uwWidth - PADDLE_WIDTH, uwPaddleRightPosY,
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
  }
}
```

After running the code, you'll see that moving the paddle down makes it draw the blue vertical bar.
We'll fix that shortly, but first a bit more of computer graphics theory.

## Immediate and retained draw modes

Modern computers are quick enough to draw the whole frame from scratch.
This makes generating graphics quite simple and consists of following steps:

- clear the display bitmap
- draw the background across the whole bitmap
- draw all the foreground objects
- display the bitmap

This however comes with big performance cost and isn't possible to do on stock Amiga hardware while keeping decent frame rate and color count.
Even clearing the whole display bitmap would eat around half of your frame time budget!

The other way to do things is to use retained mode - redraw only things that are moving and keeping remaining things intact.
Assuming there is some kind of background, and we have some foreground objects, all we need to do is:

- "undraw" the moving object with background, which we've stored prior to drawing it, by drawing the background on old position of the object,
- store the background under new position, which will be used for next undraw,
- draw the object on new position

Since the pong game uses solid background, you don't need to store it - you just have to draw solid black color on old position to undraw the paddle, and then draw it in the new one.

## Tutorial code

The following code correctly moves paddles up and down without any kind of trails:

```c
#include "game.h"
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
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
//-------------------------------------------------------------- NEW STUFF START
#define PLAYFIELD_HEIGHT (256-32)
#define PADDLE_MAX_POS_Y (PLAYFIELD_HEIGHT - PADDLE_HEIGHT - 1)
#define PADDLE_SPEED 4
//---------------------------------------------------------------- NEW STUFF END

static tView *s_pView; // View containing all the viewports
static tVPort *s_pVpScore; // Viewport for score
static tSimpleBufferManager *s_pScoreBuffer;
static tVPort *s_pVpMain; // Viewport for playfield
static tSimpleBufferManager *s_pMainBuffer;

//-------------------------------------------------------------- NEW STUFF START
static UWORD uwPaddleLeftPosY = 0;
static UWORD uwPaddleRightPosY = 0;
//---------------------------------------------------------------- NEW STUFF END

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
  // Undraw left paddle
  blitRect(
    s_pMainBuffer->pBack, 0, uwPaddleLeftPosY,
    PADDLE_WIDTH, PADDLE_HEIGHT, 0 // color zero is black
  );

  // Update left paddle position
  if(keyCheck(KEY_S)) {
    uwPaddleLeftPosY = MIN(uwPaddleLeftPosY + PADDLE_SPEED, PADDLE_MAX_POS_Y);
  }
  else if(keyCheck(KEY_W)) {
    uwPaddleLeftPosY = MAX(uwPaddleLeftPosY - PADDLE_SPEED, 0);
  }

  // Undraw right paddle
  blitRect(
    s_pMainBuffer->pBack, s_pVpMain->uwWidth - PADDLE_WIDTH, uwPaddleRightPosY,
    PADDLE_WIDTH, PADDLE_HEIGHT, 0 // color zero is black
  );

  // Update right paddle position
  if(keyCheck(KEY_DOWN)) {
    uwPaddleRightPosY = MIN(uwPaddleRightPosY + PADDLE_SPEED, PADDLE_MAX_POS_Y);
  }
  else if(keyCheck(KEY_UP)) {
    uwPaddleRightPosY = MAX(uwPaddleRightPosY - PADDLE_SPEED, 0);
  }
    // Draw left paddle - NEW: uwPaddleLeftPosY
    blitRect(
      s_pMainBuffer->pBack, 0, uwPaddleLeftPosY,
      PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_LEFT_COLOR
    );

    // Draw right paddle - NEW: uwPaddleRightPosY
    blitRect(
      s_pMainBuffer->pBack, s_pVpMain->uwWidth - PADDLE_WIDTH, uwPaddleRightPosY,
      PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_RIGHT_COLOR
    );
//---------------------------------------------------------------- NEW STUFF END

    // Draw ball
    blitRect(
      s_pMainBuffer->pBack,
      // x center: half of screen width minus half of ball
      (s_pVpMain->uwWidth - BALL_WIDTH) / 2,
      // y center: half of screen height minus half of ball
      (s_pVpMain->uwHeight - BALL_WIDTH) / 2,
      BALL_WIDTH, BALL_WIDTH, BALL_COLOR
    );
    vPortWaitForEnd(s_pVpMain);
  }
}

void gameGsDestroy(void) {
  systemUse();

  // This will also destroy all associated viewports and viewport managers
  viewDestroy(s_pView);
}
```
