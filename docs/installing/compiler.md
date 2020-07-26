# Compiler setup

Currently, only Bartman's and Bebbo's GCC are supported.

## Bartman's toolchain

Bartman has created excellent extension for Visual Studio Code, which comes with latest GCC version and integrated debugging and profiling features.
To download it, in VSCode go to Extensions page and install `bartmanabyss.amiga-debug` extension.

## Bebbo's toolchain

You can get Bebbo's compiler from his [amiga-gcc](https://github.com/bebbo/amiga-gcc) repository.
You can download precompiled libraries (May require MSYS2 installed) or build it from source.
Its build system tends to be occasionally broken, so if you can't build it by following his instructions, don't be shy and drop him an issue. ;)
Also, if you're on Windows, you'll need Cygwin, MSYS2 or Windows Subsystem for Linux.

### Building with WSL

This is currently the preferred way to build, but requires Windows 10.
After installing WSL in your OS and Ubuntu on it, follow instructions from Bebbo's repo.

### Building with Cygwin

You may want to create a shortcut which launches `cmd` with cygwin & compiler paths in PATH:

```plain
C:\WINDOWS\system32\cmd.exe /c "SET PATH=path/to/bebbo/bin;path/to/cygwin/bin;%PATH%&& set PREFIX=/cygdrive/path/to/bebbo&& START cmd.exe"
```

Where `path/to/bebbo` is a path to destination where Bebbo's compiler is installed. On Cygwin, path `X:/dir1/dir2/file` gets changed to `/cygdrive/x/dir1/dir2/file` so be sure to setup `PREFIX` in proper way.
Before building ACE or Bebbo's compiler, enter `sh` or `bash` or you will experience strange errors.

Be sure there are no spaces before `&&` or one of `rm -rf` may get an empty arg and destroy your HDD contents. :)

## Prebuilt VBCC for Windows - **no longer supported**

**ACE doesn't work with VBCC Anymore.**
VBCC doesn't support latest C standards, popular language extensions, and its optimizers produce broken code.
This section is only kept for archive reasons, or in case VBCC gets better in the future.

If you're allergic to building anything and are using Windows, Kusma maintains
prebuilt VBCC on his [amiga-dev](https://github.com/kusma/amiga-dev) repo.

As noted there, you'll need to set up system variables:

- `VBCC`: your `vbcc\targets\m68k-amigaos` directory,
- `PATH`: add your `vbcc\bin` dir.

## Which compiler to choose

Bartman's GCC uses newer version (at the time of writing, GCC 10.1 vs GCC 6.5), has the debugger, latest C/C++ standards support, but doesn't have properly implemented standard library.
To mitigate this problem, ACE implements its basic parts in mini-std, but it's not and will never be a complete implementation.
Bebbo implements his own optimizers in GCC code which may produce better code than Bartman's suite, but may also break your code until they get properly tested in the field.
I'm using following ruleset:

- use Bartman's GCC as main compiler for development, or Bebbo's if you need standard library.
- when doing release builds and your code doesn't require latest compiler features, do a Bebbo build and compare size and performance.
- if anything breaks mysteriously with Bebbo's compiler, disable optimizations in ACE/game CMakeLists (change `-O3` to `-O0` or build with `-DCMAKE_BUILD_TYPE=Debug`) and try adding `-fbbb=-` to disable Bebbo's optimizers
- if problem still persists, it's probably your bug.
- if it's not, try to isolate where bug occurs and report it on ACE repo **with code sample**

## Integration with Visual Studio Code

If you don't know VSCode, give it a try - You won't go back to any other IDE.
To properly set it up for work with ACE, here are some tips on how to set it up in optimal way:

- install `twxs.cmake` and `ms-vscode.cmake-tools` extensions for CMake support
- clone the [AmigaCMakeCrossToolchains](https://github.com/AmigaPorts/AmigaCMakeCrossToolchains) repo
- from Command Palette (<kbd>ctrl</kbd>+<kbd>shift</kbd>+<kbd>p</kbd>), select `CMake: edit kits` or something.

Add following definitions for Bartman compiler, replacing `path\\to` and `yourUser` with proper contents:

```json
  {
    "name": "GCC Bartman m68k",
    "toolchainFile": "C:\\path\\to\\AmigaCMakeCrossToolchains\\m68k-bartman.cmake",
    "compilers": {
      "C": "C:\\Users\\yourUser\\.vscode\\extensions\\bartmanabyss.amiga-debug-1.1.0-preview11\\bin\\opt\\bin\\m68k-amiga-elf-gcc.exe",
      "CXX": "C:\\Users\\yourUser\\.vscode\\extensions\\bartmanabyss.amiga-debug-1.1.0-preview11\\bin\\opt\\bin\\m68k-amiga-elf-g++.exe"
    },
    "environmentVariables": {
      "PATH": "C:/Users/yourUser/.vscode/extensions/bartmanabyss.amiga-debug-1.1.0-preview11/bin/opt/bin;C:/Users/yourUser/.vscode/extensionartmanabyss.amiga-debug-1.0.0/bin;${env:PATH}"
    },
    "preferredGenerator": {
      "name": "MinGW Makefiles"
    },
    "cmakeSettings": {
      "M68K_CPU": "68000",
      "TOOLCHAIN_PREFIX": "m68k-amiga-elf",
      "TOOLCHAIN_PATH": "C:/Users/yourUser/.vscode/extensions/bartmanabyss.amiga-debug-1.1.0-preview11/bin/opt"
    },
    "keep":true
  },
```

For Bebbo compiler, if you've installed it via WSL, install `ms-vscode-remote.remote-wsl` extension.
As a side note, your Windows drives are auto-mounted there as `/mnt/c/` etc.
It will allow you to open projects from WSL point of view.
You may need to install again cmake extensions in that scope.
The CMake kit config is similar, but simpler:

```json
  {
    "name": "GCC for m68k-amigaos 6.5.0b",
    "compilers": {
      "C": "/opt/amiga/bin/m68k-amigaos-gcc",
      "CXX": "/opt/amiga/bin/m68k-amigaos-g++"
    },
    "toolchainFile": "/path/to/AmigaCMakeCrossToolchains/m68k-amigaos.cmake",
    "cmakeSettings": {
      "M68K_CPU": "68000",
      "M68K_FPU": "soft",
      "M68K_CRT": "nix13",
      "TOOLCHAIN_PREFIX": "m68k-amigaos",
      "TOOLCHAIN_PATH": "/opt/amiga"
    }
  },
```
