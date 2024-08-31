# Views and viewports

If you did any Amiga development using graphics.library, you'll discover that
ACE builds on same basic concepts but implements it a bit differently.

## Views

Everyting on screen is contained by a single view. You create view using
`viewCreate` function.

You can have multiple views, but only one can be displayed at once. To switch
between views, use `viewLoad` function.

## Viewports

Each view consists of at least one viewport. Viewports are scrollable windows
displaying associated bitmaps. Viewports may have different palettes, resolution
etc. For example, they can be used to split view into main game area and
horizontal HUD bar at top and/or bottom of screen.

The only limitation of viewports is that they have to be placed one beneath
another - they can't be located side by side or one over another. This is
because of Amiga hardware - as data gets sent to display from left to right,
from top to bottom, viewpors are implemented by changing data source,
but it takes so long that it usually needs whole horizontal blanking time
between display lines.

## Viewport managers

A viewport alone doesn't display anything. It needs viewport managers to expand
its features:

- **Simple buffer manager** allocates display bitmap and allows for its
  scrolling. Since it's simple, it works on plain bitmap, which results
  in relatively large memory usage and prevents it from being used in games
  with large scrollable backgrounds: shmup, beat 'em up, RTS, etc.
- **Scroll buffer manager** allocates bitmap only a little larger than
  viewport's dimensions. It wraps bitmap buffer vertically and reserves
  additional buffer lines to facilitate long, but finite horizontal scrolls
  efficiently. Since display bitmap is larger than viewport, it allows
  off-screen drawing just before that portion gets displayed.
- **Tile buffer manager** is derivative of scroll buffer manager. Apart from
  facilitating large scrolls it manages drawing background for you using
  tilemaps. This is what you'll most likely want to use.

## Tutorial code

Now let's expand `src/game.c` with basic view & viewport creation. There'll be
two viewports: one for score, second one for proper playfield. We still won't
display anything on it but we'll get there next time.

View and viewport creation uses tag syntax - it derives from native Amigan style
of coding. The first arg is zero, then it's option-value pair, and then it ends
with `TAG_DONE` or `TAG_END`. You don't need to specify all tags, some are
compulsory. If you omit optional tag, it will result with view/viewport's
default behavior. Be sure to check game.log for errors after running game -
if you omitted something important you'll get an error along with game's crash.
The order of passed tags is irrelevant.

``` c
#include "game.h"
#include <ace/managers/key.h> // Keyboard processing
#include <ace/managers/game.h> // For using gameExit
#include <ace/managers/system.h> // For systemUnuse and systemUse
#include <ace/managers/viewport/simplebuffer.h> // Simple buffer

// All variables outside fns are global - can be accessed in any fn
// Static means here that given var is only for this file, hence 's_' prefix
// You can have many variables with same name in different files and they'll be
// independent as long as they're static
// * means pointer, hence 'p' prefix
static tView *s_pView; // View containing all the viewports
static tVPort *s_pVpScore; // Viewport for score
static tSimpleBufferManager *s_pScoreBuffer;
static tVPort *s_pVpMain; // Viewport for playfield
static tSimpleBufferManager *s_pMainBuffer;

void gameGsCreate(void) {
  // Create a view - first arg is always zero, then it's option-value
  s_pView = viewCreate(0,
    // Same color palette for all viewports, can be skipped as it's set
    // by default to 1.
    TAG_VIEW_GLOBAL_PALETTE, 1,
  TAG_END); // Must always end with TAG_END or synonym: TAG_DONE

  // Viewport for score bar - on top of screen
  s_pVpScore = vPortCreate(0,
    TAG_VPORT_VIEW, s_pView, // Required: specify parent view
    TAG_VPORT_BPP, 4, // Optional: 4 bits per pixel, 16 colors
    TAG_VPORT_HEIGHT, 32, // Optional: let's make it 32px high
  TAG_END); // same syntax as view creation

  // Create simple buffer manager with bitmap exactly as large as viewport
  s_pScoreBuffer = simpleBufferCreate(0,
    TAG_SIMPLEBUFFER_VPORT, s_pVpScore, // Required: parent viewport
    // Optional: buffer bitmap creation flags
    // we'll use them to initially clear the bitmap
    TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
  TAG_END);

  // Now let's do the same for main playfield
  s_pVpMain = vPortCreate(0,
    TAG_VPORT_VIEW, s_pView,
    TAG_VPORT_BPP, 4, // 4 bits per pixel, 16 colors
    // We won't specify height here - viewport will take remaining space.
  TAG_END);
  s_pMainBuffer = simpleBufferCreate(0,
    TAG_SIMPLEBUFFER_VPORT, s_pVpMain, // Required: parent viewport
    TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
  TAG_END);

  // Since we've set up global palette, palette will be loaded from first viewport
  // Colors are 0x0RGB, each channel accepts values from 0 to 15 (0 to F).
  s_pVpScore->pPalette[0] = 0x0000; // First color is also border color
  s_pVpScore->pPalette[1] = 0x0888; // Gray
  s_pVpScore->pPalette[2] = 0x0800; // Red - not max, a bit dark
  s_pVpScore->pPalette[3] = 0x0008; // Blue - same brightness as red

  // We don't need anything from OS anymore
  systemUnuse();

  // Load the view
  viewLoad(s_pView);
}

void gameGsLoop(void) {
  // This will loop forever until you "pop" or change gamestate
  // or close the game
  if(keyCheck(KEY_ESCAPE)) {
    gameExit();
  }
  else {
    // Process loop normally
    // We'll come back here later
  }
}

void gameGsDestroy(void) {
  // Cleanup when leaving this gamestate
  systemUse();

  // This will also destroy all associated viewports and viewport managers
  viewDestroy(s_pView);
}
```
