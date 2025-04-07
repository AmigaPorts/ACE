# Palette Conversion Tool

The palette_conv tool allows you to convert between various palette formats for use with the ACE engine. This document explains how to use the palette_conv tool and how to integrate palette conversion into your build process.

## Supported Palette Formats

The palette_conv tool supports the following formats:

- `.plt` - ACE's native palette format (binary, optimized for Amiga OCS)
- `.gpl` - GIMP Palette format (text)
- `.act` - Adobe Color Table format
- `.pal` - ProMotion NG palette format
- `.png` - Image preview of the palette - only for output

## Command Line Usage

```
palette_conv inPath.ext [outPath.ext]
```

Where:
- `inPath.ext` - Path to the input palette file with its extension
- `outPath.ext` - Path to the output palette file with its extension

If no output path is provided, it defaults to converting to `.gpl` format.

### Example Usage

```
# Converting from GIMP palette to ACE format
palette_conv palette.gpl palette.plt

# Converting from ACE format to Adobe Color Table
palette_conv palette.plt palette.act

# Creating a PNG preview of a palette
palette_conv palette.gpl preview.png
```

## Color Considerations

ACE is designed for the Amiga OCS/ECS hardware, which uses 12-bit color (4 bits per RGB channel). When converting to ACE's native `.plt` format, the tool ensures colors are compatible with OCS limitations.

When creating artwork for your game, it's recommended to:

1. Use colors that work within Amiga's 12-bit color limitations (4 bits per channel)
1. Limit your palette to the number of colors supported by your chosen bit depth

## CMake Integration

You can automate palette conversion in your build process using the `convertPalette` function in your CMakeLists.txt file:

```cmake
convertPalette(
  ${TARGET_NAME}          # Your target binary
  ${RES_DIR}/palette.gpl  # Source palette file 
  ${DATA_DIR}/palette.plt # Destination palette file
)
```

This will automatically convert the palette during the build process and add the resulting file as a dependency to your target.

## Using Palettes in Code

Once you have converted your palette to the ACE format, you can load and use it in your code:

```c
// Define storage for your palette (for 32 colors)
#define GAME_COLOR_COUNT 32
static UWORD s_pPalette[GAME_COLOR_COUNT];

// Load palette from converted file
paletteLoadFromPath("data/palette.plt", s_pPalette, GAME_COLOR_COUNT);

// Apply palette to a viewport
memcpy(s_pViewPort->pPalette, s_pPalette, sizeof(UWORD) * GAME_COLOR_COUNT);
```

### Additional Palette Functions

ACE provides several utility functions for working with palettes:

- `paletteLoadFromPath()` - Loads a palette from a file
- `paletteLoadFromFd()` - Loads a palette from an already open file
- `paletteSave()` - Saves a palette to a file
- `paletteDim()` - Dims a palette to a specified brightness level (for fades)
- `paletteColorDim()` - Dims a single color (0-15 brightness, 15=no dim)

## Using Palettes with Bitmap Conversion

When converting bitmaps, you can specify the palette to use:

```cmake
convertBitmaps(
  TARGET ${TARGET_NAME}
  PALETTE ${RES_DIR}/palette.gpl
  INTERLEAVED
  SOURCES ${RES_DIR}/image.png
  DESTINATIONS ${DATA_DIR}/image.bm
)
```

This ensures that the bitmap uses the correct palette colors during conversion.

## Troubleshooting

### Common Issues

1. **Error: "Invalid input path or palette is empty"**  
   Verify that the input palette file exists and is a valid palette file.

2. **Error: "Input and output extensions are the same"**  
   You must specify different formats for input and output.

3. **Error: "Unsupported output extension"**  
   Check that you're using one of the supported extensions (plt, gpl, act, pal, png).

4. **Colors look different on Amiga**  
   Remember that Amiga uses 12-bit color (4 bits per channel). The conversion tool will adjust colors to fit this limitation.