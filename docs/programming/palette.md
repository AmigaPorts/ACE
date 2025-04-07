# Using Palettes

## Loading Palettes in ACE

ACE currently use **.gpl (Gimp Palette)** as palette input. [Full spec here](https://developer.gimp.org/core/standards/gpl/).

To be complete, at the time of writing, ACE's palette converter can handle following palette formats:
  - GIMP palette file (`.gpl`)
  - Adobe Color Table file (`.act`)
  - ProMotion NG palette file (`.pal`)

Because **.gpl** is from the opensource world, this article will focus on it.

## Extracting .gpl palette from your tools.

### From Aseprite

You might have to apply addon [Aseprite Script: Amiga OCS/ECS Color Palette Mixer](https://prismaticrealms.itch.io/aseprite-script-amiga-ocsecs-color-palette-mixer) to align png and palette to amiga capacity/rgb colors.

You need to add an addon just process to this installation : https://github.com/behreajj/AsepriteAddons/

Then go to `File > Script > dialogs > palette > gplExport` and put your `.tpl` into your `res` folder.

NOTE: in asesprite you can also save directely in `.act` format.

### From GIMP

_To be completed_

## Loading palette into your game

You need to append your `CMakeLists.txt` file and add at the end. It loads and converts the palette and then use it to convert your graphic file.

```cmake
# Convert palette and background image
convertPalette(${GAME_LINKED} ${RES_DIR}/mypalette.gpl ${DATA_DIR}/mypalette.plt)
convertBitmaps(
  TARGET ${GAME_LINKED} PALETTE ${RES_DIR}/mypalette.gpl
  INTERLEAVED SOURCES ${RES_DIR}/YourTiles.png
  DESTINATIONS ${DATA_DIR}/YourTiles.bm
```

Then in your code :

```c
// GAME_BPP is your number of color choice - 4 for 16 colors, 5 for 32 colors
#define GAME_BPP 5
// declare palette G
static UWORD s_pPalette[1 << GAME_BPP];
// [...]
void gameGsCreate(void) {
    // [...]
    // Load your palette
    paletteLoadFromPath("data/mypalette.plt", s_pPalette, 1 << GAME_BPP);
    // One way to apply palette, copy it to the 2 view ports
    memcpy(s_pVpScore->pPalette, s_pPalette, sizeof(s_pVpScore->pPalette));
    memcpy(s_pVpMain->pPalette, s_pPalette, sizeof(s_pVpMain->pPalette));
    // [...]
}
// [...]
```


### Additional Palette Functions

ACE provides several utility functions for working with palettes:

- `paletteLoadFromPath()` - Loads a palette from a file
- `paletteLoadFromFd()` - Loads a palette from an already open file
- `paletteSave()` - Saves a palette to a file
- `paletteDim()` - Dims a palette to a specified brightness level (for fades)
- `paletteColorDim()` - Dims a single color (0-15 brightness, 15=no dim)

