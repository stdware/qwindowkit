# TODO

## Wayland: validate system menu placement on a compositor

**File:** `src/core/contexts/waylandcontext.cpp`
**Status:** coordinate conversion fixed and statically reviewed; Wayland runtime validation pending.

`showSystemMenu()` accepts global Qt device-independent coordinates. The Wayland backend maps
them to window-local coordinates and uses Qt's native local conversion before issuing
`xdg_toplevel.show_window_menu`, which requires surface-local coordinates. QWindowKit sets
`Qt::FramelessWindowHint`, so there is no Qt client-side decoration offset to add. Wayland's
buffer scale must not be applied to the request coordinates.

References:

- [xdg-shell protocol](https://raw.githubusercontent.com/wayland-mirror/wayland-protocols/main/stable/xdg-shell/xdg-shell.xml), `show_window_menu`.
- [Qt Wayland window implementation](https://github.com/qt/qtwayland/blob/6.8/src/client/qwaylandwindow.cpp), `mapFromWlSurface()` and `createDecoration()`.
- [Qt window coordinate mapping](https://github.com/qt/qtbase/blob/6.8/src/gui/kernel/qwindow.cpp), `mapFromGlobal()`.

On a Wayland compositor that supports this menu, check both Widgets and Quick windows:

1. Right-click different title-bar positions, then move the window and repeat. The menu anchor
   should follow the click, subject to compositor placement constraints.
2. Repeat with compositor scaling at 100%, 150%, and 200%, and with `QT_SCALE_FACTOR=2` to
   exercise Qt's extra coordinate scaling separately from the Wayland buffer scale.
3. Trigger the public `showSystemMenu()` API during a valid input event using a point obtained
   from `QWindow::mapToGlobal()`, and confirm the corresponding local anchor is used.

Record the compositor and Qt versions. Windows builds and tests do not execute this backend;
no Wayland compile or runtime result is claimed for the fix.
