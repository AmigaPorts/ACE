
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
