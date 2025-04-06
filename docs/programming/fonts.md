# Working with Fonts

ACE provides a font system that allows you to display text in your games with various formatting options. This document explains how to use fonts in your ACE applications.

## Overview

Fonts in ACE are stored as 1-bit bitmap data with each glyph placed side by side in a single continuous bitmap. The system supports:

- Variable width characters
- Text alignment (left, right, center)
- Text effects (shadows, cookies)
- Efficient memory usage with pre-rendered text bitmaps

## Converting Fonts for ACE

Before using fonts in your application, you'll need to convert them to ACE's `.fnt` format using the `font_conv` tool.

### Input Font Formats

The `font_conv` tool supports converting from:
- TrueType Fonts (TTF)
- PNG fonts from ProMotion NG
- Directory with separate PNG glyphs
- Existing ACE .fnt files

### Command Line Usage

```
font_conv input.ttf fnt -out output.fnt -size 8 -chars "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
```

### Options
- `-chars` - Specify characters to include in the font
- `-charfile` - Use characters from a text file
- `-size` - Set font size (for TTF fonts)
- `-out` - Specify output path
- `-fc` - Set first character index (for ProMotion NG fonts)

## CMake Integration

You can automate font conversion in your build process using the `convertFont` function in your CMakeLists.txt file:

```cmake
convertFont(
  TARGET your_target
  SOURCE path/to/source_font.ttf
  DESTINATION path/to/output.fnt
  FIRST_CHAR 33
)
```

The `convertFont` function will automatically convert the font during the build process and add the resulting file as a dependency to your target.

## Using Fonts in Code

### Loading Fonts

```c
// Load a font from a file
tFont *pFont = fontCreateFromPath("data/fonts/your_font.fnt");
```

### Creating Text Bitmap Buffers

For optimal performance, it's recommended to create reusable text bitmap buffers:

```c
// Create a text bitmap buffer with specified dimensions
tTextBitMap *pTextBitmap = fontCreateTextBitMap(96, pFont->uwHeight);
```

### Drawing Text

```c
// Draw text directly with formatting
fontDrawStr(
  pFont,            // The font to use
  pBuffer->pBack,   // Destination bitmap
  uwX, uwY,         // Position coordinates
  "Hello, Amiga!",  // Text to display
  ubColor,          // Color index to use
  FONT_CENTER,      // Formatting flags
  pTextBitmap       // Reusable text bitmap buffer
);
```

### Text Formatting Flags

You can combine these flags to achieve different text formatting:

- Horizontal alignment: `FONT_LEFT`, `FONT_RIGHT`, `FONT_HCENTER`
- Vertical alignment: `FONT_TOP`, `FONT_BOTTOM`, `FONT_VCENTER`
- Combined alignment: `FONT_CENTER` (horizontally and vertically centered)
- Effects: `FONT_SHADOW` (draw with shadow), `FONT_COOKIE` (text with outline)

### Measuring Text

```c
// Get the dimensions required for displaying text
tUwCoordYX textSize = fontMeasureText(pFont, "Hello, Amiga!");
// textSize.uwX contains the width, textSize.uwY contains the height
```

### Cleanup

Always free resources when they're no longer needed:

```c
// Destroy text bitmap buffer
fontDestroyTextBitMap(pTextBitmap);

// Destroy font
fontDestroy(pFont);
```

## Performance Tips

1. Pre-render text that doesn't change frequently:
   ```c
   // Create a bitmap containing the text
   tTextBitMap *pStaticText = fontCreateTextBitMapFromStr(pFont, "Game Over");
   
   // Draw the pre-rendered text repeatedly
   fontDrawTextBitMap(pBuffer->pBack, pStaticText, uwX, uwY, ubColor, ubFlags);
   ```

2. Reuse text bitmap buffers for dynamic text:
   ```c
   tTextBitMap *pScoreText = fontCreateTextBitMap(64, pFont->uwHeight);
   
   // Later in game loop
   sprintf(szBuffer, "SCORE: %d", uwScore);
   fontFillTextBitMap(pFont, pScoreText, szBuffer);
   fontDrawTextBitMap(pBuffer->pBack, pScoreText, uwX, uwY, ubColor, ubFlags);
   ```

3. Only redraw text when it changes to minimize blitter operations