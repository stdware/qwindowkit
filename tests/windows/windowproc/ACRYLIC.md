# Acrylic API and failure regressions

`windows.acrylic` uses production agents/delegates, native HWNDs, and the production
Win32 context. Shared builds compile the private context implementation into the
test; static builds link it from Core. There are 92 expected passes with Widgets
and Quick, or 90 with Widgets alone. Skips and expected failures are rejected.
The process timeout is 15 seconds, CTest timeout 20 seconds, with a desktop lock.
All hosts and agents have scoped cleanup. Execution does not download anything.

The controlled component cases inject only capability checks and API results.
They cover enable/disable/removal from both cached boolean states, unavailable
APIs, failed reads/writes, failed margin extension/restoration, rollback failure,
explicit retry, a foreign prior backdrop, untouched private accent policy, latest
QWK margins (including Mica extension and negative margins), replay failure, and
inner writes, HWND recreation or context deletion during either API boundary.
They assert return values, cache, operation order and simulated platform state.
These cases are not old-OS acceptance tests.

The native Widgets/Quick cases exercise official DWM backdrop reads/writes,
switching from Mica/Mica Alt, enable/disable and HWND recreation. Separate forced
legacy cases call the real `SetWindowCompositionAttribute` entry point. They
check the accepted payload and successful frame-margin calls; they do not pretend
to read back the private accent policy. A forced legacy entry on a newer Windows
build is not acceptance on Windows 11 21H2. No test here asserts rendered pixels,
transparency/accessibility settings, or physical mouse input.

## Compatibility and failure contract

Build 22621+ uses the documented `DWMWA_SYSTEMBACKDROP_TYPE`. Earlier Windows 11
builds keep the project's private hack: `WCA_ACCENT_POLICY = 19`,
`ACCENT_ENABLE_ACRYLICBLURBEHIND = 4`, luminosity flags `482`, zero gradient in
`#AABBGGRR` format. It was introduced in commit
`0e9c2e428fb61953fd8f152125897646fe6fd337` and later left as reference comments.
The current implementation restores an executable legacy path; it does not turn
22000 into an unconditional rejection. The Mica `1029` enable/disable branches
and its enumeration remain intact.

For official DWM, snapshot the actual backdrop, set the requested type, and only
then apply margins. A failed margin call rolls the backdrop back. For the private
API, apply margins first and restore the previously successful QWK margins if the
setter fails. The private policy is never overwritten if the initial margin call
fails. Do not gate this hack on `GetWindowCompositionAttribute`: the local native
probe on build 26200 rejected an accent-policy read with error 87 while accepting
the corresponding setter. This does not establish read support on every OS.

All failed operations return false without committing the requested attribute.
If rollback itself fails, emit a warning and retain the last successful cache;
actual backdrop (official path) or margins (private path) may then differ. Retry
with an explicit boolean, especially `false` to disable. Removing an absent cache
entry is a base-class no-op and cannot repair a failed first application. No retry
is hidden inside the implementation. Margins tracking covers QWK's writes, not
external raw DWM margin calls. Window generations and successful nested margin
updates prevent stale tracking writes; successful inner same-key updates keep
priority, and obsolete/deleted contexts are not rolled back into.

References:

- [Microsoft DWMWINDOWATTRIBUTE](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute)
- [Tauri's private Acrylic fallback](https://github.com/tauri-apps/window-vibrancy/blob/dev/src/windows.rs)

```sh
cmake --build build --config Release --target tst_acrylic
ctest --test-dir build -C Release -R '^windows.acrylic$' --output-on-failure
```
