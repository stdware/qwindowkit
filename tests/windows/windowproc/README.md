# Native window procedure regressions

This Windows integration suite uses real QWidget HWNDs, the production Win32
context, and independently installed predecessor/successor procedures. Each case
runs in an isolated child process with a five-second execution deadline. It needs
Widgets, Qt Test and the native Windows QPA; the Qt fallback is excluded. No network,
input injection, additional test framework, or system setting changes are needed.

Five chain cases check distinct predecessors and exact message/return-value
forwarding, restoration on detach, retention under later subclasses, reattachment
without a procedure cycle, native destruction after detaching, removal from an
active callback, and fresh registration after teardown. Assertions verify procedure
identity, callback order and teardown counts, not just absence of crashes. Creating
a fresh HWND does not guarantee that Windows reuses the same numeric handle.

Eight reentrancy cases compare nonclient geometry and return values against
non-nested calls on the same native windows. They cover same/different windows,
unmanaged and inactive-hook windows, two nested levels, and deletion or replacement
of the outer agent from an inner callback. A separate dispatcher probe submits the
same HWND/message type with a different payload and requires it to remain untouched.
Callback counts and nesting depth confirm that the intended paths actually ran.
These are synchronous native-message tests, not native mouse/keyboard input tests.

```sh
cmake --build build --config Release --target tst_windowproc --parallel 4
ctest --test-dir build -C Release -R '^windows.windowproc$' --output-on-failure --no-tests=error
```

The runner checks the exact Qt Test pass count and rejects skips/expected failures.
CTest limits the suite to 20 seconds; its process has a 15-second inner deadline.
Tests use the desktop resource lock shared with other native regressions.

The implementation follows the documented per-window predecessor chain in
[SetWindowLongPtrW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowlongptrw)
and retires retained records at
[WM_NCDESTROY](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-ncdestroy).
Installation/restoration failure branches and exact numeric HWND reuse are reviewed
statically; they are not injected in this native suite.

Local validation (2026-09-22): Windows 11, MSVC, Qt 6.12.0, Release, system borders
enabled. All 13 native-suite cases and all 22 project CTest entries passed. Restoring
the original global predecessor failed the distinct-window and detach assertions.
Restoring the previous message-context handling failed seven reentrancy cases on
geometry or unwanted consumption; restoring the fix passed the complete regression.
Other Qt/compiler/Windows versions, sanitizers and hosted CI were not run.

## Native menu and WinId lifetime regressions

`windows.windowlifetime` contains twenty-one cases, each in an isolated process. Eight
menu cases use real HWNDs and native menu loops: cancellation, selecting Close,
icon double-click, deleting the agent or window, recreating the HWND, replacing
the agent, and a reentrant menu request. A fixed menu mnemonic drives selection
through posted character messages; these are native-message integration tests,
not physical mouse/keyboard input tests. Callbacks must actually run, Close must
reach the widget, and surviving registrations must service another menu.

Five WinId cases cover context deletion during actual Windows QPA surface teardown,
nested surface notifications, native surface recreation, and deletion of QWindow
or QWidget receivers during explicit event delivery. The receiver-deletion cases
are controlled Qt event probes, not deletion from inside Qt's own destroy() stack.
The QWindow delegate uses controlled non-lifecycle operations; surface events,
the WinId filter, shared dispatcher and Win32 context are production code.

The shared-library test compiles the unmodified private Win32 implementation,
which is not exported, and injects a derived context through the existing factory.
Static builds link that implementation from QWKCore. The derived context and
surface filter reserve their allocations after destruction and protect them with
`VirtualProtect(PAGE_NOACCESS)`, so stale access fails deterministically. The
test does not replace menu APIs or stub their return values. Hook cleanup and
exact numeric HWND reuse also require static review; fresh HWND allocation does
not guarantee numeric reuse.

Seven attribute cases reach application callbacks through the production effect
workaround's synchronous `MoveWindow` / `WM_WINDOWPOSCHANGING` path. They delete
the context, replace/remove the active key, change other supported keys, recreate
the native window, and delete/replace during attribute replay. The tests verify
callback reachability, return values, cached state, subsequent writes and native
geometry restoration for a surviving unchanged window. Context deletion uses the
same protected allocations as the menu cases. The real Win32 implementation and
Windows QPA are used with controlled non-lifecycle QWindow delegate operations;
these tests do not claim visual effect correctness or physical input coverage.
The recreation callback consumes the old native message, so Qt does not continue
dispatching it to the QPA window that the callback just destroyed.
An eighth attribute case deletes the context during `WM_STYLECHANGING` in native
window initialization, before replay starts; the obsolete initialization must stop.

The runner requires 23 passes (21 cases plus initialization/cleanup), rejects
skips/expected failures, and shares the 15-second inner / 20-second CTest bounds
and desktop lock with `windows.windowproc`. Native menu loops have a 1.5-second
watchdog; failure to enter the intended callback remains a test failure.

```sh
cmake --build build --config Release --target tst_windowlifetime --parallel 4
ctest --test-dir build -C Release -R '^windows.windowlifetime$' --output-on-failure --no-tests=error
```
