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

Not done yet:

- Wide sprites support is still in progress
- Sub-pixel scrolling support is still in progress

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
