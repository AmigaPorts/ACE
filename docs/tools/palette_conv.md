# Palette Conversion Tool

The palette_conv tool allows you to convert between various palette formats for use with the ACE engine.

## Supported Palette Formats

At the time of writing, The `palette_conv` tool supports following formats:

- `.plt` - ACE's native palette format (binary; **v2** by default — see below)
- `.gpl` - GIMP Palette format (text) - full spec can be found at https://developer.gimp.org/core/standards/gpl/
- `.act` - Adobe Color Table format
- `.pal` - ProMotion NG palette format
- `.png` - Image preview of the palette - supported only as output format

### `.plt` v2 layout (default when writing `.plt`)

The first byte selects encoding; the next two bytes are a **big-endian** 16-bit color count; then the color records.

| First byte | Meaning | Record size |
|------------|---------|-------------|
| `0` | ECS/OCS packed 12-bit | 2 bytes per color |
| `1` | AGA (`0`, R, G, B per entry) | 4 bytes per color |

Older **v1** `.plt` files (first byte **≥ 2**) are not supported; reconvert sources with `palette_conv`.

### Extracting .gpl palette from from Aseprite

You might have to apply addon [Aseprite Script: Amiga OCS/ECS Color Palette Mixer](https://prismaticrealms.itch.io/aseprite-script-amiga-ocsecs-color-palette-mixer) to align png and palette to amiga capacity/rgb colors.

You need to add an addon just process to this installation : https://github.com/behreajj/AsepriteAddons/

Then go to `File > Script > dialogs > palette > gplExport` and put your `.tpl` into your `res` folder.

NOTE: in asesprite you can also save directely in `.act` format.

### Extracting .gpl palette from from GIMP

_To be completed_

## Command Line Usage

To convert palette.gpl to palette.plt, use following command:

```shell
palette_conv palette.gpl palette.plt
```

If no output path is provided, it defaults to converting to `.gpl` format with same file name.

When writing `.plt`, optional flags:

| Flag | Effect |
|------|--------|
| _(none)_ | **v2 ECS/OCS** — sentinel `0`, big-endian count, packed 12-bit colors (input must be valid OCS 12-bit unless `-cc`) |
| `--aga` | **v2 AGA** — sentinel `1`, 4 bytes per color (`0`, R, G, B) |
| `--ocs` | Same as default (explicit ECS/OCS v2) |
| `-cc` | With ECS/OCS output only: **truncate** 8-bit RGB inputs to 12-bit OCS precision (optional escape hatch; default is strict validation so bad colors surface here, not in `bitmap_conv`) |

Examples:

```shell
palette_conv palette.gpl palette.plt
palette_conv palette.gpl palette_aga.plt --aga
palette_conv palette.gpl palette.plt -cc
```

> [!NOTE]
> You can also generate an image with palette preview using `palette_conv palette.gpl preview.png`

## Color Considerations

ACE is primarily designed for the Amiga OCS/ECS hardware, which uses 12-bit color (4 bits per RGB channel).

When converting to ACE's native `.plt` format by default (**ECS/OCS v2**), the tool validates that colors are compatible with OCS limitations, throwing errors when that's not the case—unless you pass **`-cc`**, which quantizes channels to 12-bit instead.

Use **`--aga`** for **AGA v2** `.plt` (full 8-bit RGB channels per color).

When creating artwork for your game, you have to:

- Use colors that work within Amiga's 12-bit color limitations (4 bits per channel), e.g. hex code `#112233` but not `#123456` (when targeting ECS/OCS output)
- Limit your palette to the number of colors supported by your chosen bit depth

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
