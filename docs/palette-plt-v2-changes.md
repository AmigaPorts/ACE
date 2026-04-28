# `.plt` v2, AGA alignment, and `palette_conv` defaults

This document summarises the palette / `.plt` work: what changed, and why.

## Why change the format?

**Legacy `.plt`** used a **single byte** for the colour count, then chose **ECS (2 bytes per colour)** vs **AGA-style (4 bytes: `0`, R, G, B)** using **`count > 32`**. That breaks a real case: a **32-colour AGA** palette is **not** `> 32`, so the loader treated it as **ECS** and read the wrong record size.

**v2** fixes that by **not** inferring encoding from the count:

- First byte: **`PLT_NEW_ECS` (0)** = packed 12-bit ECS/OCS entries, or **`PLT_NEW_AGA` (1)** = 4-byte AGA entries.
- Next two bytes: **big-endian `UWORD`** `num_colours`.
- Then the colour payload for that mode.

**Legacy (v1)** files (first byte **≥ 2**) are **not** loaded or written by current ACE or `palette_conv`; reconvert assets to **v2**.

## Why the leading `0` in AGA file entries?

The 4-byte AGA record is the same as before: **alpha (0) + R, G, B**. It matches **bulk `ULONG` loads** in `paletteLoadFromFd` and the in-memory **AGA viewport** layout. Dropping to 3 bytes on disk would need a new format version and pack/unpack; the small size win is not worth the API churn for typical 2 MiB chip-RAM targets.

## Runtime (Amiga)

- **`paletteLoadFromPath` / `paletteLoadFromFd`**: **v2** only (ECS or AGA sentinel); legacy v1 returns without loading (`ACE_DEBUG` logs an error). AGA v2 loads into a buffer you use as **`ULONG` per entry** for AGA viewports.
- **`paletteSave`**: writes **v2 ECS** (`0` + count + `UWORD` colours).
- **`paletteSaveAGA`**: writes **v2 AGA** (with `ACE_USE_AGA_FEATURES`).
- **`paletteDimAGA`**: off-by-one in the loop fixed.
- **`paletteColorMixAGA`**, **`paletteDumpAGA`**: implemented (were only declared before).

## Tools

- **`tPalette::toPlt` / `fromPlt`**: **v2** read/write only.
- **`palette_conv` default output** (no flags): **v2 ECS/OCS** so it matches the **OCS/ECS-first** use of ACE and validates 12-bit colours. Use **`--aga`** for **v2 AGA** `.plt`.

## Docs

- See [tools/palette_conv.md](tools/palette_conv.md) and [programming/palette.md](programming/palette.md) for end-user details.

## Integration note (CMake / build)

`convertPalette` in `cmake/ace_functions.cmake` runs `palette_conv` with **no extra flags**, so it now produces **v2 ECS** `.plt` by default. For **AGA-only** palettes, invoke `palette_conv` with **`--aga`** (extend your CMake helper if needed).
