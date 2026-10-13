# Cocoa hook cleanup acceptance (FIX-011 / FIX-012)

## Static review

The cleanup is based on `6ec4dfa`. The preceding double-click change establishes
the custom title bar policy; its native acceptance remains pending separately.

- `NSWindowProxy::sendEvent` only forwarded to the saved IMP. Its event capture
  and event-loop processing were inside `#if 0`. Remove the wrapper together with
  installation, restoration, saved IMP/type and disabled event-capture remnants.
- `canBecomeKeyWindow` and `canBecomeMainWindow` installation/restoration were
  inside `#if 0`, with no other caller. Remove those disabled blocks, functions
  and saved IMP/types together.
- `mac_getNSWindow` had no caller. The current proxy obtains the NSWindow from
  its NSView directly. Remove the unused translation-unit-local helper.
- Cocoa's `m_cursorShapeChanged` was only declared and initialized. Remove both;
  the Qt fallback member with the same name is active and remains untouched.

These C++ helpers are defined solely in `cocoawindowcontext.mm`, have no export
annotation or Q_OBJECT metadata, and are not declared in installed headers.
`CocoaWindowContext`'s declaration and virtual interface are unchanged. The
project sets hidden C++/inline visibility. Source inspection found no public ABI
consumer; this is not a before/after Mach-O symbol comparison.

Tracked source, test/build integration, local patch files and available local
Git refs were checked. The cached `origin/dev` and `origin/wwx190/bugfixes` copies
retain the same inactive/forwarding code; no required local patch dependency was
identified. Unavailable downstream forks or newer remote commits were not checked.

The remaining hook pairs are:

| Target | Installed callback | Saved implementation restored |
| --- | --- | --- |
| NSWindow `setStyleMask:` | `setStyleMask` | `oldSetStyleMask` |
| NSWindow `setTitlebarAppearsTransparent:` | `setTitlebarAppearsTransparent` | `oldSetTitlebarAppearsTransparent` |
| Native view `mouseDownCanMoveWindow` | `mouseDownCanMoveWindow` | `oldMouseDownCanMoveWindow` |

Their callback bodies, order, view-class selection and observer ownership remain
unchanged. Installation still occurs when the proxy registry is empty; taking
and deleting the final proxy still restores the hooks and releases the window
observer. The proxy lifetime token and asynchronous native button work remain
unchanged. QWindowKit no longer replaces/restores NSWindow `sendEvent:` at all.

## Required native acceptance

Status: pending. Windows does not compile this Objective-C++ file. Windows builds
or dispatch/agent tests cannot establish Cocoa hook correctness. No new macOS UI
job or mock runtime test is introduced for this deletion.

On a local macOS desktop, build Core/Widgets/Quick using supported Qt 5 and Qt 6
configurations with the Cocoa context enabled. Record exact macOS/SDK/compiler/Qt
versions and compare the exported symbols before/after cleanup. Run the following
with real Widgets and Quick windows, including both kinds alive together:

1. Before creating any agent, record the IMPs of the three remaining selectors
   and NSWindow `sendEvent:`. After creating the first managed window, confirm
   only the three retained selectors change. Adding another window must not
   replace them again. Capture actual view classes as well.
2. Deliver mouse and keyboard input to managed and unmanaged windows. Check
   title dragging, focus activation, text input and ordinary button clicks;
   verify each action is delivered once and unmanaged windows behave normally.
3. Enter/exit fullscreen and exercise traffic-light close/minimize/zoom actions,
   native button placement and visibility, and the
   [double-click matrix](cocoa-doubleclick.md). Check that custom title bars stay
   transparent and content remains extended under them.
4. Destroy windows/agents in both creation and reverse order. Keep one managed
   window alive and confirm its hooks and input continue to work. Destroy the
   last one and verify all retained IMPs equal their original values and
   `sendEvent:` never changed. Repeat creation/destruction to check reinstallation.
5. Cover hide/show, native handle recreation, and immediate destruction after
   show with queued native button work. Process pending events after teardown;
   check for crashes, stale callbacks or duplicate observer notifications.

Use bounded transition waits (for example, five seconds), clean up all windows
between runs, and record every failed/unavailable case. Missing a macOS desktop
or SDK is missing coverage, not a pass. FIX-011 and FIX-012 remain open until
native build, symbol and behavior checks are complete.
