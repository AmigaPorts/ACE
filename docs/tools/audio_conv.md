# Converting Audio for ACE

Before using sound effects in your game, you'll need to convert your audio files using the `audio_conv` tool.
The simplest usage is:

```shell
audio_conv explode.wav -o explode.sfx
```

> [!CAUTION]
> `audio_conv` you should only use 8-bit signed/unsigned PCM files - although it can resample from 16-bit, it does so very naively and it will yield worse results than resampling indedicated audio software.

Additional options are as follows:

- `-n` - Normalizes audio amplitude, making sure it uses full value range
- `-d N` - Divide amplitude by N - useful for audio mixers
- `-cd N` - Check that amplitude divided by N fits in range - useful for audio mixers
- `-strict` - Treat warnings as errors (recommended)
- `-fpt` - Enforce PTPlayer-friendly mode (adds empty first word if missing)
- `-fpad N` - Force specific byte padding - useful for audio mixers

## CMake Integration

You can automate audio conversion in your build process:

```cmake
convertAudio(
  TARGET your_target
  SOURCE path/to/input.wav
  DESTINATION path/to/output.sfx
  PTPLAYER  # Enable PTPlayer-friendly mode
  NORMALIZE # Normalize amplitude
  STRICT    # Treat warnings as errors
  PAD 2     # Ensure 16-bit alignment
  # For audio mixers:
  DIVIDE_AMPLITUDE 3 # Allows playback of up to 3 samples on same channel without audio glitches
)
```
