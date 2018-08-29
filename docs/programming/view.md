# Views and viewports

If you did any Amiga development using graphics.library, you'll discover that
ACE builds on same basic concepts but implements it a bit differently.

## Views

Everyting on screen is contained by a single view. You create view using
`viewCreate` function.

You can have multiple views, but only one can be displayed at once. To switch
between views, use `viewLoad` function.

## Viewports

Each view consists of at least one viewport. Viewports are scrollable windows
displaying associated bitmaps. Viewports may have different palettes, resolution
etc. For example, they can be used to split view into main game area and
horizontal HUD bar at top and/or bottom of screen.

The only limitation of viewports is that they have to be placed one beneath
another - they can't be located side by side or one over another. This is
because of Amiga hardware - as data gets sent to display from left to right,
from top to bottom, viewpors are implemented by changing data source,
but it takes so long that it usually needs whole horizontal blanking time
between display lines.

## Viewport managers

A viewport alone doesn't display anything. It needs viewport managers to expand
its features:

- **Simple buffer manager** allocates display bitmap and allows for its
  scrolling. Since it's simple, it works on plain bitmap, which results
  in relatively large memory usage and prevents it from being used in games
  with large scrollable backgrounds: shmup, beat 'em up, RTS, etc.
- **Scroll buffer manager** allocates bitmap only a little larger than
  viewport's dimensions. It wraps bitmap buffer vertically and reserves
  additional buffer lines to facilitate long, but finite horizontal scrolls
  efficiently. Since display bitmap is larger than viewport, it allows
  off-screen drawing just before that portion gets displayed.
- **Tile buffer manager** is derivative of scroll buffer manager. Apart from
  facilitating large scrolls it manages drawing background for you using
  tilemaps. This is what you'll most likely want to use.
