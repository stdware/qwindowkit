# Cocoa title bar double-click acceptance (FIX-010)

## Contract and static review

The existing English and Chinese tutorials document clearing
`Qt::WindowMaximizeButtonHint` as the maximization opt-out. The production Qt
fallback already checks it before both maximizing and restoring. Cocoa read the
same flags but discarded them, allowing either transition despite the opt-out.
The Cocoa guard now uses that value with the existing fullscreen check.

This is a QWindowKit custom title bar policy, not a claim that Qt's button hint
alone forbids every native or programmatic state change. Qt documents the flag
as a maximize-button hint, with a separate macOS fullscreen-button hint:
[Qt window flags](https://doc.qt.io/qt-6/qt.html#WindowType-enum).
macOS native title bars offer configurable double-click actions:
[Apple Desktop & Dock settings](https://support.apple.com/en-nz/guide/mac-help/mchlp1119/mac).
QWindowKit's existing Cocoa filter does not consult those preferences; this fix
preserves its maximize/restore policy and honors the documented opt-out.

Both UI modules reach the same Cocoa event filter. `WidgetItemDelegate` reads
`QWidget::windowFlags()` / `windowState()` and writes `setWindowState()`;
`QuickItemDelegate` reads `QQuickWindow::flags()` / `windowStates()` and writes
`setWindowStates()`. There is no Qt-major-specific branch in this decision.
The existing left-button, draggable-area, fixed-size and fullscreen guards remain.
Only the maximized bit is toggled, preserving other state bits. The new condition
introduces no callback, stored state, exported symbol or hook lifetime change.
The mouse move/release state machine and native button handlers are unchanged.

## Required native acceptance

Status: pending. Windows fallback tests do not execute `cocoawindowcontext.mm`.
No macOS UI job or substitute platform test is added by this change.

Use a local macOS desktop with the Cocoa QPA and
`QWINDOWKIT_FORCE_QT_WINDOW_CONTEXT=OFF`. Build Core, Widgets and Quick with a
supported Qt 5.15.2+ configuration and a supported Qt 6.6.2+ configuration. Record
the exact macOS, SDK, compiler and Qt versions; do not infer coverage of another
combination. Exercise both a real QWidget with WidgetWindowAgent and a real
QQuickWindow with QuickWindowAgent, each with a registered custom title bar.

Set explicit window flags (including `Qt::CustomizeWindowHint` where needed) and
verify their readback before each case. Show the window again after flag changes
if Qt hides/recreates it. Establish initial states programmatically, wait for the
native transition to settle, and record both the Qt state and visible geometry.
Use real double-click input with no drag, away from native traffic-light buttons.
For a fixed-size case, set equal minimum and maximum sizes on both axes.

| Maximize hint | Initial state | Input / location / constraint | Expected QWindowKit action |
| --- | --- | --- | --- |
| Present | Normal | Left double-click, draggable title | Maximize |
| Present | Maximized | Left double-click, draggable title | Restore |
| Absent | Normal | Left double-click, draggable title | No state change |
| Absent | Maximized | Left double-click, draggable title | No state change |
| Either | Fullscreen | Left double-click, draggable title | No state change |
| Either | Normal | Left double-click, fixed-size window | No state change |
| Either | Normal or maximized | Right or middle double-click | No double-click state toggle |
| Either | Normal or maximized | Left double-click, client area | No state change |
| Either | Normal or maximized | Left double-click, hit-test-visible title child | No state change |
| Either | Normal or maximized | Left double-click, hidden/disabled/unregistered title | No state change |

For the excluded child use an inert item without an application-defined action.
Right-button presses may have their own system-menu behavior; inspect state changes
separately. For eligible cases verify two double-clicks return to the initial state
and unrelated state bits are preserved. Repeat after clearing/re-enabling the flag
on the same window, after hide/show, and after any resulting native handle change.
Restore/close all windows between cases and bound each transition wait (for example,
five seconds); an unavailable input capability or timeout is an incomplete/failing
case, not a pass.

Repeat with the native title bar double-click preferences available on the tested
macOS version. The custom title bar must retain the documented QWindowKit policy.
Confirm normal native traffic-light actions still work independently. Capture
before/after state and geometry for each row and report every unavailable case.

To demonstrate the regression, run the two absent-hint rows on the parent revision
and the fix with identical setup. The parent is expected to toggle state; only an
actual native run can establish that before/after result. FIX-010 remains open
until the required native builds and behavior checks pass.
