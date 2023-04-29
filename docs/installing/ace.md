# Building ACE

Currently, only CMake is supported.

## Building using CMake

Currently, CMake build supports only GCC (be it Bebbo's or Bartman's version).
You need to use the appropriate
CMake Toolchain File.
Start with cloning the [AmigaCMakeCrossToolchains](https://github.com/AmigaPorts/AmigaCMakeCrossToolchains) repo to anywhere on your computer.

```shell
git clone https://github.com/AmigaPorts/AmigaCMakeCrossToolchains
```

Notably, this toolchain file allows you to set `M68K_CPU` and `M68K_FPU` variables to your liking, allowing you to optimize code for FPU and/or specific CPU version.
By default, for most compatibility, 68000 and soft-FPU is used.

### Using ACE as submodule dependency in your project

This is the recommended way of building ACE.
Since ACE's development often breaks things, it's best to attach ACE as a submodule in your game's repository.
This way, your commit history will store the last-known good ACE commit which worked well with your project.

To attach ACE repository as a submodule, do the following in your repo's root directory:

```sh
mkdir deps
git submodule add https://github.com/AmigaPorts/ACE deps/ace
```

And, if you already have CMake project set up, link it to your main executable by adding in CMakeLists.txt:

```cmake
add_subdirectory(deps/ace)
target_link_libraries(myGame ace)
```

### Building standalone ACE library

``` sh
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/AmigaCMakeCrossToolchains/m68k.cmake -DM68K_TOOLCHAIN_PATH=/path/to/toolchain -DM68K_CPU=68000 -DM68K_FPU=soft
make
```

Some notes:

- You can pass other `-DM68K_CPU` values. Supported are `68000`, `68010`, `68020`, `68040` and `68060`. See AmigaCMakeCrossToolchains docs or sources for more info.
- If you're on cygwin, you might need to add `-G "Unix Makefiles"`.
- If you want to enable debug build (e.g. to have logs and better sanity checks), pass `-DCMAKE_BUILD_TYPE=Debug`.
- If you really want to depend on standalone-built ACE library, be sure to take note of the commit you've built it from.
  ACE breaks things very often and it's almost certain that after some time you won't be able to build your game with latest ACE version.
- By default, ACE is built as a bunch of .o files which are then linked to your executable using powers of CMake.
  This allows for better link-time optimization.
  If that's not what you need, add `-DACE_BUILD_KIND=STATIC` to produce `libace.a` for classic link scenarios.

After building, you should have a bunch of `.o` files or `libace.a` in your build folder.
Be sure to link it to your game.
