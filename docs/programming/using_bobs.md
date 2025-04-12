# Working with BOBs (Blitter OBjects)

ACE provides a powerful system for managing and animating 2D sprites using the Amiga's hardware blitter through its BOB (Blitter OBject) system.

BOBs are 2D graphic elements that can be efficiently moved around the screen using the Amiga's hardware blitter.
They do some busywork for you:

- Manage background restoring after moving/removing the object,
- Optimize blitter operations in contrast to more versatile `blitCopy...()` functions
- Optimize memory usage for either minimal or extensive usage of bobs by using bg restore buffer or pristine buffer
- Allow an easy switch of displayed frames for each bob

There are also some caveats:

- Requires interleaved bitmaps for it to work
- Double-buffering is recommended for any reasonable use
- Resizing bobs requires some extra attention
- Adding bob structs must be done outside the game loop, so in dynamic scenarios bob struct pooling might be required

## Pristine Buffer Vs Undraw Buffer

ACE BOBs can work in two ways:

- Undraw Buffer creates two bitmaps of size totalling to all of your initialized BOB dimensions - this scenario is most memory-efficient if you have a limited amount of BOBs, or a large main display buffer (e.g. Simple Buffer with large dimensions).
- Pristine Buffer creates a third buffer for your display buffer, which is used to do the BOB undraw.
  - This is a faster approach since it skips the background save phase
  - It might be more memory-wasteful with small amount of BOBs
  - On the other hand, it might be more efficient for large amount of BOBs.
  - If you need to update your backgrounds/tiles, you are responsible for keeping the Pristine Buffer up to date.

To enable Pristine Buffer, be sure to build ACE with `ACE_BOB_PRISTINE_BUFFER` CMake switch enabled.

## Setting Up Your Game for BOBs

> [!NOTE]
> For non-Tile Buffer games, disable `ACE_BOB_WRAP_Y` CMake switch for better performance.

Before using BOBs, you need to set up a proper display buffer.
BOBs work best with interleaved and double-buffered simplebuffers:

```c
// Create a simple buffer viewport manager with double buffering and interleaved bitmap
s_pVpManager = simpleBufferCreate(0,
   TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
   TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
   // Add other tags as needed
TAG_DONE);
```

> [!NOTE]
> Make sure to export your bitmap resources as interleaved format.

At the end of each frame, you should process viewport managers and wait for vertical blank:

```c
// End of game loop
viewProcessManagers(s_pView);
copProcessBlocks();
vPortWaitForEnd(s_pVp);
```

## Using BOBs

Define some bob structs for later use.
You don't need to use them each frame, so you can define as much as you will need in peak scenarios.

```c
static tBob s_sBobPlayer;
static tBob s_sBobEnemy;
```

In gamestate create, initialize the bob structs, then reallocate buffers for them:

```c
// Create manager with your buffers
bobManagerCreate(s_pVpManager->pFront, s_pVpManager->pBack, uwAvailHeight);

// Initialize each BOB struct
bobInit(
   &s_sBobPlayer, uwWidth, uwHeight, 1,
   bobCalcFrameAddress(s_pBmPlayerFrames, 0),
   bobCalcFrameAddress(s_pBmPlayerMasks, 0),
   uwX, uwY
);
bobInit(
   &s_sBobEnemy, uwWidth, uwHeight, 1,
   bobCalcFrameAddress(s_pBmEnemyFrames, 0),
   bobCalcFrameAddress(s_pBmEnemyMasks, 0),
   uwX, uwY
);
// ... initialize more BOBs as needed

// Allocate draw queues and memory for background saving.
// Must be called after all BOBs are initialized!
bobReallocateBuffers();
```

> [!NOTE]
> If you dare to use single buffering, passing same front/back pointers in `bobManagerCreate()` should work.

In gamestate loop, you need to:

- trigger the undraw,
- add bobs to draw queue,
- ensure that all bobs are drawn

