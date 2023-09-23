# Building ACE tools

ACE comes bundled with its own tools for converting file formats.
Since those apps run on your development machine and not the Amiga itself, to build them, you'll need a compiler for your operating system.

- On Windows, you need recent version of either the MinGW GCC compiler toolchain (e.g. [winlibs-x86_64-posix-seh-gcc-12.2.0-llvm-15.0.7-mingw-w64ucrt-10.0.0-r4](https://github.com/brechtsanders/winlibs_mingw/releases/download/12.2.0-15.0.7-10.0.0-ucrt-r4/winlibs-x86_64-posix-seh-gcc-12.2.0-llvm-15.0.7-mingw-w64ucrt-10.0.0-r4.7z)) or MSVC.
- On Linux, using GCC is strongly recommended.
- Using Clang on any system is not tested.

To build your tools, open the terminal, navigate to ACE's `tools` directory, and issue following commands:

```shell
mkdir build && cd build

# When using MacOS (to avoid Clang) - assuming you have: brew install gcc@13
export CC=/usr/local/bin/gcc-13
export CXX=/usr/local/bin/g++-13

# When using GCC on Linux, MacOS or MSVC on Windows:
cmake ..
# When using MinGW GCC on Windows:
cmake .. -G "MinGW Makefiles"

cmake --build .
```

When done successfully, you should now have `tools/bin` directory with ACE tool executables.
If you're stuck by issuing wrong commands, navigate out of build folder, delete it and try again.
