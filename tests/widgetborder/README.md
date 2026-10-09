# Widgets border lifetime regression

`widgetborder.lifetime` exercises the production Windows Widgets border handler
and shared event dispatcher using the existing context factory. A recording context
requests the Windows 10 workaround on any Windows version and counts native draw
hook calls. The offscreen Qt platform keeps execution independent of the desktop.
This is a component test: it does not verify automatic workaround selection on
Windows 10, native message delivery, or GDI/DWM pixels.

Thirteen cases cover UpdateRequest and Expose delivery, consuming filters, deletion
of only the agent, deletion of the window with or without its agent, nested deletion,
and deletion of the agent in the widget's event handler. Assertions check event
delivery counts, draw counts, object lifetimes, and suppression of later shared
filters. Normal delivery must draw once; destruction must prevent the final draw.
A receiver that survives agent destruction must still receive its event.

Each case runs in its own subprocess so a lifetime failure cannot corrupt later
cases. The standard runner requires all 15 Qt Test passes (including initialization
and cleanup), rejects skips, and enforces an eight-second suite limit inside CTest's
ten-second timeout. Each child has bounded startup, completion, and kill waits.
The suite requires Widgets, Windows system borders, and Qt Test; it uses no network,
additional framework, desktop input, or system setting changes.

```sh
cmake --build build --config Release --target tst_widgetborder --parallel 4
ctest --test-dir build -C Release -R '^widgetborder.lifetime$' --output-on-failure --no-tests=error
```

The companion `core.dispatchlifetime.unit` suite checks destruction and immediate
address reuse of shared/native dispatchers, including nested callbacks. Its cases
also check that a replacement dispatcher handles self-removing filters correctly.

Local verification on Windows 11/MSVC with Qt 6.12.0 (Release): all 21 registered
CTest entries passed, including the eight dispatcher and thirteen border cases.
With the original dispatch/forwarding implementations restored as a negative
control, all eight dispatcher cases failed callback-order assertions and both
window-deletion cases that retain the agent incorrectly recorded one draw. Restoring
the fix made both suites pass. Other Qt versions, compilers, platforms, sanitizers,
and hosted CI were not run; native-message destruction remains statically reviewed.
