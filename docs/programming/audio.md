# Working with Audio in ACE

ACE provides audio capabilities through two main subsystems: a basic audio manager and a more advanced ProTracker player (PTPlayer). This document explains how to use these systems in your ACE applications.

## Audio System Overview

ACE offers two audio subsystems:

1. **Basic Audio Manager** - Simple, low-level sample player
   - Provides basic functionality for playing raw audio samples
   - Supports up to 4 channels (Amiga hardware limitation)
   - Marked as deprecated in favor of the PTPlayer module

2. **PTPlayer Module** - Full-featured ProTracker replayer
   - Supports MOD music files (31-sample format)
   - Handles prioritized sound effects playback
   - Offers channel management (reserving channels for music vs. effects)
   - Controls volume at multiple levels
   - **Recommended for most audio needs**

## Converting Audio for ACE

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

## Using the Basic Audio Manager

The basic audio manager provides simple functions for playing samples:

```c
// Initialize audio system
audioCreate();

// Load a sample (8000Hz sample rate)
tSample *pSample = sampleCreateFromPath("sample.raw", 8000);

// Play sample on channel 0 at maximum volume, play once
audioPlay(AUDIO_CHANNEL_0, pSample, AUDIO_VOLUME_MAX, 1);

// Play sample in a loop
audioPlay(AUDIO_CHANNEL_1, pSample, AUDIO_VOLUME_MAX, AUDIO_REPLAY_CONTINUOUS);

// Stop playback
audioStop(AUDIO_CHANNEL_0);

// Clean up
sampleDestroy(pSample);
audioDestroy();
```

## Using the PTPlayer Module (Recommended)

The PTPlayer module provides more advanced features:

### Initialization and Configuration

```c
// Initialize (1 = PAL mode, 0 = NTSC)
ptplayerCreate(1);

// Optional: Configure which channels are reserved for music
ptplayerSetMusicChannelMask(0b0011);  // Reserves channels 0 and 1 for music

// Optional: Set master volume
ptplayerSetMasterVolume(48);  // Range is 0-64

// Optional: Configure song repeat behavior
ptplayerConfigureSongRepeat(1, myCallbackFunction);
```

### Playing MOD Music

```c
// Load a MOD file
tPtplayerMod *pMod = ptplayerModCreateFromPath("music.mod");

// Start playback (NULL for internal samples, 0 for start position)
ptplayerLoadMod(pMod, NULL, 0);

// Enable music playback
ptplayerEnableMusic(1);

// Later, stop music
ptplayerEnableMusic(0);

// When completely done
ptplayerStop();
ptplayerModDestroy(pMod);
```

### Playing Sound Effects

```c
// Load a sound effect
tPtplayerSfx *pSfx = ptplayerSfxCreateFromPath("explosion.sfx", 0);

// Play the effect:
// - on any available channel (PTPLAYER_SFX_CHANNEL_ANY)
// - at maximum volume (PTPLAYER_VOLUME_MAX)
// - with priority 10
ptplayerSfxPlay(pSfx, PTPLAYER_SFX_CHANNEL_ANY, PTPLAYER_VOLUME_MAX, 10);

// Play a looped effect on channel 3
ptplayerSfxPlayLooped(pSfx, 3, PTPLAYER_VOLUME_MAX);

// Stop a looped effect
ptplayerSfxStopOnChannel(3);

// Clean up
ptplayerSfxDestroy(pSfx);
```

### Cleanup

```c
// When done with all audio
ptplayerDestroy();
```

## Amiga-Specific Considerations

1. **Chip Memory Requirements**
   - Samples must be stored in Amiga's chip memory to be played by the hardware
   - PTPlayer sound effects should have an empty first word for stability

2. **Sound Quality**
   - Amiga has 8-bit audio channels (4 channels total)
   - Sample rate calculations are different for PAL vs. NTSC

3. **Channel Management**
   - By default, PTPlayer prioritizes sound effects over music
   - You can reserve channels for music to prevent effects from interrupting
   - Higher priority sound effects will replace lower priority ones if needed

4. **Performance**
   - Sound playback happens via interrupts and doesn't tax the CPU significantly
   - When using PTPlayer, be mindful of how many channels you reserve for music vs. effects

## Advanced Features

- **MOD E8 Command**: PTPlayer supports Protracker's E8 command for synchronizing game events with music. Use `ptplayerGetE8()` to retrieve the last E8 value.

- **Sample Packs**: You can separate MOD files from their sample data using sample packs to save memory when using multiple songs with shared samples.

- **Sample Volume Control**: You can adjust individual sample volumes with `ptplayerSetSampleVolume()` even while music is playing.