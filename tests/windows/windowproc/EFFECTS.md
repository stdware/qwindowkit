# Windows effect failure contract

`windows.effects` runs production Widget/Quick agents and delegates against real
HWNDs. Component cases substitute only the Win32 context's capability/API boundary.
Shared builds compile that private implementation into the executable; static
builds link it from Core. The expected result is 205 passes with Quick, or 204 with
Widgets alone, with no skips or expected failures. The process limit is 15 seconds,
CTest limit 20 seconds, and the test holds the desktop resource lock. Scoped hosts
clean up their windows. Test execution does not download dependencies.

## Component coverage

- Mica, Mica Alt, legacy Mica, blur, dark mode and border color: enable/disable
  (two colors for border color), both previous states, setter rejection, cache
  preservation and explicit retry. Boolean removal is included; invalid border
  color removal retains its existing rejection behavior.
- Material margin failures, failed restoration, previous successful negative
  margins, first-write rejection and unavailable backdrop capability.
- HWND replay success/failure; same-key updates, HWND replacement and context
  deletion during setters/margin calls.
- Cross-material reentry, including Acrylic as caller and callee; failed inner
  material requests; failed setters whose callback writes newer extra margins.

These tests assert public results/cache and controlled side effects. Legacy Mica
cases assert attribute **1029**, four-byte BOOL true/false payloads, and keep the
private compatibility route executable. They do not claim build 22000 acceptance.

## Native coverage and limits

Widgets and Quick call the real APIs for enable/disable, recreation and switching
between Mica, Mica Alt and Acrylic. Mica/Mica Alt/Acrylic and dark mode are read back
through DWM. Blur checks the policy payload accepted by the real composition
setter. Border color checks the payload accepted by the real DWM setter: on local
build 26200 its getter returned E_INVALIDARG despite a successful setter. Neither
accepted payload checks nor DWM readback establish rendered pixel correctness.

No case here verifies physical input, material appearance, themes/accessibility
changes, Windows 7/8/10 native branches, or native Mica 1029 on build 22000. Those
platform/visual acceptances remain separate. Tests need no network, but this run
does not establish independently enforced network isolation.

## Implementation boundaries

Mica/Mica Alt and blur apply margins before their effect setter. A failed initial
margin call leaves the effect untouched. A rejected setter restores the last
successfully applied QWK margins, unless a callback applied newer margins.
Restoration failure returns false and emits a warning; no requested value is
committed. The previously cached value can then differ from native margins.
Margin tracking cannot account for an application's direct DWM margin writes.

Old Mica retains `_DWMWA_MICA_EFFECT = 1029` for enable and disable. Acrylic retains
its `WCA_ACCENT_POLICY = 19`, state `4`, luminosity flags `482` path. The
left-only `QMargins(65536, 0, 0, 0)` material workaround is retained.
The pre-Windows-8 DwmEnableBlurBehindWindow path still exists and now checks HRESULT.
There is no new getter prerequisite for either private material.

Dark mode first checks the per-window DWM write, then updates the existing
process-wide menu policy and flushes themes. The app-mode functions return prior
policy values, not HRESULT success codes. Border color directly propagates the
DWM setter's failure.

Related material updates share a revision guard. Successful nested updates and
partial state left by a failed nested rollback supersede an older update, which
must not resume or roll back over that work. A rejected inner call with restored
state leaves the outer eligible to commit. A successful newer margin write can
also interrupt an older margin call. Interruptions return false and can leave
partial native state without a new cached value. Same-key successful inner writes
still win. Replaced HWNDs and deleted contexts receive no further writes. The existing
Quick resize workaround completes restoration while the HWND is current before
reporting an interrupted material request.

Retry an explicit value after failure to repair native state; removal of an absent
cache entry is a base-class no-op. No hidden retry, global effect-selection model
or exclusive-material cache is introduced.

```sh
cmake --build build --config Release --target tst_effects
ctest --test-dir build -C Release -R '^windows.effects$' --output-on-failure
```
