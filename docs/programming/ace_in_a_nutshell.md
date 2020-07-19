# ACE in a nutshell

## Utils and managers

ACE is divided into managers and utils.

### Utils

Utils are for working with small and/or multiple resources of same kind
(bitmaps, files, fonts, viewports). Also, there are utils which simplify doing
specific tasks (chunky processing, taglists) or simplify hardware access
(Amiga chip registers).

When you create a resource using a util, you receive a pointer to it
(e.g. `pBm = bitmapCreate(...)`). After finishing work with it, you are required
to free it using dedicated function (e.g. `bitmapDestroy(pBm)`). You can use
different functions from util to work with such resource, always passing that
resource as a parameter (e.g. `bitmapGetByteWidth(pBm)`).

### Managers

Every manager is responsible for management of single global resource
(blitter, audio, etc.). You create one with "create" or "open" function
(e.g. `blitManagerCreate()`) and close with a "destroy" or "close" counterpart
(`blitManagerDestroy()`). Between those calls you may use any of manager's
functions (e.g. `blitRect()`, `blitCopy()`, `blitLine()`). Those are used
usually once per program.

### Viewport managers

Those are a special case and addition to manager/util system. After constructing
a viewport, you need a bitmap buffer to display things on it, scroll it
and manage it in many other ways. Viewport managers are doing just that.

- A camera manager keeps track of currently displayed rectangle of a bigger
  playfield
- A buffer manager creates required bitmap for display and uses camera manager
  to issue required hardware/software operations to display relevant portion.

Buffer managers are not merely allocate bitmap for you - they do all sorts of
heavy lifting - scroll buffer displays background buffer making it fold
on Y axis, while tile buffer manages drawing of tiles which are about to display
on such buffer.

## States

Every game can be broken into some kind of states. For example:

- "menu" state may be responsible for displaying menu and navigating through it.
  When player is ready, "menu" should launch proper game and clean up so far
  used resources.
- "game" state should draw game's screen, process player's events, animate
  objects on screen, etc. After player wins or loses the game, it should free
  its resources and move state to menu.

This shows two basic concepts: lifecycle of state and transitions
between them.

### Lifecycle of state

Each state's life could be split into three phases:

- **creation** during which needed bitmaps gets allocated, screen is set up, and
  all initializations take place, e.g. initial state of players, game AI, etc.
- **loop** which is responsible for processing player's key presses, moving
  objects on screen, processing game logic: loss of health, win conditions,
  physics, etc.
- **destruction** which cleans up resources after state - in practice it
  does the exact opposite of state's creation code.

States are managed with stack, so besides swapping current state with another
state, states can be pushed and popped on stack. See "Pushing and popping
states" chapter for more in-depth overview. For fine tunning push/pop
transitions between states, ACE provides two additional phases:

- **suspend** when some child state is about to be pushed
- **resume** when some child state just popped

Both are usefull when you need run some smaller tweaks without touching creation
or destruction of a state.

### Changing states

Suppose you want to implement the logic above. It can be illustrated as below:

```
OS  --stateChange-->  menu  --stateChange-->  game
 ^                    |  ^                    |
 |----stateChange-----|  |----stateChange-----|
```

Function `stateChange` does the following:

- calls current state's destruction function,
- sets new state as current one,
- calls new state's create function.

Current state's loop code is called every time game reaches `stateProcess`
function.

### Pushing and popping states

What if you'd like to implement in-game menu which pauses game? You can use
state pushing and popping:

``` plain
OS  --stateChange-->  menu  --stateChange-->  game  --statePush--> pause
 ^                    |  ^                    |  ^                 |
 |----stateChange-----|  |----stateChange-----|  |----statePop-----|
```

This looks similar to changing state, but there's significant difference:

- when you call `statePush`, _game_ state isn't destroyed. Instead, _game_'s
  suspend function and then _pause_'s create function are called. From now on
  `stateProcess` will process _pause_'s loop.
- when you call `statePop`, _pause_'s state is destroyed, and _game_'s resume
  function is called, making _game_'s loop the current one. After that
  `stateProcess` will process _game_ loop.

### Complex state example

Suppose your menu state gets bigger and bigger. Suppose you want to allocate
common things for whole menu (font, background, display) and then process each
part of menu separately. You can implement this as below:

``` plain
                     |--POP-----------  menuMapSelect
                     |                     | ^
                     |               CHANGE| |CHANGE
                     v                     v |
OS  --CREATE-->  menuCommon  --PUSH-->  menuMain
                   | ^                     | ^
             CHANGE| |CHANGE         CHANGE| |CHANGE
                   v |                     v |
                  game                  menuOptions
```

- _menuCommon_ allocates common parts and at the end of its create function
  it pushes _menuMain_.
- In _menuMain_'s create function you draw main menu's background and make
  its loop function responsible for processing input and navigation
  to other parts.
- When user wants to navigate to options, you change state to _menuOptions_,
  which in its create function draws its background and options list.
  Loop is reponsible for setting them and navigating back to menuMain.
- _menuMapSelect_ is implemented in same way as _menuOptions_, with additional
  ability of popping into menuCommon when map is selected.
- lastly, _menuCommon_ should in its loop examine why its loop got called and
  decide what to do next. It can be done by e.g. examining some global variable.
  If it was because menuMapSelect requested launching game, you change state
  to _game_, otherwise you call `gameExit` which will effectively close the
  game.

