
# Converting Audio for ACE

Before using audio in your application, you'll need to convert your audio files using the `audio_conv` tool.

### Command Line Usage

```
audio_conv input.wav [options]
```

### Options
- `-o output.sfx` - Specify output path (default changes extension to .sfx)
- `-n` - Normalize audio amplitude
- `-d N` - Divide amplitude by N
- `-cd N` - Check that amplitude divided by N fits in range
- `-strict` - Treat warnings as errors (recommended)
- `-fpt` - Enforce PTPlayer-friendly mode (adds empty first word if missing)
- `-fpad N` - Force specific byte padding

### CMake Integration

You can automate audio conversion in your build process:

```cmake
convertAudio(
  TARGET your_target
  SOURCE path/to/input.wav
  DESTINATION path/to/output.sfx
  PTPLAYER        # Enable PTPlayer-friendly mode
  NORMALIZE       # Normalize amplitude
  STRICT          # Treat warnings as errors
)
```
