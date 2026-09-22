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
