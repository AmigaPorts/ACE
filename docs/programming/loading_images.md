# Loading images and displaying background

So far, only solid rectangles were used for graphics.
It's time to use images.
This step uses graphics based on [MegaCrash's Breakout Assets](https://megacrash.itch.io/breakout-assets).

## Preparing images and palette

When preparing graphics for your game, ensure that all of them which are to be displayed on same screen, use the same color palette. Some technical points to consider:

- OCS/ECS Amiga's display modes can handle up to 32-color palettes (5 bits per pixel), with EHB and HAM modes being an exception which need some more work.
- The bigger the pixel color depth, the slower the system will work - this is due to Denise chip eating memory access cycles, which could otherwise be used by Copper, Blitter or CPU.
  Consider limiting palette to as little as you can to maintain best performance.
- Amiga blitter as well as display hardware handles graphics in batches of 16 horizontal pixels - ACE itself doesn't work around it, so be sure that all your bitmaps are saved with width equal to multiple of 16.
- At the time of writing, ACE's palette converter can handle following palette formats:
  - GIMP palette file (`.gpl`)
  - Adobe Color Table file (`.act`)
  - ProMotion NG palette file (`.pal`)
- Currently, ACE only supports converting bitmaps from `.png` files.

Download following assets: [Pong background](../../showcase/res/pong_bg.png) and [Pong palette](../../showcase/res/pong.gpl) and put them in folder of your choice - it is a good practice to put resources for conversion in separate folder from source code, e.g. `res` subdirectory.

## Converting images for ACE's use

ACE uses custom file formats: palettes are stored in `.plt` files, whereas for bitmaps `.bm` files are used.
You need to convert your files into those formats and the best way to do this is to add some code to CMake so that they're automatically converted for you, including when they've changed their contents.
To do so, assuming you've [built the conversion tools](../installing/tools.md), add following lines to your CMakeLists.txt file, somewhere after the `add_executable()` line, e.g. at the end of the file:

```cmake
# Convenience variables for source/destination paths
set(RES_DIR ${CMAKE_CURRENT_LIST_DIR}/res)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)

# Create directory for converted files next to built executable
file(MAKE_DIRECTORY ${DATA_DIR})

# Convert palette and background image
convertPalette(${GAME_LINKED} ${RES_DIR}/pong.gpl ${DATA_DIR}/pong.plt)
convertBitmaps(
  TARGET ${GAME_LINKED} PALETTE ${RES_DIR}/pong.gpl
  SOURCES ${RES_DIR}/pong_bg.png
  DESTINATIONS ${DATA_DIR}/pong_bg.bm
)
```

After you've saved and rebuilt your game, you should have `data` folder in your build directory and in it `pong.plt` palette file, as well as `pong_bg.bm` bitmap file for your background.

## Loading palette and images

Now that files are converted, it's time to load them in-game and draw the background.
Replace the old palette-setting code with new one, as well as add the background from loaded tile.

```c
// At the top of the file, near other includes
#include <ace/utils/palette.h>

void gameGsCreate(void) {
  // ...old stuff here...

//-------------------------------------------------------------- NEW STUFF START
  // Load palette from file to first viewport - second one will use the same
  // due to TAG_VIEW_GLOBAL_PALETTE being by default set to 1. We're using
  // 4BPP display, which means max 16 colors.
  paletteLoadFromPath("data/pong.plt", s_pVpScore->pPalette, 16);

  // Load background graphics and draw them immediately
  tBitMap *pBmBackground = bitmapCreateFromPath("data/pong_bg.bm", 0);
  for(UWORD uwX = 0; uwX < s_pMainBuffer->uBfrBounds.uwX; uwX += 16) {
    for(UWORD uwY = 0; uwY < s_pMainBuffer->uBfrBounds.uwY; uwY += 16) {
      blitCopyAligned(pBmBackground, 0, 0, s_pMainBuffer->pBack, uwX, uwY, 16, 16);
    }
  }
  bitmapDestroy(pBmBackground);
//---------------------------------------------------------------- NEW STUFF END

  // Draw line separating score VPort and main VPort, leave one line blank after it
  blitLine(
    s_pScoreBuffer->pBack,
    0, s_pVpScore->uwHeight - 2,
    s_pVpScore->uwWidth - 1, s_pVpScore->uwHeight - 2,
    SCORE_COLOR, 0xFFFF, 0 // Try patterns 0xAAAA, 0xEEEE, etc.
  );

  // NEW: Don't need the line for bottom wall anymore

  systemUnuse();

  // Load the view
  viewLoad(s_pView);
}
```

Functions which operate on files and that are named `somethingLoad()` usually load file contents into already existing places - in this case you're loading colors from palette file directly into palette.
Other files are named `somethingCreate()` and those reserve new memory and load file contents into it for you.
Note that bitmaps and other assets loaded this way must be manually destroyed after they're no longer needed by using `somethingDestroy()` function to free the memory - not doing so will result in memory leak.

With this code, you should now have fine and dandy background for your pong game.
There are some issues, however: the paddles have wrong colors, as well as they erase the background when they move.
We'll fix those issues shortly.
