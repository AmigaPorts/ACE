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

## Adding tests & examples

To avoid .o name collisions, tests should be added in 'test' dir.
Every test should consist of single .c and .h file.

Same thing refers to examples in 'example' dir.