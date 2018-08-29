# "Hello world" example

First, create folder for your project. Then, create src folder for keeping your
.c files. Let's start with `main.c`:

``` c
#include <ace/generic/main.h>

void genericCreate(void) {
  // Here goes your startup code
  logWrite("Hello, Amiga!\n");
}

void genericLoop(void) {
  // Here goes code done each game frame
  // Nothing here right now
}

void genericDestroy(void) {
  // Here goes your cleanup code
  logWrite("Goodbye, Amiga!\n")
}

```

As you may suspect, this code will print two lines into log file. To build it
you'll need a basic makefile or CMakeLists. You can base it on one found
in `showcase` directory or something like this:

Makefile:

``` makefile
 # TODO
```

CMake:

``` cmake
cmake_minimum_required(VERSION 2.8.5)
project(hello)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Lowercase project name for binaries and packaging
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

if(NOT AMIGA)
	message(SEND_ERROR "This project only compiles for Amiga")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DAMIGA -Wall -Wextra -fomit-frame-pointer")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DACE_DEBUG")
file(GLOB_RECURSE SOURCES src/*.c)
file(GLOB_RECURSE HEADERS src/*.h)

include_directories(
	${PROJECT_SOURCE_DIR}/src
)

# ACE
# If you cloned ACE into subdirectory, e.g. to `deps/ace` folder, use following:
add_subdirectory(deps/ace ace)
include_directories(deps/ace/include)
# If you installed ACE, use following:
find_package(ace REQUIRED)
include_directories(${ace_INCLUDE_DIRS})

# Linux/other UNIX get a lower-case binary name
set(TARGET_NAME ${PROJECT_NAME_LOWER})
add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${TARGET_NAME} ace)
```

Compile it by launching `make`. Put your executable in UAE or real hardware
and launch it. Observe that `game.log` has been created in the same directory
as executable - it has lots of lines showing things done by the engine,
among them two lines specified by you with `logWrite`.

## First gamestate

Now let's add a blank loop which will wait until you press escape key on your
keyboard. Let's add first gamestate in `game.c`:

``` c

void gameGsCreate(void) {
  // Initializations for this gamestate - load bitmaps, draw background, etc.
  // We don't need anything here right now
}

void gameGsLoop(void) {
  // This will loop forever until you "pop" or change this state
  // or close the game
  if(keyCheck(KEY_ESCAPE)) {
    gameClose();
  }
  else {
    // Process loop normally
    // We'll come back here later
  }
}

void gameGsDestroy(void) {
  // Cleanup when leaving this gamestate
  // Empty at the moment
}

```

To make those functions visible outside `game.c` we need to create header file -
for convenience we'll name it the same way, with different extension `game.h`:

``` c
#ifndef _GAME_H_
#define _GAME_H_

// Copy and paste headers from game.c here
// It's best to put only those function which are needed in other files.

void gameGsCreate(void);

void gameGsLoop(void);

void gameGsDestroy(void);

#endif // _GAME_H_
```

Those `#ifndef`/`#define`/`#endif` lines are called
[include guards](https://en.wikipedia.org/wiki/Include_guard) - they speed up
compilation and prevent conflicts as they ensure that given file will be parsed
only once - regardless of number of inclusions in other files.

Now let's add this gamestate along with keyboard refreshing to `main.c`:

``` c
#include <ace/generic/main.h>

// Without it compiler will yell about undeclared gameGsCreate etc
#include "game.h"

void genericCreate(void) {
  // Here goes your startup code
  logWrite("Hello, Amiga!\n");
  keyCreate(); // We'll use keyboard
  // Initialize gamestate
  gamePushState(gameGsCreate, gameGsLoop, gameGsDestroy);
}

void genericLoop(void) {
  // Here goes code done each game frame
  keyProcess();
  gameProcess(); // Process current gamestate's loop
}

void genericDestroy(void) {
  // Here goes your cleanup code
  keyDestroy(); // We don't need it anymore
  logWrite("Goodbye, Amiga!\n")
}
```

After launching your executable, you should see a black screen until you press
escape. If everything works properly, let's move on and try to put something on
the screen.
