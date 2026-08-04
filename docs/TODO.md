# TODO

## Wayland: `showSystemMenu()` may be passing the wrong coordinate space

**File:** `src/core/contexts/linuxwaylandcontext.cpp`
**Introduced by:** the Wayland/X11 backend contribution (Wing-summer), needs the original author's eyes.
**Status:** unverified — reported from code reading only, no Wayland machine was available to test.

`LinuxWaylandContext::virtual_hook()` forwards the incoming point straight to the
xdg-shell request:

```cpp
auto pos = static_cast<const QPoint *>(data);
xdg_toplevel_show_window_menu(toplevel, seat, serial, pos->x(), pos->y());
```

The point that arrives here is a **global** position. `AbstractWindowContext::showSystemMenu()`
is fed from `QtWindowEventFilter::sharedEventFilter()`, which passes
`getMouseEventGlobalPos(me)`, and the public `WindowAgentBase::showSystemMenu()` is documented
in terms of global coordinates too.

`xdg_toplevel.show_window_menu` is believed to expect **surface-local** coordinates (relative to
the top-left of the window geometry). If that is right, the menu pops up displaced by the
window's own position on screen, and the error disappears only when the window happens to sit at
the origin.

Circumstantial evidence that the two Linux backends disagree about what they are handed: the X11
path in `linuxx11context.cpp` treats the same argument as global and converts it to root
coordinates,

```cpp
qreal dpr = m_windowHandle->devicePixelRatio();
int root_x = qRound(pos->x() * dpr);
int root_y = qRound(pos->y() * dpr);
```

which is the correct thing to do for `_GTK_SHOW_WINDOW_MENU`. Both backends implement the same
hook and receive the same input, so at most one of them can be right.

### To check

1. Confirm against the xdg-shell protocol XML which coordinate space `show_window_menu` wants.
2. If it is surface-local, subtract the window position before sending, e.g.
   `*pos - m_windowHandle->position()`, and decide whether a device pixel ratio conversion is
   needed at all (Wayland surface-local coordinates are logical, so probably not).
3. Test with a window that is **not** at the top-left of the screen — the bug is invisible at the
   origin.

### Related, same file

`m_windowHandle` is passed to `nativeResourceForWindow()` without a null check. It happens to be
safe today because a null window yields a null `xdg_toplevel` and the function returns early, but
the X11 path had the same shape and did crash; it has since been given an explicit guard.
