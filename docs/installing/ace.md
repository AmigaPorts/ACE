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

If you build with CMake you need to use the appropriate CMake Toolchain File.
Start with cloning the AmigaCMakeCrossToolchains repo, then either make a copy of the appropriate cmake68k file and modify it to point to your toolchain location.

```
git clone https://github.com/AmigaPorts/AmigaCMakeCrossToolchains.git toolchains
mkdir -p build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchains/cmake68k-040-hardFPU-libnix ..
make
```

