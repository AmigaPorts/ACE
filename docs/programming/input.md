# Working with Input

ACE handles different input device families using similar managers:

- the keyboard via `<ace/managers/key.h>`
- joysticks via `<ace/managers/joy.h>`
- mice via `<ace/managers/mouse.h>`

All input managers share the same three-call pattern:

1. Create once, e.g. in `genericCreate()`: `keyCreate()` / `joyOpen()` / `mouseCreate()`.
2. Process once per frame: `keyProcess()` / `joyProcess()` / `mouseProcess()`.
  The generic main from `<ace/generic/main.h>` does this for you.
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

Joystick 2 corresponds to the port which usually serves as a mouse port, joystick 1 correspond to the latter one.

Joysticks 3 and 4 are supported through the parallel port adapter.
In order to enable them, use `joyEnableParallel()` beforehand and check the function result - if it fails, the parallel port is taken over by other process and additional joysticks are NOT meant to be used.

> [!CAUTION]
> Make sure to instruct your players to not connect anything into parallel port except for the 4-joy adapter.
>
> Connecting anything else and attempting to use it as a joystick adapter **WILL RESULT IN HARDWARE DAMAGE!**

## Mice

See the relevant [Using a sprite as a mouse pointer](mouse_sprite.md) article on how to use a mouse manager and display the mouse sprite.
