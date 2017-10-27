# ACE - Amiga C Engine

Game engine written totally in C for classic Amiga hardware. Lightweight, flexible and hackable. Coded mostly for bare HW, OS function usage is receeding. Nonetheless code is OS-friendly, allowing running from and exiting to Workbench gracefully.

Current feature set is OCS-oriented, although produced code should work on AGA just fine.

## Installing

Currently only VBCC on Amiga/Windows is supported. More compilers and OSes will
follow.

### Building using GNU Make

#### Windows

You will need to build VBCC from sources or get pre-built package from
[amiga-dev@github](https://github.com/kusma/amiga-dev). As noted there,
you'll need to set up system variables:
* VBCC: your vbcc\targets\m68k-amigaos directory,
* PATH: add your vbcc\bin dir.

ACE Uses GNU/Make. If you don't have one, be sure to install it via mingw
or something. If you already own Code::Blocks, you can simply add your
CodeBlocks\MinGW\bin dir to PATH.

If your make throws system errors, try forcing use of cmd.exe instead of sh:
`make SHELL=cmd.exe`

#### Amiga

Get yourself VBCC and GNU/Make. The second one typically requires
ixemul.library, which can be downloaded from Aminet.

### Building using ACE CLI

Proxy have made his own simple build tool. To use it, you must have Python installed.

## Contributing

As engine is during cleanup and overhaul related to public release, code is
expected to change drastically. Until then, suggestions & merge requests will be rarely accepted.

## Games created using ACE

- CastleHack
- Goblin Villages
- [OpenFire](https://github.com/tehKaiN/openFire)
- [Impsbru](https://github.com/approxit/impsbru)
