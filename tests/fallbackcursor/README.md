# Qt fallback cursor regression

These Windows-only tests select the production `QtWindowContext` using the existing
context factory, then construct real Widgets/Quick agents and their production
delegates. Shared builds compile the unchanged private fallback implementation into
the executable; static builds link the library implementation.

`fallbackcursor.offscreen` is a component test. `fallbackcursor.windows` uses real
Windows QPA windows and native handle destruction/recreation. Mouse and leave events
are delivered synchronously through Qt's event delivery. These are not physical
input or cursor-pixel tests. No network access or additional dependencies are used
during execution. Both tests enforce a 40-second process deadline, a 45-second CTest
timeout, exact pass counts, and no skips or expected failures. RAII destroys all
windows/agents, and the Windows test holds the desktop resource lock.

With both modules enabled, each run requires 70 passes: initialization/cleanup plus
32 cases per module, two Widgets inheritance cases, and two cases deleting the
agent from a QWidget cursor-change callback during takeover/restoration. Cases cover standard shapes,
color pixmaps, monochrome bitmaps, hotspots and image content, consecutive edges,
release/client/leave, hide, maximize, constraints, flags, handle recreation, repeated
ownership, application writes during ownership, and both host/agent destruction
orders (including a host-owned agent). True parent inheritance is checked directly
with a production `WidgetItemDelegate` and a child widget: Qt top-level widgets do
not inherit their parent's cursor. The default top-level test separately checks
restoration of `WA_SetCursor` after agent deletion.

The ownership contract saves the first complete cursor until ownership ends.
Application cursor writes during that interval are temporary and do not replace
the snapshot. An inherited widget resumes inheritance, including parent changes
made during ownership. Quick constraint/state signals restore immediately; QWidget
state/hide events restore immediately, but its constraint setters do not emit
QWindow constraint signals, so constraints and fixed-size flags are rechecked at
the next mouse event. Cursor updates otherwise retain the fallback move/resize
state machine's existing behavior. Dead hosts are never restored into.

Run after building `tst_fallbackcursor` and `tst_qtwindowcontext`:

```sh
ctest --test-dir build -C Release -R "^(fallbackcursor\.|core.qtwindowcontext.unit)" --output-on-failure
```
