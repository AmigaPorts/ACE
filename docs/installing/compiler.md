# Compiler setup

Currently, only VBCC and Bebbo's GCC are supported. The best source for both
is Bebbo's [amiga-gcc](https://github.com/bebbo/amiga-gcc) repository. Its build
system tends to be occasionally broken, so if you can't build it by following
his instructions, don't be shy and drop him an issue. ;)

If you're on windows, you may want to create a shortcut which launches `cmd`
with cygwin & compiler paths in PATH:

`C:\WINDOWS\system32\cmd.exe /c "SET PATH=path/to/bebbo/bin;path/to/cygwin/bin;%PATH%&& set PREFIX=/cygdrive/path/to/bebbo&& START cmd.exe"`

It also sets prefix so that you may (re)build compiler with it easily. Be sure
there are no spaces before `&&` or Cygwin's `rm -rf` may get an empty arg`.

## Prebuilt VBCC for Windows

If you're allergic to building anything and are using Windows, Kusma maintains
prebuilt VBCC on his [amiga-dev](https://github.com/kusma/amiga-dev) repo.

As noted there, you'll need to set up system variables:

- `VBCC`: your `vbcc\targets\m68k-amigaos` directory,
- `PATH`: add your `vbcc\bin` dir.

## Which compiler to choose

Be aware that VBCC is somewhat inferior to GCC in terms of code optimization and
enabling optimizer may result in broken code. On the other hand, Bebbo
implements his own optimizations in GCC code, which may break your code until
they get properly tested in the field. I'm using following ruleset:

- use GCC as main compiler
- if anything breaks mysteriously, disable optimizations in ACE/game makefiles
  (change `-O3` to `-O0`)
- if problem persists, try building with VBCC
- if problem still persists, it's your bug
- if it's not, try to isolate where bug occurs and report it on ACE repo
  **with code sample**
