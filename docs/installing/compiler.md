# Compiler setup

Currently, only Bartman's and Bebbo's GCC are supported.

## Bartman's toolchain

Bartman has created excellent extension for Visual Studio Code, which comes with latest GCC version and integrated debugging and profiling features.
To download it, in VSCode go to Extensions page and install `bartmanabyss.amiga-debug` extension.

Note: Bartman's extension contains prebuilt compiler and slightly modified WinUAE, hence it's **Windows-only**.

Note: currently, only the very old version extension is available directly through VSCode Extensions.
To obtain the newest package:

- navigate to [GitHub Releases](https://github.com/BartmanAbyss/vscode-amiga-debug/releases) page,
- download latest .vsix file
- in VSCode, enter Extensions tab (<kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>X</kbd>), select "..." icon and select "Install from VSIX..." option.

### Using Bartman's compiler with CMake

This is recommended approach by [Last Minute Creations](https://github.com/Last-Minute-Creations) team members.
The latest versions of Bartman's plugin support Windows, Linux, and macOS for development.
This setup works on each of these platforms.

- On Windows, you need recent version of either the MinGW GCC compiler toolchain (e.g. [winlibs-x86_64-posix-seh-gcc-12.2.0-llvm-15.0.7-mingw-w64ucrt-10.0.0-r4](https://github.com/brechtsanders/winlibs_mingw/releases/download/12.2.0-15.0.7-10.0.0-ucrt-r4/winlibs-x86_64-posix-seh-gcc-12.2.0-llvm-15.0.7-mingw-w64ucrt-10.0.0-r4.7z)) or MSVC.
- If using MinGW, make sure the directory containing the `mingw32-make` executable is in the system PATH (you can check it using `where mingw32-make` command in terminal window).
- **IMPORTANT:** When using MinGW, ensure that there are no spaces, pound signs (`#`) and other special characters in your paths
- Also, download and install [CMake](https://cmake.org) if you don't have it already.
- Be sure `cmake`'s executable directory is in your PATH or you'll have to configure vscode extension with it manually (again check using terminal command).

On Linux or macOS, make sure you have CMake and Make installed and on the PATH.

Now for the VSCode setup:

- In VSCode, install the `ms-vscode.cmake-tools`, `ms-vscode.cpptools` and `BartmanAbyss.amiga-debug` extensions.
- Also in VSCode, open any directory which will contain your project.
- Clone the [AmigaCMakeCrossToolchains](https://github.com/AmigaPorts/AmigaCMakeCrossToolchains) repo either as a submodule inside of it or anywhere outside project if you plan to use it globally.

You now need to set up the compiler for VSCode's CMake extension.
To do so on a per-project basis:

- Create a folder `.vscode` in your project
- Create a file `.vscode/cmake-kits.json` with content as follows.
  - The only reason there are two entries in configuration file is because PATH on Windows needs `;` as separator, and on Unix it's `:`. If you intend to build your project on a single platform, you can skip the unneeded entry.
  - Adapt the path to `AmigaCMakeCrossToolchains` to match where you cloned that repository.
  - Note that this configuration assumes that you have MinGW toolchain installed. You may want to replace `preferredGenerator` with `ninja` if you run into problems with it, use non-GCC compiler for native programs or you want faster building times. In this case, download Ninja from [its releases page](https://github.com/ninja-build/ninja/releases), point your system PATH to its directory and restart vscode before proceeding.

  ```json
  [
    {
      "name": "GCC Bartman m68k Win32",
      "toolchainFile": "${workspaceFolder}/../AmigaCMakeCrossToolchains/m68k-bartman.cmake",
      "environmentVariables": {
        "PATH": "${command:amiga.bin-path}/opt/bin;${command:amiga.bin-path};${command:amiga.bin-path}/opt/m68k-amiga-elf/bin;${env:PATH}"
      },
      "preferredGenerator": {
        "name": "MinGW Makefiles"
      },
      "cmakeSettings": {
        "M68K_CPU": "68000",
        "TOOLCHAIN_PREFIX": "m68k-amiga-elf",
        "TOOLCHAIN_PATH": "${command:amiga.bin-path}/opt"
      },
      "keep": true
    },
    {
      "name": "GCC Bartman m68k Unix",
      "toolchainFile": "${workspaceFolder}/deps/AmigaCMakeCrossToolchains/m68k-bartman.cmake",
      "environmentVariables": {
        "PATH": "${command:amiga.bin-path}/opt/bin:${command:amiga.bin-path}:${command:amiga.bin-path}/opt/m68k-amiga-elf/bin:${env:PATH}"
      },
      "preferredGenerator": {
        "name": "Unix Makefiles"
      },
      "cmakeSettings": {
        "M68K_CPU": "68000",
        "TOOLCHAIN_PREFIX": "m68k-amiga-elf",
        "TOOLCHAIN_PATH": "${command:amiga.bin-path}/opt"
      },
      "keep": true
    }
  ]
  ```

Next, create empty CMakeLists.txt file in the base directory of your project **and restart the editor** so that it can discover that you're inside CMake-based project.
(You can do this via command palette (<kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>P</kbd>) and typing "Reload Window".)

After this you should be good to go.
You can check in the `.vscode/cmake-kits.json` file, as long as you keep `AmigaCMakeCrossToolchains` in the same relative path to the project.
You will be able to compile your project on Windows, macOS, and Linux without any further config.

- From the command palette, select "CMake Configure" to configure your project.
- The <kbd>F7</kbd> key builds your project,
- The <kbd>F5</kbd> key runs currently selected debugging task.

Note: The Bartman's compiler produces ELF executables instead of Hunk file format.
To mitigate this, you need to invoke `elf2hunk` bundled with Bartman's toolchain.
You may want to look at how its made in CMakeLists of some other projects (e.g. [GermZ](https://github.com/tehKaiN/germz)) to see how ACE is included and the debugger is invoked there.
Also, you may want to further investigate `CMakeLists.txt` there and `.vscode/launch.json` to see how debugger is set up, as well as `.vscode/c_cpp_properties.json` to get auto-completion working.

You may also want to set up the CMake compiler kit once on your local machine instead of per-project basis.
To do so, follow the steps below:

- From command palette, launch "CMake: Scan for Kits" command to generate the kit configurations for your user.
- Access your user's CMake kit config by using the "CMake: Edit User-Local CMake Kits" command.
- Insert the toolchain definitions listed above to tweak the json configuration file and make Bartman's toolchain available for all your projects.

### Integrating ACE into Bartman's sample project

TBD

The Bartman plugin comes with a command to bootstrap the makefile-based sample project.
A dirty way is to copy src/inc files from ACE to the base project.

### Setting up the debugger

- By default, your project will be built in newly created `build` folder.
  After first successful build, take note of the executable's name.
- Inside `.vscode` folder, create `launch.json` file and paste in following contents, replacing following with your information:
  - `myGame` with your executable name, without the extension
  - `/path/to/kick13.rom` and `/path/to/kick31.rom` with valid paths to kickstart ROM files.

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "amiga",
      "request": "launch",
      "name": "Amiga 500",
      "config": "A500",
      "program": "${workspaceFolder}/build/myGame",
      "kickstart": "/path/to/kick13.rom",
      "internalConsoleOptions": "openOnSessionStart"
    },
    {
      "type": "amiga",
      "request": "launch",
      "name": "Amiga 1200",
      "config": "A1200",
      "program": "${workspaceFolder}/build/myGame",
      "kickstart": "/path/to/kick31.rom",
      "internalConsoleOptions": "openOnSessionStart"
    }
  ]
}
```

You can also add `chipmem` and `fastmem` parameters to tweak the emulator's configuration.

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
- if anything breaks mysteriously with Bebbo's compiler, disable optimizations in ACE/game CMakeLists (change `-O3` to `-O0` or build with `-DCMAKE_BUILD_TYPE=Debug`) and try adding `-fbbb=-` in compile options to disable Bebbo's optimizers.
- if problem still persists, it's probably your bug.
- if it's not, try to isolate where bug occurs and report it on ACE repo **with code sample**

## Integrating Bebbo's compiler with Visual Studio Code

If you don't know VSCode, give it a try - You won't go back to any other IDE.
To properly set it up for work with ACE, here are some tips on how to set it up in optimal way:

- If you've installed Bebbo compiler via WSL, install `ms-vscode-remote.remote-wsl` extension.
  It will allow you to open projects from WSL point of view.
- Install `twxs.cmake` and `ms-vscode.cmake-tools` extensions for CMake support. Note that you may need to reinstall some of the extensions mentioned above when connecting to WSL as they are stored on target system.
- Clone the [AmigaCMakeCrossToolchains](https://github.com/AmigaPorts/AmigaCMakeCrossToolchains) repo
- From Command Palette (<kbd>ctrl</kbd>+<kbd>shift</kbd>+<kbd>p</kbd>), select `CMake Edit User-Local CMake Kits`.

The CMake kit config is as follows:

```json5
[
  {
    "name": "Some other compiler which was automatically detected",
    // Some other fields...
  },
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
  }
]
```