```c
// Begin the BOB drawing cycle - undraws previous BOBs
bobBegin();

// At this point, it's the best time to draw non-bob stuff, e.g. update background.

// Update BOB positions.
// You can safely modify the `sPos` field to update a BOB's position at any time.
// Other fields, especially with underscore prefix, should only be changed
// using the provided functions.
s_sBobPlayer.sPos.uwX = newX;
s_sBobPlayer.sPos.uwY = newY;

// Add BOBs to drawing queue - this will trigger a blitter operation if blitter is idle
bobPush(&s_sBobPlayer);
// ... perform other calculations, e.g. weave-in game logic processing while the blitter is busy
bobPush(&s_sBobEnemy);

// Signal no more BOBs will be added this frame
bobPushingDone();

// Process remaining BOBs (draw them on the back buffer)
bobEnd();
```

> [!CAUTION]
> Don't ever do any blitter operations between first `bobPush()` and calling `bobEnd()` - it will destroy the blitter state expected by the bob system!

> [!CAUTION]
> Drawing anything after `bobEnd()` will likely cause it to be destroyed by the bob undraw.
> It's best to draw your stuff between `bobBegin()` and first `bobPush()`.

> [!NOTE]
> If you do some `bobPush()` close to each other, your blits will be enqueued but they won't automatically trigger after the finishing of the previous one.
> Instead, the next one will try to execute at the end of next `bobPush()` call.
> To process the queue manually, you can weave in `bobProcessNext()` between your calculations.

To clean up the bob system, do folliwing in your gamestate destroy:

```c
bobManagerDestroy();
```

## Animating your BOBs

BOB system expects bitmaps to use single-column bitmaps with frame animations one beneath the other.
The bitmap itself must be of same width as the BOB.

Use `bobSetFrame()` to change the animation frame:

```c
bobSetFrame(
   &s_sBobPlayer,
   bobCalcFrameAddress(s_pBmPlayerFrames, ubFrameIndex * s_sBobPlayer.uwHeight),
   bobCalcFrameAddress(s_pBmPlayerMasks, ubFrameIndex * s_sBobPlayer.uwHeight),
);
```

To speed things up, you should precalculate results of `bobCalcFrameAddress()` and e.g. store it in array indexed by animation frame index, or something.

> [!NOTE]
> Since pixel data and mask are stored separately, you can pass the solid color rectangle frame to your bob to make a common hit effect.

## Resizing your BOBs

To resize the bobs, use `bobSetWidth()` and `bobSetHeight()` functions.
Be sure to call them after `bobBegin()` in your current frame.

> [!CAUTION]
> When using Undraw Buffer mode, make sure that the bob wasn't drawn in frame preceding the resize, since it will trash the undraw operation.
> If you need such feature, please report it in the issue on the ACE repository.
>
> This doesn't affect the BOBs with `isUndrawRequired` set to `0`.

> [!CAUTION]
> When resizing BOBs in Undraw Buffer mode, be sure not to exceed initial bob size used prior to `bobReallocateBuffers()` call.
> Doing so will make the BOB system exceed the background restore buffers, and thus potentially corrupting your game's memory.
>
> This doesn't affect the BOBs with `isUndrawRequired` set to `0`.

## Tile Buffer considerations

- make sure that you're building ACE with `ACE_BOB_WRAP_Y` CMake switch enabled - otherwise, drawing bobs on buffers with large height won't work correctly!
- If you want to "teleport" your camera and/or use `tileBufferRedrawAll()` at any time, you should call `bobDiscardUndraw()` to skip the undraw of your bobs from previous frame.
- You can easily instruct the Tile Buffer to update the Pristine Buffer for you with a tile callback function:

```c
static void onTileDraw(
   UNUSED_ARG UWORD uwTileX, UNUSED_ARG UWORD uwTileY,
   tBitMap *pBitMap, UWORD uwBitMapX, UWORD uwBitMapY
) {
   blitCopyAligned(
      pBitMap, uwBitMapX, uwBitMapY,
      s_pPristineBuffer, uwBitMapX, uwBitMapY, TILE_SIZE, TILE_SIZE
   );
}

// Initialize the Tile Buffer with following:
g_pMainBuffer = tileBufferCreate(0,
   TAG_TILEBUFFER_CALLBACK_TILE_DRAW, onTileDraw,
   // ... other tags here
TAG_END);
```
