# ACE in a nutshell

ACE is divided into managers and utils.

## Utils

Utils are for working with small and/or multiple resources of same kind
(bitmaps, files, fonts, viewports). Also, there are utils which simplify doing
specific tasks (chunky processing, taglists) or simplify hardware access
(Amiga chip registers).

When you create a resource using a util, you receive a pointer to it
(e.g. `pBm = bitmapCreate(...)`). After finishing work with it, you are required
to free it using dedicated function (e.g. `bitmapDestroy(pBm)`). You can use
different functions from util to work with such resource, always passing that
resource as a parameter (e.g. `bitmapGetByteWidth(pBm)`).

## Managers

Every manager is responsible for management of single global resource
(blitter, audio, etc.). You create one with "create" or "open" function
(e.g. `blitManagerCreate()`) and close with a "destroy" or "close" counterpart
(`blitManagerDestroy()`). Between those calls you may use any of manager's
functions (e.g. `blitRect()`, `blitCopy()`, `blitLine()`). Those are used
usually once per program.

### Viewport managers

Those are a special case and addition to manager/util system. After constructing
a viewport, you need a bitmap buffer to display things on it. Viewport managers
are doing just that.

- A camera manager keeps track of currently displayed rectangle of a bigger
  playfield
- A buffer manager creates required bitmap for display and uses camera manager
  to issue required hardware/software operations to display relevant portion.

Buffer managers are not merely allocate bitmap for you - they do all sorts of
heavy lifting - scroll buffer displays background buffer making it fold
on Y axis, while tile buffer manages drawing of tiles which are about to display
on such buffer.

## Game states

TBD

TODO: pictures!

## Main file

Most of games have same boilerplate which consists of freezing OS, creating
copper & blitter manager etc. To make initial setup less rudimentary, @approxit
has created [ace/generic/main.h](../../include/ace/generic/main.h) file. Instead of writing `main()` function you just define:

- `genericCreate()` - for creation of additional managers
- `genericLoop()` - called in a loop until game gets closed
- `genericDestroy()` - for freeing previously created managers
