# Building ACE

## Building using GNU Make

The following will build `.o` files:

``` sh
cd path/to/ace
make all
```

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
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/AmigaCMakeCrossToolchains/m68k.cmake -DM68K_TOOLCHAIN_PATH=/path/to/toolchain -DM68K_CPU=68020 -DM68K_FPU=soft make
```

If you're on cygwin, you might need to add `-G"Unix Makefiles"`.

After building, you should have `libace.a` in your build folder.
Be sure to link it to your game.
