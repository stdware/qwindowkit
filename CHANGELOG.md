# Changelog

**Language:** English | [简体中文](./CHANGELOG.zh-CN.md)

Historical versions and dates follow the Git tags, including the original names `1.1`, `1.2`, and `1.3`. Each entry summarizes commits since the preceding tag; the first entry covers the initial implementation. Untagged development version bumps, including `1.5.1`, are included in the next release's changes. Internal work is condensed into one line per version.

## 1.6.0 — Unreleased

Commit range: [`1.5.0..05755ba`](https://github.com/stdware/qwindowkit/compare/1.5.0...05755ba).

### Features and behavior changes

- Add macOS Liquid Glass effects, including regular and clear styles, tint colors, and rounded corners. (#233)
- Add the `dwm-border-color` window attribute for Windows 11 border colors. (#226)
- Add the `no-system-menu` window attribute to disable the native Windows system menu.
- Keep macOS system buttons hidden when AppKit would otherwise show them again after closing a panel. (#228)
- Allow disabling the Windows resize flicker workaround with `QWK_DISABLE_FLICKER_WORKAROUND`.
- Add a warning when setting up a window agent on an unsupported `QDockWidget`.
- Add an FPS display to the Qt Quick example.

### Bug fixes

- Windows: preserve client geometry when installing custom frames, prevent window drift, and retain visibility when updating titles or icons. (#243)
- Windows: fix overlapping top borders and Qt Quick border rendering, scaling, ownership, and cleanup when an agent is destroyed. (#229, #252)
- Windows: preserve explicitly configured border attributes when the native window handle is recreated.
- Windows: correct accent colors and report native effect failures while rolling back failed changes without overwriting newer material settings.
- Windows: preserve each window's original procedure chain and prevent invalid access during nested messages, system menu callbacks, and native handle changes.
- Hit testing: preserve draggable title bars for windows with a fixed width or height, and handle transformed Qt Quick items in their local coordinates.
- Qt fallback: respect resize constraints independently on each axis, restore application cursors, and retain reachable negative coordinates when dragging across monitors.
- Lifecycle: prevent crashes and stale registrations when callbacks destroy windows, agents, dispatchers, title bars, or system buttons; make attribute replay and appearance notifications safe during reentry.
- macOS: fix invalid window proxy access, handle unavailable system buttons, and update their custom area after Qt Quick scene transformations. (#234, #242)
- macOS: restore the window menu's Fill and Center actions and honor the maximize button hint when double-clicking the title bar. (#235)
- Linux: fix X11 system menu pointer grabs that freeze the UI, guard missing native resources, and correct Wayland menu coordinates, API availability checks, and native call signatures. (#213)
- Build and installation: fix Windows 7 builds, macOS ARM64 consumers with recent Apple Clang, and static import definitions in MSBuild. (#250)
- Build and installation: fix cross-compilation and bundled `qmsetup` discovery with custom library directories; honor installed CMake components and the build-time Qt major version; install only currently enabled qmake modules and link required Windows system libraries for static consumers.
- Qt Quick: generate and install Qt 6 QML tooling metadata, initialize example agents before registering controls, and fix FPS sampling and builds with StyleAgent disabled.
- Examples: preserve the custom Linux title bar and correct QML window and effect configuration. (#239)

Other refactoring and optimizations, including dependency, documentation, test, and CI updates.

## 1.5.0 — 2026-01-18

Commit range: [`1.4.0..1.5.0`](https://github.com/stdware/qwindowkit/compare/1.4.0...1.5.0).

### Features and behavior changes

- Add Linux native system menu support for Qt 6 X11 and Wayland. (#203)
- Add StyleAgent accent color access and change notifications. (#205)
- Handle Windows non-client touch events. (#211)
- Show the window title in macOS fullscreen mode. (#197)
- Always build the Qt fallback window context.
- Allow Windows title bar maximize handling without requiring `Qt::WindowMaximizeButtonHint`. (#198)

### Bug fixes

- Fix manual window move and resize operations continuing after the mouse button is released.
- Improve the Windows 10 top border workaround and remove custom margin handling that caused geometry problems. (#48)
- Prevent hidden or disabled system buttons from intercepting title bar input. (#199)
- Fix builds with Qt 6.10.1 and later, Qt 6.11, Qt 5 on Linux, and `WIN32_LEAN_AND_MEAN` on Windows.
- Fix CMake variable handling and avoid overriding parent project settings when built as a subproject.

Other refactoring and optimizations, including dependency and documentation updates.

## 1.4.0 — 2025-05-23

Commit range: [`1.3.2..1.4.0`](https://github.com/stdware/qwindowkit/compare/1.3.2...1.4.0).

### Features

- Add an always-on-top button to the Widgets example title bar. (#162)

### Bug fixes

- Preserve window attributes and their application order across native handle recreation, and fix attribute updates.
- Windows: fix native title bars reappearing when window flags change, system menu action availability, and visual effect backgrounds. (#162)
- Windows: fix top border rendering and Qt Quick border thickness at high DPI. (#167)
- Correct title bar draggable area checks so system buttons do not start window dragging.
- macOS: make `setSystemButtonVisible` take effect and use the correct QVariant type API with Qt 6. (#175, #181)
- Fix possible Qt Quick crashes and problems reopening windows. (#179)
- Fix compatibility with older Windows SDKs and missing Windows API declarations.
- Build examples as GUI applications on Windows.

Other refactoring and optimizations, including dependency and documentation updates.

## 1.3.2 — 2025-02-09

Commit range: [`1.3.1..1.3.2`](https://github.com/stdware/qwindowkit/compare/1.3.1...1.3.2).

### Bug fixes

- Fix compilation with older Qt versions by avoiding unavailable surface type enum values. (#161)

## 1.3.1 — 2025-02-09

Commit range: [`1.3..1.3.1`](https://github.com/stdware/qwindowkit/compare/1.3...1.3.1).

No functional changes or bug fixes; this tag only corrects the project version metadata.

## 1.3 — 2025-02-09

Commit range: [`1.2..1.3`](https://github.com/stdware/qwindowkit/compare/1.2...1.3).

### Bug fixes

- Fix crashes involving child frameless windows and Windows native event handling. (#151)
- Fix Windows registry access with Qt 6.8.1. (#154)
- Correct DPI conversion when calculating Windows system metrics.
- Prevent unwanted native title bars when Windows visual effects are enabled, and reduce Direct3D resize flicker.
- Work around Windows 10 border layout issues at fractional display scaling.
- Reject Qt 6.6.0 and 6.6.1 builds affected by a critical upstream Windows windowing bug.
- Fix compilation and qmake integration issues, QML example behavior, and mutually exclusive visual effect selection in the Widgets example. (#137)

Other refactoring and optimizations, including dependency and documentation updates.

## 1.2 — 2024-09-10

Commit range: [`1.1..1.2`](https://github.com/stdware/qwindowkit/compare/1.1...1.2).

### Features and behavior changes

- Add Windows native system menu handling with window icon click behavior and mouse interaction.
- Improve fixed-width, fixed-height, and fixed-size window hit testing, and respect the maximize button hint.
- Adjust Qt 5 Quick hit-test API signatures to accept mutable items for QML compatibility. (#94)

### Bug fixes

- Fix macOS title bar buttons not displaying correctly. (#75)
- Fix native child widget handling and avoid aborting on unrecognized Windows hit-test results. (#125, #133)
- Automatically remove destroyed hit-test items from registrations.
- Fix compatibility with changes to `QObjectPrivate::threadData` in Qt 5.15 and later. (#98)
- Fix installed CMake dependency discovery, qmake static import macros, and MSBuild integration. (#81)
- Fix example window behavior and system menu event handling.

Other refactoring and optimizations, including dependency and documentation updates.

## 1.1 — 2024-04-30

Commit range: [`1.0.1..1.1`](https://github.com/stdware/qwindowkit/compare/1.0.1...1.1).

### Features and behavior changes

- Disable the Qt Quick module by default in CMake; enable it explicitly with `QWINDOWKIT_BUILD_QUICK`.
- Allow mutable title bar and system button objects in agent change signals.

### Bug fixes

- Fix crashes when using multiple macOS windows.
- Fix window centering and native handle tracking when closing and showing a window again. (#34)
- Fix Qt Quick initialization, QML imports, and border painting with additional rendering backends.
- Improve the Windows 10 top border workaround and compatibility with Qt 6.6.1 frameless window handling. (#23)
- Use dynamically resolved Windows APIs correctly on older systems.
- Fix CMake package configuration, MSBuild integration, and builds with StyleAgent disabled.

Other refactoring and optimizations, including dependency and documentation updates.

## 1.0.1 — 2024-01-01

Initial tagged version: [`c2e349f`](https://github.com/stdware/qwindowkit/tree/1.0.1).

### Features

- Introduce Qt Widgets and Qt Quick window agents with custom title bars, system buttons, draggable areas, and window moving and resizing.
- Add native Windows and macOS window contexts and a Qt fallback context for other platforms.
- Support Windows system menus, Windows 11 Snap Layouts, and optional Windows system borders with a Windows 10 top border workaround.
- Add Windows dark mode, Mica, Mica Alt, and Acrylic window attributes.
- Add custom macOS system button positioning, blur effects, and system appearance notifications.
- Provide StyleAgent theme notifications, window centering and raising APIs, and CMake, qmake, and MSBuild integration.

### Bug fixes

- Fix initial Windows title bar dragging, hover, hit testing, maximization, visibility, shadow, and top border issues, including Windows 7 window behavior. (#9)
- Fix macOS window and blur view behavior and Qt Quick integration issues.
- Fix Qt 5 compilation and establish Qt 5.12.8 compatibility.

Other refactoring and optimizations.
