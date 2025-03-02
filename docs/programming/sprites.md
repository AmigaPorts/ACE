# Using sprites

## Basic description of sprite hardware

There are eight sprite channels, meaning that Amiga can display data of 8 different sprites on the same line.
Each channel has its own assigned sprite, specified by filling `sprXpt` custom register with pointer to sprite data, where `X` is the channel number.

The sprite data consists of:

- the header consisting of 2 16-bit words and indicating the position of the sprite as well as its height,
- the actual sprite display data,
- the footer consisting of a single 32-bit pointer to the data of next sprite to be shown after displaying the previous one. Such configuration is often described as *sprite list* or *sprite chain*.

On Amiga hardware, each sprite is 16 pixels wide, can be of arbitrary height and is always 2BPP.
With header and footer in mind, this means that e.g. 16x32 sprite data can be prepared as 16x34 2BPP interleaved bitmap.

In order to create wider sprites, you need to use multiple 16px-wide sprites and position them next to each other.
Although AGA chipset supports wider sprites, the support for that feature is still unimplemented.

The final sprite palette is dependent on the channel.
The sprite 2BPP colors are translated into colors from upper half of the current display palette, even if lower screen BPP is used:

| Sprite color index | 0 (always transparent) |  1 |  2 |  3 |
|:------------------:|:----------------------:|:--:|:--:|:--:|
|  channels 0 and 1  |    16 (none)           | 17 | 18 | 19 |
|  channels 2 and 3  |    20 (none)           | 21 | 22 | 23 |
|  channels 4 and 5  |    24 (none)           | 25 | 26 | 27 |
|  channels 6 and 7  |    28 (none)           | 29 | 30 | 31 |

To enable loading sprite data, you must enable sprite-related bit on DMACON register.
This operation enables all sprite channels at once.
In order for Amiga hardware to display meaningful data, you must set up `sprXpt` registers on each VBlank - otherwise the hardware will fetch and display garbage on screen from random location. If you don't intend to use a channel, you must still ensure that its `sprXpt` points to valid data - the best way to not display anything is to point it to an empty two 16-bit words of CHIP memory.

It is possible to attach pairs of sprite channels together to form 16-color sprites - this way the data fetched by one channel controls the lower 2BPP of 4BPP sprites, whereas the other controls the upper half.

It is also possible to skip sprite DMA and load sprite's data directly into designated registers using copper, but it's more advanced topic.
For more elaborate description on sprite hardware, be sure to read other materials, such as Amiga Hardware Reference Manual.

## Using sprite manager

The sprite manager is located in [managers/sprite.h](../../include/ace/managers/sprite.h).
It's principle of work is based on hardware-based linked list of sprites which are set separately for each channel.
The manager works in block, as well as raw copperlist mode.

### Manager setup

```c
// Somewhere in your gamestate creation:
void myGsCreate(void) {
  // Some stuff goes here...

  spriteManagerCreate(s_pView, 0, NULL); // For raw copper mode, pass position on
                                         // copperlist for sprite initialization.
  systemSetDmaBit(DMAB_SPRITE, 1); // Enable sprite DMA.

  // Stuff continues...
}

// Somewhere in your gamestate destruction:
void myGsDestroy(void) {
  // Some stuff goes here...

  systemSetDmaBit(DMAB_SPRITE, 0); // Disable sprite DMA
  spriteManagerDestroy();

  // Stuff continues...
}

// Somewhere in gamestate loop
void myGsLoop(void) {
  // Some stuff goes here...

  // Before you call copProcessBlocks():
  spriteProcessChannel(0); // Process first sprite channel...
  spriteProcessChannel(3); // ...and the fourth one.

  // Stuff continues...

  copProcessBlocks(); // Be sure to call it or sprite-related copper commands won't work!
  vPortWaitForEnd(s_pVp);
}
```

Note that each channel's processing must be called separately.
This allows for further optimization and doing only what is required at the moment.
You can of course call processing of all 8 channels each frame, even if you don't use most of them, but this way you lose a bit of processing time.

### Adding sprites

Amiga sprites use 2BPP interleaved format, so you can load its data using bitmap funcions.
Apart from data, adding sprite requires channel index.

```c
tSprite *s_pSprite0, *s_pSprite3;
tBitMap *s_pSprite0Data, *s_pSprite3Data;

// Somewhere in your gamestate creation:
spriteManagerCreate(...);
// Remember about limited width to 16px AND extra line for header and footer
s_pSprite0Data = bitmapCreate(16, 34, 2, BMF_CLEAR | BMF_INTERLEAVED); // 16x32 2BPP
s_pSprite3Data = bitmapCreate(16, 34, 2, BMF_CLEAR | BMF_INTERLEAVED); // 16x32 2BPP
s_pSprite0 = spriteAdd(0, pSprite0Data); // Add sprite to channel 0
s_pSprite3 = spriteAdd(3, pSprite1Data); // Add second sprite to channel 3
```

> [!NOTE]
> You can't share sprite data bitmaps across channels - although the display data is the same, the header and footer metadata will differ.

Be sure to dispose of them afterwards:

```c
// Somewhere in your gamestate destruction:
bitmapDestroy(s_pSprite0Data);
bitmapDestroy(s_pSprite3Data);
spriteRemove(s_pSprite0);
spriteRemove(s_pSprite3);
spriteManagerDestroy();
```

### Updating sprite

In order for sprite to successfully update its metadata, you need to call `spriteProcess(tSprite *)`.
This function will do almost nothing if no change has been done, so you can call it every frame for each sprite.
When optimizing its calls, you need to call it at least twice (once for each double-buffered copperlist buffer) after significant metadata change has been made, including but not limited to:

- position,
- height,
- sprite bitmap,
- enabling sprite,
- changes in sprite chaining.

### Chaining sprites in a single channel

This is currently not supported.

## Managing sprites in a different way

It is very much possible that you will find ACE's sprite manager's abilities insufficient (e.g. you want to manage your sprite pointers or data using copperlist).
In this case, you should make your own sprite manager.
In order to do this more easily, you may want to use primitives found in [utils/sprite.h](../../include/ace/utils/sprite.h).
