# Working with and without OS

To squeeze as much performance as possible, ACE allows for disabling Amiga OS and re-enabling it when needed.
Typically, 95% of stuff which game performs doesn't need operating system, so you're safe to have it disabled when your game runs.
Things that require having OS enabled:

- memory allocation and deallocation - most `ace/managers/memory.h` functions
- logging to file - all `ace/managers/log.h` functions if log file is created and when in debug mode
- general file access - all `ace/utils/file.h` and `ace/utils/dir.h` functions
- system .library and .device usage
- multithreading, communicating with other apps

## Manually managing OS state

**By default, ACE leaves OS in enabled state.**
You should disable it using `systemUnuse()` to reach the maximum performance.

The functions which require OS will try to re-enable and disable it on the fly.
Although it's safe approach, it is not optimal, since each OS enabling/disabling takes lots of time (about 2-4 frames, or 40-80ms).
Consider following code:

```c
systemUnuse(); // Assume that OS is disabled at this point

logWrite("I'm initializing stuff\n");
paletteLoadFromPath("palette.plt", s_pPalette, 32);
s_pBm1 = bitmapCreate(320, 256, 5);
s_pBm2 = fontCreate("test.fnt");
logWrite("I'm done initializing\n");
```

This small code wastes about quarter to half a second to do very simple things, because each of this function re-enables OS at start and disables it at its end.
This can be easily fixed by adding `systemUse()` and `systemUnuse()` calls:

```c
systemUse(); // Re-enable OS once
logWrite("I'm initializing stuff\n");       // Any of these functions
paletteLoadFromPath("palette.plt", s_pPalette, 32); // Won't re-enable/disable OS
s_pBm1 = bitmapCreate(320, 256, 5);         // They have systemUse()/systemUnuse calls
s_pBm2 = fontCreate("test.fnt");            // But they won't do anything since OS
logWrite("I'm done initializing\n");        // Is already enabled
systemUnuse(); // Disable OS once when we're done with it
```

## Best practices

Try to have OS disabled at the end of gamestate creation, and to re-enable it at the beginning of gamestate destruction.
This way, the game loop performs with maximum performance, whereas you have OS at your disposal in create/destroy phases.

You can assume that every `create()` and `destroy()` function use OS.
Thus, try to not use them in game loop.
Instead, call them close to each other in state's create/destroy phases and put `systemUse()` / `systemUnuse()` calls around them.

Be careful when using state pushing/popping.
If pushed state needs OS and previous state disabled OS by that point, be sure to have it re-enabled at the beginning of gamestate create phase and disabled at the end of gamestate destroy phase.

## Things that doesn't work when OS is enabled

In future, ACE will be fully usable whether OS is enabled or not.
Currently, this is not the case. Some things will not work when OS is enabled:

- .mod playback and other audio stuff (requires CIA/audio interrupts)
- keyboard handling (requires CIA interrupts)
- any ACE interrupt handlers and features which requires them
- timers (requires vertical blanking interrupt)

## System usage stacking

How does the grouping OS enabling work?
`systemUse()` and `systemUnuse()` use internal OS usage counter: the former increments it, while the latter decreases its value.
The only time the OS gets enabled is when the counter gets to 1, and disabled if it gets to 0.
Consider following code:

```c
// Os is disabled at this point - counter value is 0
systemUse(); // Usage counter gets set to 1 - re-enable OS

// Internal workings of bitmapCreate fn:
systemUse(); // Usage counter increased to 2
logWrite(...); // systemUse/unuse inside fn: Usage counter gets to 3, then to 2
memAllocChip(...); // ditto, os usage 2 -> 3 -> 2
logWrite(...);     // ditto, os usage 2 -> 3 -> 2
systemUnuse(); // Usage counter decreased to 1

systemUnuse(); // Usage counter gets set to 0 - disable OS
```
