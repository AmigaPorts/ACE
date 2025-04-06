# Working with BOBs (Blitter Objects)

ACE provides a powerful system for managing and animating 2D sprites using the Amiga's hardware blitter through its BOB (Blitter OBject) system. This document explains how to use BOBs effectively in your ACE applications.

## Setting Up Your Display

Before using BOBs, you need to set up a proper display buffer. BOBs work best with interleaved and double-buffered simplebuffers:

```c
// Create a simple buffer viewport manager with double buffering and interleaved bitmap
s_pVpManager = simpleBufferCreate(0,
    TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
    TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
    // Add other tags as needed
    TAG_DONE);
```

**Important**: 
- Always use interleaved bitmaps (`BMF_INTERLEAVED`) for best performance
- Double buffering (`TAG_SIMPLEBUFFER_IS_DBLBUF`) is essential for smooth animation
- Make sure to export your bitmap resources as interleaved format

At the end of each frame, you should process viewport managers and wait for vertical blank:

```c
// End of game loop
viewProcessManagers(s_pView);
copProcessBlocks();
vPortWaitForEnd(s_pVp);
```

## What are BOBs?

BOBs are 2D graphic elements that can be efficiently moved around the screen using the Amiga's hardware blitter. They offer several advantages:

- Background preservation for clean undrawing
- Transparency support via masks
- Animation capabilities by switching frames
- Efficient memory usage and blitter operations
- Support for double-buffering for smooth animation

## Basic BOB Workflow

The typical workflow for using BOBs is:

1. **Initialization** (in gamestate create):
```c
// Create manager with your buffers
bobManagerCreate(pFrontBuffer, pBackBuffer, uwAvailHeight);

// Initialize each BOB
bobInit(&sBob1, uwWidth, uwHeight, 1, pFrameData, pMaskData, uwX, uwY);
bobInit(&sBob2, uwWidth, uwHeight, 1, pFrameData, pMaskData, uwX, uwY);
// ... initialize more BOBs as needed

// Allocate memory for background saving (must be called after all BOBs are initialized)
bobReallocateBuffers();
```

2. **Game Loop** (in gamestate loop):
```c
// Begin the BOB drawing cycle - undraws previous BOBs
bobBegin();

// Update BOB positions
sBob1.sPos.uwX = newX;
sBob1.sPos.uwY = newY;

// Add BOBs to drawing queue
bobPush(&sBob1);
// ... perform other calculations
bobPush(&sBob2);

// Process some BOBs (save backgrounds) while doing other calculations
bobProcessNext();

// Signal no more BOBs will be added this frame
bobPushingDone();

// Process remaining BOBs (draw them)
while(bobProcessNext()) {}
// Or simply call bobEnd() to finish all processing
bobEnd();
```

3. **Cleanup** (in gamestate destroy):
```c
bobManagerDestroy();
```

## Important BOB Functions

### Initialization and Setup
- `bobManagerCreate()`: Initializes the BOB system with front/back buffers
- `bobInit()`: Sets up a BOB with its dimensions, position, and frame data
- `bobReallocateBuffers()`: Allocates memory for background saving

### Animation and Dimensions
- `bobSetFrame()`: Changes a BOB's animation frame
- `bobSetWidth()`: Modifies a BOB's width (be careful with memory allocation)
- `bobSetHeight()`: Modifies a BOB's height (be careful with memory allocation)
- `bobCalcFrameAddress()`: Helps calculate the memory address of a specific animation frame

### Drawing Cycle
- `bobBegin()`: Starts a new drawing cycle, undraws previous BOBs
- `bobPush()`: Adds a BOB to the drawing queue
- `bobProcessNext()`: Processes the next BOB operation (background save or drawing)
- `bobPushingDone()`: Signals no more BOBs will be added this frame
- `bobEnd()`: Completes all drawing operations
- `bobDiscardUndraw()`: Skips undrawing (useful for first frame)

## Tips and Best Practices

1. **Memory Management**:
   - For BOBs with background preservation (isUndrawRequired=1), the initial width/height should be set to the maximum anticipated size.
   - Changing width/height to larger than initial size can cause memory corruption.

2. **Performance Optimization**:
   - Interleave blitter operations with CPU work by calling `bobProcessNext()` periodically.
   - Don't use the blitter for other operations between `bobBegin()` and `bobEnd()`.
   - For non-scrolling games, consider disabling Y-wrapping for better performance.

3. **Animation**:
   - Store animation frames sequentially in memory for easy frame changes.
   - Use `bobCalcFrameAddress()` to find the correct memory address for each frame.

4. **Double Buffering**:
   - The BOB system is designed to work with double-buffered displays.
   - You can use a single buffer by passing the same pointer for front and back buffers.

5. **First Frame Handling**:
   - Call `bobDiscardUndraw()` on the first frame to avoid undrawing BOBs that weren't previously drawn.

## The BOB Structure

```c
typedef struct tBob {
    UBYTE *pFrameData;      // Current frame bitmap data
    UBYTE *pMaskData;       // Transparency mask data
    tUwCoordYX pOldPositions[2]; // Previous positions for undrawing
    tUwCoordYX sPos;        // Current position (x,y)
    UWORD uwWidth;          // BOB width
    UWORD uwHeight;         // BOB height
    UBYTE isUndrawRequired; // If 1, background is preserved and restored
    // Private fields follow...
} tBob;
```

You can safely modify the `sPos` field to update a BOB's position. Other fields should only be changed using the provided functions.