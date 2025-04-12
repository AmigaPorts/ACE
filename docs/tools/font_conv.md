# Converting Fonts for ACE

The `font_conv` tool allows you to convert common font files to ACE's custom `.fnt` file format.

To convert .ttf file to output.fnt file, with given glyph set, do the following:

```shell
font_conv input.ttf fnt -out output.fnt -size 8 -chars "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
```

The second parameter specifies the file format of the output.
At the time of writing, the `font_conv` tool supports following formats:

- ttf - TrueType Fonts (.ttf)
- dir - Directory with separate PNG glyphs
- png - Single PNG file with whole font
- fnt - ACE .fnt file
- pmng - PNG fonts from ProMotion NG

Unless otherwise stated, you are free to convert between any of them.

> [!NOTE]
> It is not recommended to convert from TTF directly to .fnt as automatic rasterization and determining margins might need some manual cleanup.

Extra options are:

- `-chars` - Specifies characters to include in the font
- `-charfile` - Use characters from a text file
- `-size` - Set font size for rasterization (for TTF fonts)
- `-out` - Specify output path
- `-fc` - Set first character index (for ProMotion NG fonts)

## CMake Integration

You can automate font conversion in your build process using the `convertFont` function in your CMakeLists.txt file:

```cmake
convertFont(
  TARGET your_target
  SOURCE path/to/source_font.ttf
  DESTINATION path/to/output.fnt
  FIRST_CHAR 33 # Equivalent to -fc param
)
```

The `convertFont` function will automatically convert the font during the build process and add the resulting file as a dependency to your target.
