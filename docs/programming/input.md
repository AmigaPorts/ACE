# Working with Input

ACE handles two input device families: the keyboard (`ace/managers/key.h`)
and joysticks (`ace/managers/joy.h`). Joysticks 1 and 2 are the mouse ports;
joysticks 3 and 4 are optionally available on the parallel port.

All input managers share the same three-call pattern:

1. Create once, e.g. in `genericCreate()`: `keyCreate()` / `joyOpen()`.
2. Process once per frame: `keyProcess()` / `joyProcess()`.
   The generic main from `<ace/generic/main.h>` does this for you.
3. Read the state in your game loop:
   - `keyCheck()` / `joyCheck()` - held state, readable every frame.
   - `keyUse()` / `joyUse()` - one-shot: reports the key as pressed only in
     the frame the press happened. Use it for menu confirmations, toggles,
     pauses and other edge-triggered actions.

```c
void gameGsLoop(void) {
  if(keyUse(KEY_ESCAPE)) {   // pressed exactly this frame
    gameExit();
  }
  else if(keyCheck(KEY_RIGHT)) { // held down
    playerMoveRight();
  }
}
```

> [!CAUTION]
> `keyUse()` consumes the press edge. If two pieces of code check the same
> key with `keyUse()` in the same frame, only the first one sees the press.

## Stale state across gamestates

The key manager keeps its state table and updates it only on key events.
If a release gets lost - e.g. the player holds a direction while the
gamestate changes - the stuck key carries over into the next gamestate and
keeps "moving" things until it is pressed again. Reset the state when
entering a gamestate that must start clean:

```c
keyReset(); // forget all currently held keys
```

Nothing is lost by doing so: any key physically held at that moment
re-asserts itself on its next event. Clear your own input bookkeeping
(edge latches, previous-state caches) at the same place.

## Joysticks

- Ports 1 and 2 are queried with `JOY1_*` / `JOY2_*` codes, e.g.
  `joyCheck(JOY1_FIRE)`.
- `joyEnableParallel()` enables JOY3/JOY4 on the parallel port. It returns 0
  when no parallel port joystick adapter is present - safe to ignore.
