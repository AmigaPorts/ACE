# Using Palettes

ACE uses its custom `.plt` file format, to which you can convert your palettes from multiple common formats.
See [palette_conv](../tools/palette_conv.md) for details.

## Loading palette into your game

To load your palette in the game, do the following:

```c
// GAME_BPP is your number of color choice - 4 for 16 colors, 5 for 32 colors
#define GAME_BPP 5

static UWORD s_pPalette[1 << GAME_BPP];

void gameGsCreate(void) {
  // ...

  // Load your palette
  paletteLoadFromPath("data/mypalette.plt", s_pPalette, 1 << GAME_BPP);
  // Copy the palette to your viewport.
  // When using `TAG_VIEW_GLOBAL_PALETTE`, only palette in topmost viewport matters.
  memcpy(s_pViewPort->pPalette, s_pPalette, sizeof(s_pVpMain->pPalette));

  // ...

  // If using `TAG_VIEW_GLOBAL_PALETTE`, load the changed palette
  viewUpdateGlobalPalette(s_pView);
}
```

> [!NOTE]
> If you don't need any fancy palette effects, you can load your palette to the `s_pVpMain->pPalette` directly.

## Dimming the palette

You can dim your palette using `paletteDim()` as well as `paletteColorDim()`.
The `ubLevel` parameter accepts values between 0 and 15, where 0 results in fully dimmed color and 15 results in unchanged color.

> [!CAUTION]
> Using previously-dimmed palette/color as a `paletteDim()`/`paletteColorDim()` input will yield degraded color quality.
> Always use pristine palette as a fade source!

> [!NOTE]
> When using `TAG_VIEW_GLOBAL_PALETTE`, store the dimmed result in `s_pVpMain->pPalette` and be sure to call `viewUpdateGlobalPalette()` to update your displayed colors afterwards.
