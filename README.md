# ACE - Amiga C Engine

[![Documentation](https://codedocs.xyz/AmigaPorts/ACE.svg)](https://codedocs.xyz/AmigaPorts/ACE/)

Game engine / framework / support library written totally in C for classic Amiga hardware.
Lightweight, flexible and hackable.
ACE uses Amiga hardware features directly, aiming for as much speed as possible.
The code is OS-friendly, allowing running from and exiting to Workbench gracefully.

Current feature set is OCS-oriented, although produced code should work on AGA just fine.

For build instructions basic how to and contributing guidelines, refer to [docs](docs/README.md).

## Why I should use ACE?

ACE is best if you want to:

- Learn how to code on Amiga - the only skill you need is familiarity with C language.
  ACE Provides very thin hardware abstraction layer, which aims to be as documented as possible.
  **Lack of documentation is treated as a bug**, so feel free to report it in an issue!
- Code a high performance game/demo but don't want to do it entirely in assembly.
  Use stock ACE functions to make the code work, trim it down for maximum performance.
  Or just write the assembly by hand for parts which really require it.

## What ACE does

- Provides slim HAL for audio, blitter, copper, keyboard and mouse.
- Provides view and viewport system which work in similar manner to Amiga OS, but in less bloated way.
- Has lots of runtime error checks when building in debug mode, which are completely gone in release builds.
- Disables OS for maximum performance, but allows re-enabling it when needed - e.g. for file access, memory allocation, etc.
- Provides optional framework which helps in organizing games and reducing boilerplate code.
- Provides code for scrolling large tilemaps in memory-efficient way.
- Requires only kickstart 1.3, but provides OS2.0+ features, like functions using taglists, interleaved bitplane modes, etc.

## Things you won't find in ACE

- A Game Maker experience - you need to code game logic and most of the engine yourself.
- Elaborate sprite management.
- Functions for direct data read from floppy sectors.

Most of those features were deliberately omitted because they can be done in very different ways depending on the game/demo.
Still, the list of open source games is increasing, so you can copy and modify most building blocks from them to suit your needs.

## Games created using ACE

In alphabetical order:

- [ami-invaders](https://github.com/approxit/amiga-invaders)
- [AMIner](https://github.com/Last-Minute-Creations/AMIner)
- [Atarenium Falcon](https://github.com/Last-Minute-Creations/AtareniumFalcon)
- [Battle Squadron 2](https://github.com/bitwise-brains/AGJ2024_BS2)
- CastleHack
- [Chaos Arena](https://github.com/Last-Minute-Creations/chaosArena)
- [Flappy Ace](https://github.com/NZjeux26/FlappyAceWin)
- [GermZ](https://github.com/Last-Minute-Creations/germz)
- [Goblin Villages](https://github.com/Last-Minute-Creations/goblin-villages)
- [Impsbru](https://github.com/approxit/impsbru)
- [OpenFire](https://github.com/Last-Minute-Creations/openFire)
- [Squared](https://github.com/tehKaiN/ld40-squared)

## Demos created using ACE

In alphabetical order:

- [Casentino Day 2020 invitation](https://github.com/Ozzyboshi/Casentinoday2020AmigaDemo/)
- [Starfield](https://github.com/Ozzyboshi/AmigaStarfield)

## License, 3rd party code

Unless otherwise stated, the code is licensed under [Mozilla Public License 2.0](LICENSE). The exceptions are:

- The ptplayer module is based on [ptplayer](http://aminet.net/package/mus/play/ptplayer.lha) by Frank Wille (Public Domain)
- the mini_std library is based on 3rd party code:
  - [mpaland/printf](https://github.com/mpaland/printf) (MIT)
  - [strtoul](https://github.com/diegocr/libnix/blob/master/stdlib/strtoul.c) from Libnix (Public Domain)
  - [DeidaraC/ctype](https://github.com/DeidaraC/ctype.h/blob/master/ctype.c) (MIT)
  - `qsort()` replacement: [noporpoise/sort_r](https://github.com/noporpoise/sort_r) (Public Domain)
