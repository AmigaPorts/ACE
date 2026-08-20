# Sprite conversion

`sprite_conv` turns a PNG into an ACE hardware-sprite `.bm` (always 2BPP interleaved) and a `.plt`.

If you get stuck, run `sprite_conv` with no args for the switch list.

## How to...

First argument is the palette (`.gpl` / `.plt` / …), second is the PNG. Palette indices 16..31 are treated as sprite `COLOR16..31`.

- 4-color sprite:

  `sprite_conv path/to/pal.gpl path/to/sprite.png -o path/to/sprite.bm`

  Writes `sprite.bm` and `sprite.plt` (the input palette). Use `-p` to pick the `.plt` path, `-np` to skip it, `-aga` for an AGA v2 palette.

- Attached 16-color sprite (two DMA channels):

  `sprite_conv path/to/pal.gpl path/to/sprite.png -attached -o lo.bm hi.bm`
