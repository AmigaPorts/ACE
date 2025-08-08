# Palette Conversion Tool

The palette_conv tool allows you to convert between various palette formats for use with the ACE engine.

## Supported Palette Formats

At the time of writing, The `palette_conv` tool supports following formats:

- `.plt` - ACE's native palette format (binary, optimized for Amiga OCS)
- `.gpl` - GIMP Palette format (text) - full spec can be found at https://developer.gimp.org/core/standards/gpl/
- `.act` - Adobe Color Table format
- `.pal` - ProMotion NG palette format
- `.png` - Image preview of the palette - supported only as output format

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

> [!NOTE]
> You can also generate an image with palette preview using `palette_conv palette.gpl preview.png`

## Color Considerations

ACE is primarily designed for the Amiga OCS/ECS hardware, which uses 12-bit color (4 bits per RGB channel).
When converting to ACE's native `.plt` format, the tool ensures that colors are compatible with OCS limitations, throwing errors when that's not the case.

If you want to skip the OCS check and truncate colors to OCS limitations, pass `-cc` ("convert colors") as an extra option after the input path:

```shell
palette_conv palette.gpl palette.plt -cc
```

When creating artwork for your game, you have to:

- Use colors that work within Amiga's 12-bit color limitations (4 bits per channel), e.g. hex code `#112233` but not `#123456`
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

If you want to skip the OCS check and truncate colors to OCS limitations, pass `CONVERT_COLORS` as an extra option:

```cmake
convertPalette(
  ${TARGET_NAME}          # Your target binary
  ${RES_DIR}/palette.gpl  # Source palette file
  ${DATA_DIR}/palette.plt # Destination palette file
  CONVERT_COLORS          # Truncate colors to OCS precision
)
```
