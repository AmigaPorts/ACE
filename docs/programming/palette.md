# Using Palettes

ACE uses its custom `.plt` file format, to which you can convert your palettes from multiple common formats.
See [palette_conv](../tools/palette_conv.md) for details.

By default `palette_conv` writes **v2 ECS/OCS** (`PLT_NEW_ECS`): big-endian **UWORD** colour count, then packed 12-bit colours. Pass **`--aga`** for **v2 AGA** (`PLT_NEW_AGA`, 4 bytes per colour). Older **legacy** `.plt` files (single-byte count first) still load via `paletteLoadFromFd()`.

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

For an **AGA** viewport (`VP_FLAG_AGA`), use a **`ULONG`** palette buffer sized for your bit depth (`1 << bpp`). Pass the same pointer to `paletteLoadFromPath()`; when the file is **PLT_NEW_AGA**, entries are read as in the viewport.

Use **`paletteSave()`** for v2 ECS output, **`paletteSaveAGA()`** (with `ACE_USE_AGA_FEATURES`) for v2 AGA output, and **`paletteSaveLegacy()`** only if you must write the old single-byte-count `.plt` layout.

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
