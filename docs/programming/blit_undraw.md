# Handling blitted object undraw

So far, you've managed to load and display the background, but paddles are trashing it with black color.
Now it's time to handle proper background restoring.

In order to fix this, you need to create bitmaps which you'll use as buffers to store background beneath the paddles.
To do so, add following code:

```c
// ...old defines go here...
//-------------------------------------------------------------- NEW STUFF START
#define PADDLE_BG_BUFFER_WIDTH CEIL_TO_FACTOR(PADDLE_WIDTH, 16)
#define BALL_BG_BUFFER_WIDTH CEIL_TO_FACTOR(BALL_WIDTH, 16)
//---------------------------------------------------------------- NEW STUFF END

// ...old static variables go here...
//-------------------------------------------------------------- NEW STUFF START
static tBitMap *s_pBmPaddleLeftBg;
static tBitMap *s_pBmPaddleRightBg;
static tBitMap *s_pBmPaddleBallBg;
static UBYTE s_hasBackgroundToRestore;
//---------------------------------------------------------------- NEW STUFF END

void gameGsCreate(void) {
  //... old code here
  // Draw line separating score VPort and main VPort, leave one line blank after it
  blitLine(
    s_pScoreBuffer->pBack,
    0, s_pVpScore->uwHeight - 2,
    s_pVpScore->uwWidth - 1, s_pVpScore->uwHeight - 2,
    SCORE_COLOR, 0xFFFF, 0 // Try patterns 0xAAAA, 0xEEEE, etc.
  );

//-------------------------------------------------------------- NEW STUFF START
  s_pBmPaddleLeftBg = bitmapCreate(PADDLE_BG_BUFFER_WIDTH, PADDLE_HEIGHT, 4, 0);
  s_pBmPaddleRightBg = bitmapCreate(PADDLE_BG_BUFFER_WIDTH, PADDLE_HEIGHT, 4, 0);
  s_pBmPaddleBallBg = bitmapCreate(BALL_BG_BUFFER_WIDTH, BALL_WIDTH, 4, 0);
  s_hasBackgroundToRestore = 0;
//---------------------------------------------------------------- NEW STUFF END

  systemUnuse();

  // Load the view
  viewLoad(s_pView);
}

void gameGsDestroy(void) {
  systemUse();

//-------------------------------------------------------------- NEW STUFF START
  bitmapDestroy(s_pBmPaddleLeftBg);
  bitmapDestroy(s_pBmPaddleRightBg);
  bitmapDestroy(s_pBmPaddleBallBg);
//---------------------------------------------------------------- NEW STUFF END

  // This will also destroy all associated viewports and viewport managers
  viewDestroy(s_pView);
}
```

This will create background buffers for both paddles and the ball the moment the game state gets created, and they will be destroyed along with it.
Note that bitmap widths are dependent on `PADDLE_BG_BUFFER_WIDTH` and its ball equivalent - remember that bitmaps need to have width equal to multiple of 16 - `CEIL_TO_FACTOR()` allows to easily ensure this.

Now it all boils down to saving fragments of display bitmap on paddles' positions before drawing them, and drawing them back on the start of the frame.
You need to also do the check, which will be used for the first loop pass, if background was actually saved before restoring it - without it random garbage contents from paddle background buffers would appear.

```c
void gameGsLoop(void) {
  // This will loop every frame
  if(keyCheck(KEY_ESCAPE)) {
    gameExit();
    return; // early return - don't process anything else, no need for big `else` anymore
  }

  UWORD uwPaddleRightPosX = s_pVpMain->uwWidth - PADDLE_WIDTH;

  if(s_hasBackgroundToRestore) {
    // Restore background under left paddle
    blitCopy(
      s_pBmPaddleLeftBg, 0, 0,
      s_pMainBuffer->pBack, 0, uwPaddleLeftPosY,
      PADDLE_WIDTH, PADDLE_HEIGHT, MINTERM_COOKIE
    );

    // Restore background under right paddle
    blitCopy(
      s_pBmPaddleRightBg, 0, 0,
      s_pMainBuffer->pBack, uwPaddleRightPosX, uwPaddleRightPosY,
      PADDLE_WIDTH, PADDLE_HEIGHT, MINTERM_COOKIE
    );
  }

  // Update left paddle position
  if(keyCheck(KEY_S)) {
    uwPaddleLeftPosY = MIN(uwPaddleLeftPosY + PADDLE_SPEED, PADDLE_MAX_POS_Y);
  }
  else if(keyCheck(KEY_W)) {
    uwPaddleLeftPosY = MAX(uwPaddleLeftPosY - PADDLE_SPEED, 0);
  }

  // Update right paddle position
  if(keyCheck(KEY_DOWN)) {
    uwPaddleRightPosY = MIN(uwPaddleRightPosY + PADDLE_SPEED, PADDLE_MAX_POS_Y);
  }
  else if(keyCheck(KEY_UP)) {
    uwPaddleRightPosY = MAX(uwPaddleRightPosY - PADDLE_SPEED, 0);
  }

  // Save background under left paddle
  blitCopy(
    s_pMainBuffer->pBack, 0, uwPaddleLeftPosY,
    s_pBmPaddleLeftBg, 0, 0,
    PADDLE_WIDTH, PADDLE_HEIGHT, MINTERM_COOKIE
  );

  // Save background under right paddle
  blitCopy(
    s_pMainBuffer->pBack, uwPaddleRightPosX, uwPaddleRightPosY,
    s_pBmPaddleRightBg, 0, 0,
    PADDLE_WIDTH, PADDLE_HEIGHT, MINTERM_COOKIE
  );
  s_hasBackgroundToRestore = 1;

  // Draw left paddle
  blitRect(
    s_pMainBuffer->pBack, 0, uwPaddleLeftPosY,
    PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_LEFT_COLOR
  );

  // Draw right paddle
  blitRect(
    s_pMainBuffer->pBack, uwPaddleRightPosX, uwPaddleRightPosY,
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
  vPortWaitForEnd(s_pVpMain);
}
```

As you can now observe, background is restored correctly.
There's a problem however - in debug builds, the right paddle flickers on its topmost position.
This is due to using single-buffered mode - the game simply works too slow to draw everything before it gets displayed on-screen.
Switching to release builds in your CMake options should help for now, but there are also some optimizations to be done which could help speed up blitter operations.

The next step is using proper paddle and ball graphics as well as animating the ball itself.
