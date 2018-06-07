# "Hello world" example

At first, create folder of your project. Then, create src folder for keeping your .c files. Let's start with `main.c`:

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

As you may suspect, this code will print two lines into log file. To build it, you'll need a basic makefile:

``` makefile
 TODO
```

compile it by launching `make`. Put your executable in UAE or real hardware
and launch it. Observe that `game.log` has been created in the same directory
as executable - it has lots of lines showing things done by the engine,
and among them your two lines.

## First gamestate

Now let's add a blank loop which will wait until you press escape key on your
keyboard. Let's add first gamestate in `game.c`:

``` c

void gameGsCreate(void) {
  // Initializations for this gamestate - load bitmaps, draw background, etc.
  // We don't need anything here right now
}

void gameGsLoop(void) {
  // This will loop forever until you "pop" all states or close the game
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

To make those functions visible outside `game.c` we need to create header file - for convenience we'll name it the same way, with different extension `game.h`:

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

// Without it compiler will yell about undeclard gameGsCreate etc
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
