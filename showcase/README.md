# ACE Showcase

Used to test regressions and demonstrate features. If you're new to ACE,
it's the best place to start analyzing and hacking it.

There will be no docs to it apart of comments in source, as code is simple
and self-explanatory. Sources are very unoptimized to keep simplicity
for beginners. Quirks and clever use-cases may be shown in examples.

## Building & running

Just execute make in this directory. Executable "showcase" is produced
in the process. To run it, directory 'data' must be present in current
working directory.

You can also run it using CMake - it will build ACE automatically:

``` sh
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/AmigaCMakeCrossToolchains/m68k.cmake -DM68K_TOOLCHAIN_PATH=/path/to/toolchain -DM68K_CPU=68000 -DM68K_FPU=soft
make
```

## Adding tests & examples

To avoid .o name collisions, tests should be added in 'test' dir.
Every test should consist of single .c and .h file.

Same thing refers to examples in 'example' dir.

### Shared buffer reuse (`test/buffer_reuse`)

Demonstrates one CHIP allocation used first by `simpleBuffer`, then by
`tileBuffer` (via `scrollBuffer`) on the same viewport:

1. Run showcase, open **Tests → Shared buffer reuse**.
2. Phase 1 shows a simple-buffer checkerboard; the log prints the CHIP pool address.
3. Press **Fire** or **Return** to tear down the simple manager, re-wrap the same
   CHIP block as a scroll bitmap, and start a small tilemap (move with the joystick).
4. **ESC** returns to the menu. Enable `ACE_DEBUG` to see CHIP addresses in the log.