# ACE - Amiga C Engine

Initial stage - don't try to understand it yet. ;)

## Installing

Currently only VBCC on Amiga/Windows is supported. More compilers and OSes will
follow.

### Windows

You will need to build VBCC from sources or get pre-build package from
[amiga-dev@github](https://github.com/kusma/amiga-dev). As noted there,
you'll need to set up system variables:
* VBCC: your vbcc\targets\m68k-amigaos directory,
* PATH: add your vbcc\bin dir.
	
ACE Uses GNU/Make. If you don't have one, be sure to install it via mingw
or something. If you already own Code::Blocks, you can simply add your
CodeBlocks\MinGW\bin dir to PATH.

### Amiga

Get yourself VBCC and GNU/Make. The second one typically requires
ixemul.library, which can be downloaded from Aminet.

## Contributing

As engine is during cleanup and overhaul related to public release, code is
expected to change drastically. Before then, don't contribute to it. Forks
and suggestions are welcome, though.