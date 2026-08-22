# AGA support

ACE is still OCS/ECS-first, but it includes AGA-specific functionality behind a build flag.

## Enable AGA features

Build with:

```sh
-DACE_USE_AGA_FEATURES=ON
```

This enables `ACE_USE_AGA_FEATURES` in code.

## AGA support status

Supported:

- Creating AGA-enabled views and viewports via tags in `ace/utils/extview.h`
- AGA viewport fetch mode selection via `TAG_VPORT_FMODE`
- AGA palette handling in view loading and viewport palette allocation
- AGA palette utilities (load/save/dim/mix/dump) when built with `ACE_USE_AGA_FEATURES`
- AGA sprite palette bank control helpers
- Sub-pixel / smooth hardware scrolling via extra `BPLCON1` bits (H0/H1)

Not done yet:

- Wide sprites support is still in progress

To follow status and testing progress, see [issue #151](https://github.com/AmigaPorts/ACE/issues/151).

## Creating an AGA screen

Use AGA tags on your view and viewport:

```c
s_pView = viewCreate(0,
  TAG_VIEW_USES_AGA, 1,
TAG_END);

s_pVpMain = vPortCreate(0,
  TAG_VPORT_VIEW, s_pView,
  TAG_VPORT_USES_AGA, 1,
  TAG_VPORT_BPP, 8,
  TAG_VPORT_FMODE, 0,
TAG_END);
```

Some notes:

- `TAG_VPORT_BPP` still controls depth (`8` means 256 colors)
- For AGA viewports, palette storage is expected in AGA layout (`ULONG` entries)
- `TAG_VPORT_FMODE` controls fetch mode for the viewport

For broader view/viewport basics, see [View & viewports explained](view.md).

## Smooth / sub-pixel scrolling

On AGA viewports (`TAG_VPORT_USES_AGA`), ACE programs the extra `BPLCON1` delay bits:

- **H0 / H1** (35ns): 1/4 lores pixel, 1/2 hires pixel, 1 super-hires pixel
- **H6 / H7**: already used for 32/64-pixel wide fetch

This is **horizontal only**. `BPLCON1` has no vertical equivalent; Y stays whole pixels via bitplane pointers.

Integer camera X plus `ubFineX` together drive both the bitplane pointer and the delay (they must wrap on the same 35ns boundary):

- Lores: 0–3 (four steps per pixel)
- Hires: 0–1 (two steps per pixel)

```c
cameraMoveByFine(pCamera, 1, 0);  // nudge +35ns
cameraSetFineX(pCamera, 2);       // 1/2 lores pixel
UBYTE ubFine = cameraGetFineX(pCamera);
```

`cameraSetCoord()` / `cameraReset()` / `cameraCenterAt()` clear the fine remainder.
`cameraMoveBy()` keeps it, except at max X where it is forced to 0 so the bitmap cannot be overscrolled.

`cameraIsMoved()` is true when either the integer position or `ubFineX` changes, so simplebuffer refreshes `BPLCON1`. `cameraGetDeltaX()` stays integer-only, so tilebuffer does not redraw tiles for a fine-only nudge.

On AGA hires, odd integer X uses H1, so 1-pixel hires scrolling works (OCS/ECS hires remains 2-pixel steps).

Hardware sprites do not follow `BPLCON1`. A playfield shifted by a fraction of a pixel will look offset relative to sprites. Blitted bobs sit in the bitmap and scroll with it.
