# Working with Fonts

ACE provides a font system that allows you to display text in your games with various formatting options.

Fonts in ACE are stored as 1-bit bitmap data with each glyph placed side by side in a single continuous bitmap.
The system supports:

- Variable width characters
- Text alignment (left, right, center)
- Text effects (shadows, cookies)
- Efficient memory usage with pre-rendered text bitmaps
- **Single-color texts only**

> [!NOTE]
> For multi-color texts, consider rolling your own font system.

## Using Fonts in Code

First, convert the font from the format of your choice using the [font_conv](../tools/font_conv.md).
Then, Load your font and create the intermediate Text Bitmap for storing texts composed from separate glyphs:

```c
static tFont *s_pFont;
static tTextBitMap *s_pTextBitmap;

// In gamestate create:
s_pFont = fontCreateFromPath("data/fonts/your_font.fnt");
s_pTextBitmap = fontCreateTextBitMap(96, s_pFont->uwHeight);
```

> [!CAUTION]
> Be sure to create the Text Bitmap with size which can fit the longest text possible in your game.
>
> Because of how blitter works, increase the X size by 16.

> [!NOTE]
> To conserve memory, re-use a single Text Bitmap for drawing different texts.

To draw the text, do as follows:

```c
// Draw text directly with formatting
fontDrawStr(
  s_pFont,                // The font to use
  s_pSimpleBuffer->pBack, // Destination bitmap
  uwX, uwY,               // Position coordinates
  "Hello, Amiga!",        // Text to display
  ubColor,                // Color index to use
  FONT_CENTER,            // Formatting flags
  s_pTextBitmap             // Reusable Text Bitmap
);
```

You can combine following flags with `|` to achieve different text formatting:

- Horizontal alignment: `FONT_LEFT`, `FONT_RIGHT`, `FONT_HCENTER`
- Vertical alignment: `FONT_TOP`, `FONT_BOTTOM`, `FONT_VCENTER`
- Combined alignment: `FONT_CENTER` (horizontally and vertically centered)
- Effects: `FONT_SHADOW` (draw with shadow), `FONT_COOKIE` (text with outline)
- Lazy draw: `FONT_LAZY`

You might want to measure your text at some occasions, e.g. to allocate Text Bitmap, or to draw the UI closely matching the text.
Use `fontMeasureText()` for that.

## Cleanup

Always free resources when they're no longer needed:

```c
// Destroy text bitmap buffer
fontDestroyTextBitMap(s_pTextBitmap);

// Destroy font
fontDestroy(s_pFont);
```

## Performance Tips

1. Pre-render text that doesn't change frequently in dedicated Text Bitmap:

   ```c
   // Create a bitmap containing the text
   tTextBitMap *s_pStaticText = fontCreateTextBitMapFromStr(s_pFont, "Game Over");
   // Or lazily update its contents with fontFillTextBitMap()

   // Later on, draw the pre-rendered text repeatedly
   fontDrawTextBitMap(pBuffer->pBack, pStaticText, uwX, uwY, ubColor, ubFlags);
   ```

2. To minimize blitter operations, only redraw text when it changes.
3. If you're doing the HUD draws, consider splitting the draws of each part to separate frame.
 It will usually be fast enough and the delay will probably be barely noticable by the player.
