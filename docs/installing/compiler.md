# Compiler setup

## Bebbo's toolchain

Currently, only VBCC and Bebbo's GCC are supported. The best source for both
is Bebbo's [amiga-gcc](https://github.com/bebbo/amiga-gcc) repository. Its build
system tends to be occasionally broken, so if you can't build it by following
his instructions, don't be shy and drop him an issue. ;)

If you're on Windows, you'll need Cygwin. You may want to create a shortcut
which launches `cmd` with cygwin & compiler paths in PATH:

`C:\WINDOWS\system32\cmd.exe /c "SET PATH=path/to/bebbo/bin;path/to/cygwin/bin;%PATH%&& set PREFIX=/cygdrive/path/to/bebbo&& START cmd.exe"`

Where `path/to/bebbo` is a path to destination where Bebbo's compiler
is installed. On Cygwin, path `X:/dir1/dir2/file` gets changed
to `/cygdrive/x/dir1/dir2/file` so be sure to setup `PREFIX` in proper way.
Before building ACE or Bebbo's compiler, enter `sh` or `bash` or you will
experience strange errors.

Be sure there are no spaces before `&&` or one of `rm -rf` may get an empty arg
and destroy your HDD. :)

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
