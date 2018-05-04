# Building ACE

Currently, only VBCC and Bebbo's GCC are supported.

## Building using GNU Make

### Windows

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

### Amiga

Get yourself VBCC and GNU/Make. The second one typically requires
ixemul.library, which can be downloaded from Aminet.

## Building using ACE CLI

@approxit has made his own simple build tool. To use it, you must have Python installed.
