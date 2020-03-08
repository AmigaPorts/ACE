# Building ACE

## Building using GNU Make

The following will build `.o` files:

``` sh
cd path/to/ace
make all [OPTIONS]
```

You can build ace by passing several additional options using `OPTION=VALUE`
syntax:

- `ACE_CC` - specify compiler. Currently, supported is `vc` (VBCC) and
  `m68k-amigaos-gcc` (Bebbo's GCC).
- `TARGET` - enable or disable ACE's debug features. Set to `debug` or `release`.

## Building using ACE CLI

@approxit has made his own [simple build tool](https://github.com/approxit/ace-cli/).
To use it, you must have Python installed.

## Building using CMake

Currently CMake build supports only GCC. You need to use the appropriate
CMake Toolchain File. Start with cloning the
[AmigaCMakeCrossToolchains](https://github.com/AmigaPorts/AmigaCMakeCrossToolchains) repo.

You can also set `M68K_CPU` and `M68K_FPU` variables to your liking.

``` sh
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/AmigaCMakeCrossToolchains/m68k.cmake -DM68K_TOOLCHAIN_PATH=/path/to/toolchain -DTOOLCHAIN_PREFIX=m68k-amigaos  -DTOOLCHAIN_PREFIX_DASHED=m68k-amigaos- -DM68K_CPU=68000 -DM68K_FPU=soft
make
```

Some notes:

- You can pass other `-DM68K_CPU` values. Supported are `68000`, `68010`, `68020`, `68040` and `68060`. See AmigaCMakeCrossToolchains docs or sources for more info.
- If you're on cygwin, you might need to add `-G"Unix Makefiles"`.
- If you want to enable debug build (e.g. to have logs and better sanity checks), pass `-DCMAKE_BUILD_TYPE=Debug`.

After building, you should have `libace.a` in your build folder.
Be sure to link it to your game.
