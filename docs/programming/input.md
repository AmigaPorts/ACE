# Working with Input

ACE handles different input device families using similar managers:

- the keyboard via `<ace/managers/key.h>`
- joysticks via `<ace/managers/joy.h>`
- mice via `<ace/managers/mouse.h>`

All input managers share the same three-call pattern:

1. Create once, e.g. in `genericCreate()`: `keyCreate()` / `joyOpen()` / `mouseCreate()`.
2. Process once per frame: `keyProcess()` / `joyProcess()` / `mouseProcess()`.
   The generic main from `<ace/generic/main.h>` doesn't do it for you - call them in `genericProcess()`, before `stateProcess()`.
3. Read the state in your game loop:
   - `keyCheck()` / `joyCheck()` / `mouseCheck()` - Gets button state, readable every frame.
   - `keyUse()` / `joyUse()` / `mouseUse()` - Reports the key as pressed only in the frame the press happened.
     Use it for menu confirmations, toggles, pauses and other edge-triggered actions.

The *check* and *use* functions are to be used in different cases - button that triggers a non-continuous effect should be scanned for being *used*, whereas a button that should have an effect in following frames should be *checked*:

```c
void gameGsLoop(void) {
  if(keyUse(KEY_ESCAPE)) {
    // Do an action once, regardless if button is still held pressed
    gameExit();
  }

  else if(keyCheck(KEY_RIGHT)) {
    // Potentially repeat an action in following frame, if the button is still pressed
    playerMoveRight();
  }
}
```

> [!CAUTION]
> Calling `keyUse()` consumes the button's newly pressed state.
> If two pieces of code check the same key with `keyUse()` in the same frame, only the first one sees the press.

## Stale keyboard state across gamestates

The key manager keeps its state table and updates it only on key events.
If a button release gets lost, e.g. the player holds a direction while the gamestate changes, the stuck key carries over into the next gamestate and keeps "moving" things until it is pressed again.
To prevent this behavior, you can reset the keyboard state when entering a gamestate by calling the `keyReset()` function.

The drawback is that all the buttons that are physically held across the `keyReset()` call are going to be marked as not pressed and will require depressing and pressing again to be registered correctly.

## Joysticks

Joystick 2 corresponds to the port which usually serves as a mouse port, joystick 1 corresponds to the other one.
Use `JOY1_*` / `JOY2_*` codes to query them, e.g. `joyCheck(JOY1_FIRE)`.

Joysticks 3 and 4 are supported through the parallel port adapter.
In order to enable them, use `joyEnableParallel()` beforehand and check the function result - if it fails, the parallel port is taken over by another process and additional joysticks are NOT meant to be used.

> [!CAUTION]
> Make sure to instruct your players to not connect anything into parallel port except for the 4-joy adapter.
>
> Connecting anything else and attempting to use it as a joystick adapter **WILL RESULT IN HARDWARE DAMAGE!**

## Mice

The mouse manager only reads the mouse - it doesn't display anything.
To display a pointer, see [Using a sprite as a mouse pointer](mouse_sprite.md).

`mouseCreate()` takes the ports to be processed, as an OR combination of `MOUSE_PORT_1` and `MOUSE_PORT_2`.
All the other functions take a single port as their first argument:

```c
void genericCreate(void) {
  keyCreate();
  mouseCreate(MOUSE_PORT_1);
  // Keep the cursor within the screen and start in its center
  mouseSetBounds(MOUSE_PORT_1, 0, 0, SCREEN_PAL_WIDTH - 1, SCREEN_PAL_HEIGHT - 1);
  mouseSetPosition(MOUSE_PORT_1, SCREEN_PAL_WIDTH / 2, SCREEN_PAL_HEIGHT / 2);
  // ...
}

void genericProcess(void) {
  keyProcess();
  mouseProcess();
  stateProcess(g_pGameStateManager);
}
```

In your game loop, read the position with `mouseGetX()` / `mouseGetY()` and the buttons (`MOUSE_LMB`, `MOUSE_RMB`, `MOUSE_MMB`) with `mouseCheck()` / `mouseUse()`.
`mouseInRect()` checks if the cursor is inside a given rectangle, which is handy for buttons and other clickable areas:

```c
tUwRect sButtonRect = {.uwX = 16, .uwY = 16, .uwWidth = 64, .uwHeight = 16};
if(mouseInRect(MOUSE_PORT_1, sButtonRect) && mouseUse(MOUSE_PORT_1, MOUSE_LMB)) {
  onButtonClicked();
}
```

> [!CAUTION]
> Call `mouseProcess()` only once per frame.
> Calling it e.g. both in `genericProcess()` and the gamestate loop applies the mouse movement twice.

Don't forget to call `mouseDestroy()` when you're done, e.g. in `genericDestroy()`.