## Main file

Most of games have same boilerplate which consists of freezing OS, creating
copper & blitter manager etc. To make initial setup less rudimentary, @approxit
has created [ace/generic/main.h](../../include/ace/generic/main.h) file.
Instead of writing `main()` function you just `#include` this file and define:

- `genericCreate()` - for creation of additional managers
- `genericProcess()` - called in a loop until game gets closed
- `genericDestroy()` - for freeing previously created managers

If you prefer exiting your game in other ways than by calling `gameExit`, you
can tune game main loop condition by defining `GENERIC_MAIN_LOOP_CONDITION`
before `#include`. For example:

``` c
#define GENERIC_MAIN_LOOP_CONDITION g_pGameStateManager->pCurrent
#include <ace/generic/main.h>
```

This changes generic main loop's behavior to check if there is any state in
selected state manager. The only caveat is to put these lines of code after
manager definition.

## Debug mode

When building ACE in debug mode, you're enabling 3 main things:

- General logging (`game.log`)
- Memory usage logging (`memory.log`)
- Sanity checks

### General logging

When writing apps, it's very convenient to produce logs with debug messages.
Since Amiga is low-spec machine and writing to disk/floppy is resource-intensive,
decision has been made to only enabling debugging in debug mode. To use it
you can use `logWrite()` fn, which accepts printf-like arguments.
Note that it will not append new line character for you, so be sure to add `\n`
code where appropriate. Simple logging is shown below:

``` c
logWrite("Hello, my favourite number is %d\n", 8);
// outputs in game.log: "Hello, my favourite number is 8"
```

Most of ACE functions are logging their actions, hence such kind of log
gets quickly large and illegible. To make it more readable, an indent system
was added, called log blocks. It consists of `logBlockBegin()` and `logBlockEnd()`
functions. `logBlockBegin()` also accepts printf-like parameters, while
`logBlockEnd()` accepts single string.

``` c
void baz(UBYTE x) {
	logBlockBegin("bar(x: %hhu)", x);
	// Note that there is no logging inside here except block begin/end
	// It will fold into one line in log
	logBlockEnd("bar()");
}

void bar(UBYTE x) {
	logBlockBegin("bar(x: %hhu)", x);
	// This will be written with indent
	logWrite("this is bar\n");
	logBlockEnd("bar()");
}

void foo(UBYTE x) {
	logBlockBegin("foo(x: %hhu)", x);
	// And this will not be folded since it will contain other blocks
	bar(x+1);
	baz(x-1);
	logBlockEnd("foo()");
}

// somewhere in the code:
foo(7);
```

Above code will produce following log:
```
Block begin: foo(x: 7)
This is foo
	Block begin: bar(x: 8)
		This is bar
	Block end: bar(), time:   0.1 us
	Block begin: baz(6)...OK, time:   0.1 us
Block end: foo(), time:   0.1 us
```

As you can see, this can also measure performance in limited manner.
Bear in mind that execution time will be most valid for deepest
blocks, since those containing any other ones will have added time
which was spent on writing internal logBlock messages.

To speed things up, in release builds `logWrite()` and logBlock
calls are changed to no-ops.

### Memory usage log

Each memory allocation and release will be logged into `memory.log`
file, as seen in example below:

``` plain
Allocated FAST memory 0@0x2193ec, size 16 (/path/to/game/ACE/src/ace/managers/copper.c:136)
Allocated CHIP memory 1@0x1c614, size 336 (/path/to/game/ACE/src/ace/utils/bitmap.c:59)
freed memory 1@0x1c614, size 336 (/path/to/game/ACE/src/ace/utils/bitmap.c:258)
freed memory 0@0x2193ec, size 16 (/path/to/game/ACE/src/ace/managers/copper.c:207)
```

Those lines are telling you following things:

- is it allocation (CHIP or FAST mem) or release
- allocation's unique number, address (`idx@addr`) and size in bytes
- where allocation/release has been made (`file:line`)

Also, there are some error messages which appear when:

- memory has been freed more than once
- free fn has been called with different size than alloc
- memory has been trashed as in writing to the left or right of allocated space
  (too big array loop?)
- memory leaks - some allocations has not been freed until the end of program
- and some more things added during ACE's development

It is a good practice to look at this log from time to time in search of lines
starting with `ERR:`. Also, at the end of the log you will find summary of allocated
memory such as following one:

``` plain
=============== MEMORY MANAGER DESTROY ==============
If something is deallocated past here, you're a wuss!
Peak usage: CHIP: 858404, FAST: 96053
```

If you forgot to free some allocations, you will see them listed beneath this summary.
Since all those errors shouldn't take place in finished code, decision has been made
to omit all those checks in Release builds. This makes code lighter and
less cpu and memory hungry.

### Sanity checks

Some operations are very painful to debug, hence there are sanity checks scattered
here and there in the code. An example of such check is one done in blit functions:
they check whether source and destination coords, as well as blit size are fitting
inside source and destination bitmaps and even if bitmap pointers are non-null.

Since sanity checks are computational-expensive, decision has been made to make
them only appear by default in Debug builds, since most of the time they shouldn't
be needed. In debug build `blitCopy()` function calls `blitSafeCopy()` with
sanity checks, while in Release build it uses `blitUnsafeCopy()` to squeeze
as much performance as possible. This way, if your code depends on runtime checks,
you can use safe variants where appropriate.
