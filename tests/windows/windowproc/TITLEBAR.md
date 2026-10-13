# Title-bar registration lifetime

`windows.titlebar` uses production Widgets/Quick agents, their real Win32 context
and visible HWNDs. It reparents a registered close button and hit-test exclusion
out of their title bar (both visual and QObject ownership for Quick), preserving
their scene positions. It then replaces the title in three scenarios:

- The old title is destroyed while both registered objects remain alive.
- The old title remains alive during replacement and is destroyed later.
- The registered objects are destroyed before the old title.

Assertions cover public queries, setter signal counts, clean state inside the
title-change signal, object survival, repeated setters, consecutive replacements
and new valid registrations. WM_NCHITTEST checks that a registered button returns
HTCLOSE and an exclusion returns HTCLIENT, then both positions return HTCAPTION
after replacement. This is native-message integration, not physical mouse input
or a pixel test.

The fixture uses an explicit window position and SW_SHOWNOACTIVATE, a bounded
exposure wait and native-message calls. Windows/items/agents have scoped cleanup.
There are eight expected passes with Widgets and Quick, or five with Widgets
alone. The existing runner rejects skips and expected failures, with a 15-second
process deadline and 20-second CTest deadline under the desktop resource lock.
No network is used by test execution.

`agents.unit` separately uses production public agents/delegates with an
offscreen base context. Its independent survivor case does not delete the
registered objects before deleting the title. A separate first-assignment case
preserves registrations created before any title. The suite has 38 expected
passes with both modules, or 20 with either module alone.

The implementation remembers whether a title has been assigned, independently
of its weak pointer. Cleanup occurs on replacement, retaining the existing
destruction notification behavior and avoiding destruction callbacks from old
titles. The macOS agent setters already retire their registered system-button
area when accepting a replacement title, including when the old title's pointer
has expired; no macOS implementation changes are required here. Native macOS
behavior was inspected statically, not run on Windows.

```sh
cmake --build build --config Release --target tst_windowagents tst_titlebar
ctest --test-dir build -C Release -R '^(agents.unit|windows.titlebar)$' --output-on-failure
```
