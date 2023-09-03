# "Hello world" example

First, create folder for your project. Then, create src folder for keeping your
.c files. Let's start with `src/main.c`:

```c
// Use this only if you want to enable logging to file instead of UAE console (heavy performance hit, not recommended)
#define GENERIC_MAIN_LOG_PATH "game.log"

#include <ace/generic/main.h>

void genericCreate(void) {
  // Here goes your startup code
  logWrite("Hello, Amiga!\n");
}

void genericProcess(void) {
  // Here goes code done each game frame
  // Nothing here right now
  gameExit();
}

void genericDestroy(void) {
  // Here goes your cleanup code
  logWrite("Goodbye, Amiga!\n");
}
```

As you may suspect, this code will print two lines into log file.
To build it, create a CMakeLists.txt file in the base directory of your project.
You can base it on one found in `showcase` directory or use one added below.

```cmake
cmake_minimum_required(VERSION 3.14.0)
project(hello LANGUAGES C)

# Lowercase project name for binaries and packaging
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

if(NOT AMIGA)
	message(SEND_ERROR "This project only compiles for Amiga")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DAMIGA -Wall -Wextra -fomit-frame-pointer")
file(GLOB_RECURSE SOURCES src/*.c)
file(GLOB_RECURSE HEADERS src/*.h)

include_directories(${PROJECT_SOURCE_DIR}/src)

# Debugging flag exposed via CMake specifically for your game code
if(GAME_DEBUG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGAME_DEBUG")
endif()

# ACE
add_subdirectory(deps/ace ace)
include_directories(deps/ace/include)
# If you built standalone and installed ACE, use following instead:
# find_package(ace REQUIRED)
# include_directories(${ace_INCLUDE_DIRS})

# Force lower-case binary name for Linux etc.
set(TARGET_NAME ${PROJECT_NAME_LOWER})

if(ELF2HUNK)
  # Add elf2hunk step for Bartman compiler
  set(GAME_LINKED ${TARGET_NAME}.elf) # Intermediate executable
  set(GAME_EXE ${TARGET_NAME}.exe) # Use this to launch the game
  add_executable(${GAME_LINKED} ${SOURCES} ${HEADERS})
  add_custom_command(
    TARGET ${GAME_LINKED} POST_BUILD
    COMMAND ${ELF2HUNK} ${GAME_LINKED} ${GAME_EXE}
  )
else()
  # Just produce the executable with Bebbo compiler
  SET(GAME_LINKED ${TARGET_NAME})
  SET(GAME_EXE ${TARGET_NAME})
  add_executable(${GAME_LINKED} ${SOURCES} ${HEADERS})
endif()

target_link_libraries(${GAME_LINKED} ace)
```

Once you have that done, compile it.

Using Visual Studio Code:

- Press <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>P</kbd> to invoke command palette.
- Start typing and select the `CMake: Configure` command.
  This configures the project for subsequent builds.
- If asked to choose the compiler, select the Bartman/Bebbo Amiga compiler.
- To turn on ACE's extra debugging features:
  - Press <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>P</kbd>.
  - Select `CMake: Edit CMake Cache (UI)`.
  - Select `ACE_DEBUG` and optionally `ACE_DEBUG_UAE` for better debugging under UAE.
- Press <kbd>F7</kbd> to build your game.

Using terminal commands:

```sh
mkdir build && cd build
# Add optional -DACE_DEBUG_UAE=ON for better debugging experience under emulator
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/AmigaCMakeCrossToolchains/m68k.cmake \
  -DM68K_TOOLCHAIN_PATH=/path/to/toolchain \
  -DM68K_CPU=68000 -DM68K_FPU=soft -DACE_DEBUG=ON
cmake --build .
```

Put your executable in UAE or real hardware and launch it.
If you've enabled `GENERIC_MAIN_LOG_PATH`, observe that `game.log` has been created in the same directory as executable - it has lots of lines showing things done by the engine, among them two lines specified by you with `logWrite`.
Alternatively, logs should appear inside UAE / VSCode debugging console.

If `game.log` has not been created, you've not enabled ACE debugging: on CMake call, add `-DACE_DEBUG=ON` and optionally `-DACE_DEBUG_UAE=ON`.
You may need to remove `CMakeCache.txt` to enforce whole project rebuild.

## First gamestate

Now let's add a blank loop which will wait until you press escape key on your
keyboard. Let's add first gamestate in `src/game.c`:

``` c
#include "game.h"
#include <ace/managers/key.h> // We'll use key* fns here
#include <ace/managers/game.h> // For using gameExit
#include <ace/managers/system.h> // For systemUnuse and systemUse

// "Gamestate" is a long word, so let's use shortcut "Gs" when naming fns

void gameGsCreate(void) {
  // Initializations for this gamestate - load bitmaps, draw background, etc.
  // We don't need anything here right now except for unusing OS
  systemUnuse();
}

void gameGsLoop(void) {
  // This will loop forever until you "pop" or change gamestate
  // or close the game
  if(keyCheck(KEY_ESCAPE)) {
    gameExit();
  }
  else {
    // Process loop normally
    // We'll come back here later
  }
}

void gameGsDestroy(void) {
  systemUse();
  // Cleanup when leaving this gamestate
  // Empty at the moment except systemUse
}
```

To make those functions visible outside `game.c` we need to create header file -
for convenience we'll name it the same way, with different extension -
`src/game.h`:

``` c
#ifndef _GAME_H_
#define _GAME_H_

// Function headers from game.c go here
// It's best to put here only those functions which are needed in other files.

void gameGsCreate(void);
void gameGsLoop(void);
void gameGsDestroy(void);

#endif // _GAME_H_
```

Those `#ifndef`/`#define`/`#endif` lines are called
[include guards](https://en.wikipedia.org/wiki/Include_guard) - they speed up
compilation and prevent conflicts as they ensure that given file will be parsed
only once - regardless of number of inclusions in other files.

Now let's add this gamestate along with updating keyboard state to `main.c`:

``` c
// Use this only if you want to enable logging to file instead of UAE console (heavy performance hit, not recommended)
#define GENERIC_MAIN_LOG_PATH "game.log"

#include <ace/generic/main.h>
#include <ace/managers/key.h>
#include <ace/managers/state.h>
// Without it compiler will yell about undeclared gameGsCreate etc
#include "game.h"

tStateManager *g_pGameStateManager = 0;
tState *g_pGameState = 0;

void genericCreate(void) {
  // Here goes your startup code
  logWrite("Hello, Amiga!\n");
  keyCreate(); // We'll use keyboard
  // Initialize gamestate
  g_pGameStateManager = stateManagerCreate();
  g_pGameState = stateCreate(gameGsCreate, gameGsLoop, gameGsDestroy, 0, 0);

  statePush(g_pGameStateManager, g_pGameState);
}

void genericProcess(void) {
  // Here goes code done each game frame
  keyProcess();
  stateProcess(g_pGameStateManager); // Process current gamestate's loop
}

void genericDestroy(void) {
  // Here goes your cleanup code
  stateManagerDestroy(g_pGameStateManager);
  stateDestroy(g_pGameState);
  keyDestroy(); // We don't need it anymore
  logWrite("Goodbye, Amiga!\n");
}
```

After launching your executable, you should see a blank screen until you press
escape. If everything works properly, let's move on and try to put something on
the screen.
