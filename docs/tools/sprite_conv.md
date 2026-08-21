# Sprite conversion

`sprite_conv` turns a PNG into an ACE hardware-sprite `.bm` (always 2BPP interleaved).

Use [`palette_conv`](palette_conv.md) / `convertPalette()` when you need a `.plt`.

If you get stuck, run `sprite_conv` with no args for the switch list.

## How to...

First argument is the palette (`.gpl` / `.plt` / …), second is the PNG. Palette indices 16..31 are treated as sprite `COLOR16..31`.

- 4-color sprite:

  `sprite_conv path/to/pal.gpl path/to/sprite.png -o path/to/sprite.bm`

- Attached 16-color sprite (two DMA channels):

  `sprite_conv path/to/pal.gpl path/to/sprite.png -attached -o lo.bm hi.bm`

- Control-word rows:

  `sprite_conv path/to/pal.gpl path/to/sprite.png -pad -o path/to/sprite.bm`

  `-pad` inserts one empty header row and one empty footer row. The sprite manager stores POS/CTL there, so visible height is `bitmap->Rows - 2`. From CMake, pass `PAD` to `convertSprite`.
