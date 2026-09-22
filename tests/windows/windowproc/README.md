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
