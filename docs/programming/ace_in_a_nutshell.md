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

## Game states

Every game can be broken into states. For example:

- "menu" state may be responsible for displaying menu and navigating through it.
  When player is ready, "menu" should launch proper game and clean up so far
  used resources.
- "game" state should draw game's screen, process player's events, animate
  objects on screen, etc. After player wins or loses the game, it should free
  its resources and move state to menu.

This shows two basic concepts: lifecycle of gamestate and transitions
between them.

### Lifecycle of gamestate

Each gamestate's life could be split into three phases:

- **creation** during which needed bitmaps gets allocated, screen is set up, and
  all initializations take place, e.g. initial state of players, game AI, etc.
- **loop** which is responsible for processing player's key presses, moving
  objects on screen, processing game logic: loss of health, win conditions,
  physics, etc.
- **destruction** which cleans up resources after gamestate - in practice it
  does the exact opposite of gamestate's creation code.

All three phases are written as separate functions and passed to gamestate
manager using gameCreate, gameChangeState or gamePushState.

### Changing gamestates

Suppose you want to implement the logic above. It can be illustrated as below:

```
OS  --gameCreate-->  menu  --gameChangeState-->  game
 ^                   |  ^                         |
 |----gameDestroy----|  |----gameChangeState------|
```

Function `gameChangeState` does the following:

- calls current state's destruction function,
- sets new state as current one,
- calls new state's create function.

Current state's loop code is called every time game reaches `gameProcess`
function.

### Pushing and popping gamestates

What if you'd like to implement in-game menu which pauses game? You can use
gamestate pushing and popping:

``` plain
OS  --gameCreate-->  menu  --gameChangeState-->  game  --gamePushState--> pause
 ^                   |  ^                         | ^                      |
 |----gameDestroy----|  |----gameChangeState------| |----gamePopState------|
```

This looks similar to changing state, but there's significant difference:

- when you call `gamePushState`, _game_ gamestate isn't destroyed. Instead,
  _pause_'s create function is called. From now on `gameProcess` will process
  _pause_'s loop.
- when you call `gamePopState`, _pause_'s gamestate is destroyed, making
  _game_'s loop the current one. After that `gameProcess` will process
  _game_ loop.

### Complex gamestate example

Suppose your menu gamestate gets bigger and bigger. Suppose you want to allocate
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
  to _game_, otherwise you call `gameClose` or `gamePopState` which will
  effectively close the game.

## Main file

Most of games have same boilerplate which consists of freezing OS, creating
copper & blitter manager etc. To make initial setup less rudimentary, @approxit
has created [ace/generic/main.h](../../include/ace/generic/main.h) file.
Instead of writing `main()` function you just `#include` this file and define:

- `genericCreate()` - for creation of additional managers
- `genericLoop()` - called in a loop until game gets closed
- `genericDestroy()` - for freeing previously created managers
