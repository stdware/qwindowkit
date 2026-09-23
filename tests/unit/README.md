# Fast, offline unit tests

These tests use only Qt Test, Qt Core/Gui (and Widgets/Quick when enabled), Qt DBus for the portal suite, the
production QWindowKit code, and the existing CMake/CTest build tools. Test execution
does not download anything, access the network, install packages, show desktop windows,
inject desktop input, or require a display server. No Python, browser, external
mocking library, commercial tool, or additional test framework is required.

## Run

In an existing build with `QWINDOWKIT_BUILD_TESTS=ON`:

```sh
cmake --build build --config Release --target qwk_fast_unit_tests --parallel 4
ctest --test-dir build -C Release -L fast --output-on-failure --no-tests=error
```

For a fresh build, first make Qt (including Qt Test), the compiler, CMake and the
repository submodules available locally. For example, configure with a local Qt
prefix and enable Quick to include all suites:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=<local-Qt-prefix> -DQWINDOWKIT_BUILD_TESTS=ON -DQWINDOWKIT_BUILD_QUICK=ON
```

The `qwk_fast_unit_tests` target builds just the fast tests and their library
dependencies. `-L fast` runs only these tests: it does not run the slower installation,
consumer-build, native-window or pixel-rendering tests. Normal unfiltered CTest runs also
include the fast tests. Use `--output-junit fast-unit.xml` for a combined report;
each process also writes Qt Test text/XML reports inside its test build directory.

Compilation time is separate from test execution time. The manual-drag suite is
labeled `component;fast`; geometry is labeled `unit;fast`. Each test process has an
8-second execution deadline, with a 10-second outer CTest timeout. There are no
fixed sleeps. Quick notification cases use Qt Test condition waits with a 1000-ms requested timeout for queued updates
and offscreen frames; the process and CTest deadlines still apply. Destructive dispatch cases run in
isolated child processes with bounded startup/completion waits. The runner requires the expected
pass count and rejects skips/expected failures, including accidental loss of data
rows. The expected counts include Qt Test initialization and cleanup; the business
case counts below do not.

## Coverage

| CTest name | Business cases | Checks |
| --- | ---: | --- |
| `core.eventdispatch.unit` | 26 | Shared/native dispatch arguments, result forwarding, ordering, short-circuit consumption, duplicate/foreign registration, transfer, independent registration in both channels, removal and destruction during dispatch, appending filters, nested dispatch, dispatcher/filter destruction order, application-wide native filter cleanup/reinstallation including nested final-subscriber destruction with either callback result |
| `core.dispatchlifetime.unit` | 8 | Dispatcher destruction in shared/native callbacks, both callback results, direct/nested dispatch, immediate address reuse, suppression of stale/replacement filters and subsequent self-removal in the replacement dispatcher |
| `core.objecteventfilters.unit` | 5 | Qt filter order after the current filter, receiver/event identity, consumption, missing/last current filter, destroyed filters and application receiver exclusion |
| `core.windowcontext.unit` | 58 | Attribute CRUD, rejected writes/deletes, replay order, adjacent replay failures, reentrant writes/removals/hash growth, callback destruction, replay snapshot changes, handle loss/reuse, title replacement, destroyed objects, SystemButton boundaries, visibility/exclusions/button priority, fixed-size constraints, setup guards, raise/restore state preservation, centering, notification order, host replacement and observer cleanup |
| `core.qtwindowcontext.unit` | 182 | Double-click maximize/restore with state preservation and eligibility guards, scene/global coordinate selection, system-menu requests, title/client press-release transitions, unrelated events and frameless flags across handle loss/recreation; 150 resize/visibility rows and 10 dynamic cursor transitions |
| `core.windowmovegeometry.unit` | 22 | Production release-position geometry: negative screen coordinates, gaps, nearest correction, ties, reserved areas, tiny/invalid/missing screens, oversized windows and custom title offsets |
| `core.windowmove.component` | 8 | Production manual-drag event filter: movement/consumption, release position, screen changes, completion, deferred cleanup, window destruction and actual offscreen screen provider |
| `core.portalstyle.component` | 23 | Production portal refresh/subscription logic with a controlled transport: initial snapshot, independent application palette, changes/deduplication, invalid data, in-flight changes, service failure/recovery, timed retry, destruction, reentrancy and worker-thread emission |
| `core.styleagent.unit` | 9 | Theme/color state, duplicate notification suppression, invalid colors, signal-time values, reentrant notification and hook lifetime |
| `agents.unit` | 16 per enabled UI module | Widgets/Quick setup rejection, title replacement/reset, signal counts/arguments/state, all system button roles and invalid boundaries, exclusion toggles and destroyed registrations |
| `quicksystembuttonarea.component` (Windows + Quick) | 27 | Scene bounds/center, item and ancestor transforms, visual reparenting, window changes, destruction/reentrancy, pre-native registration and real QML transform-list frame updates |
| `quickgeometry.unit` (existing, Windows + Quick) | 12 | Quick transforms, precise containment, fractional bounds, singular transforms and dynamic geometry |
| `systembuttons.qml` (Windows + Quick) | 11 | Real QML numeric-to-enum conversion, production module/agent setup, getter/setter boundaries, registration preservation, duplicate/removal signal counts and enum metadata |

With Widgets, Quick and StyleAgent enabled on Windows there are 423 business cases
across thirteen CTest entries (nine unit suites and four component suites). The lifetime
suite contributes eight cases, each in a child process.
Core suites are available even
when Widgets and Quick are disabled. Both StyleAgent suites are omitted when that
component is disabled. The Quick geometry, system-button-area and QML button suites retain Windows-only test
registration; the Core/agent suites are registered on all platforms.

The context suite's 21 reentrancy cases run in bounded child processes. They check
that successful inner writes win, rejected writes retain the prior cache, unrelated
keys survive hash growth, callback arguments remain stable, and replay never visits
an outdated attribute revision or resumes an obsolete window generation (including
numeric handle reuse). Public queries, replay sequences and list/hash sizes are
checked together. Platform callbacks and handle changes are controlled inputs here;
`windows.windowlifetime` separately exercises synchronous production Windows calls.

The Core and agent button tests share eleven input cases: all five valid roles,
Unknown, -1, 6, 7, INT_MIN and INT_MAX. The fixed int enum makes the C++ casts
defined; the Core cases use bounded child processes. Invalid getter/setter calls
must preserve existing registrations and emit no agent signals. The QML suite
passes the same numbers through an actual QML function to the production module,
without replacing the context factory or invoking C++ setters directly. It runs
with a hidden window on the offscreen QPA and rejects QML warnings. Numeric
conversion outside the tested int range remains Qt's responsibility; this is not
a native hit-test or physical-input test. Its runner requires 13 passes with no
skips and retains the existing eight-second inner / ten-second CTest deadlines.

The dispatch, context and agent tests link the production libraries. Context tests
substitute platform inputs and record callbacks; they do not reimplement attribute
storage, replay or hit-test decisions. Agent tests use the existing context factory
to bypass native hooks while testing real public methods and delegates. StyleAgent's
private notifications are not exported, so its test compiles the unchanged production
`styleagent.cpp` and moc output with deterministic platform subscription substitutes;
it does not link a second copy of StyleAgent from QWKCore.

Shared and native dispatchers use one internal `EventDispatchState` for registration,
removal, nesting and lifetime checks. Each channel keeps its own state and filter
owner pointer. Filters appended during delivery participate in that same delivery;
removal leaves null slots until the outermost dispatch finishes. Consumption stops
the current delivery, while dispatcher destruction stops every outstanding frame,
even if a new dispatcher immediately occupies the same address. The application
native master stays alive when its last subscriber disappears during a callback;
later subscribers can reuse it, and final removal outside dispatch unregisters it.
The expanded dispatch suite passes on both the previous separate implementations
and the common implementation, checking behavior preservation rather than a new
bug fix. Installed-package consumers also compile both dispatcher headers to check
that the common state header is included in the install tree.

Production cursor snapshots and widget inheritance are separately covered by the
[fallback cursor regressions](../fallbackcursor/README.md), using real agents and
delegates on both the offscreen and Windows QPA. The recording delegate below
does not establish cursor restoration correctness.

The Qt fallback tests use the real `QtWindowContext` event filter with a recording
delegate and recording system move/resize/menu boundaries. Events are dispatched
synchronously; resize cases use windows shown only on the offscreen QPA. No desktop
window, native move, resize or menu operation is triggered. The matrix covers free,
fixed width, fixed height, both fixed and fixed-dialog flags across all edges/corners,
title/client positions and normal/maximized/fullscreen states. It asserts exact
resize edges, cursor, event consumption and title-drag fallthrough. Cursor changes
are refreshed on the next hover or left press after a constraint/state change. Shared-library builds compile
the unchanged private `qtwindowcontext.cpp` and its moc output into the test because
that class is not exported; static builds link its existing library implementation.
The base context and QObject filter forwarding always come from the production library.

The manual-drag component sends synthetic events through the real QObject event
filter installed by `WindowMoveManipulator` on a hidden QWindow. Tests provide a
fixed initial mouse position and controlled screen rectangles; a separate case
uses the production QScreen provider on the offscreen QPA. Release positions are
checked against explicit expectations, not a duplicate algorithm. No physical
monitor, OS input, or compositor drag is exercised.

The release policy preserves a point known to be draggable (the original grab
point, bounded to the window). It moves that point into an individual screen's
available rectangle with a 16-DIP inset, reduced for tiny screens, using the
smallest squared translation. A reachable point causes no correction; this also
supports oversized windows and custom title bars below the top edge. Ties prefer
the current screen, then Qt's sibling order. Areas are refreshed at release;
missing/invalid data preserves the position. This cannot recover a title area
that the application removes or disables during the drag. QScreen work-area
accuracy depends on the platform; in particular X11 may report full screen
geometry instead of reserved areas in multi-monitor configurations (see
[QScreen::availableGeometry](https://doc.qt.io/qt-6/qscreen.html#availableGeometry-prop)).

These suites do not verify real native handle recreation, OS theme subscription,
dragging/resizing, compositor output or physical monitor/DPI changes. Those require
separate integration tests. In particular, a fake handle transition verifies the
base context's replay contract, not a Win32/Cocoa/X11/Wayland implementation.

## Local verification before the lifetime fix (2026-09-22)

On Windows with Qt 6.12.0/MSVC, all seven suites passed in Release (about 0.30 seconds
total, including CTest/process startup) and Debug (about 0.33 seconds). Twenty
consecutive runs per Release suite passed with no skips: 140 process runs in about
4.72 seconds. A Core-only static build with Widgets, Quick and StyleAgent disabled,
installation disabled and the Qt fallback selected also passed all four applicable
suites plus the timeout policy check in about 0.19 seconds. The full Release regression
passed 19/19 CTest entries in about 20.11 seconds. Other platforms and hosted CI have
not been executed locally.

These timings are observations, not hardware-independent guarantees. CI requires
registration of the applicable fast suites before its normal test run. The Windows
static CI configuration also runs the unit suites and timeout policy check alongside
its MSBuild consumer test.

The timeout policy for all project tests, including build consumers and native
integration suites, is documented in [../README.md](../README.md).

## Quick system button area (AUDIT-017)

`quicksystembuttonarea.component` tests the production `QuickSystemButtonArea`
observer used by the macOS agent. Shared builds compile its unchanged private
source and moc into the test; static builds link its library implementation.
The adapter and Cocoa code are statically reviewed on Windows, not compiled or
executed here. The suite does not claim native AppKit button positioning,
fullscreen transitions or native-window recreation coverage.

The callback maps the entire local area rectangle with `mapRectToScene`, then
uses `QRectF::toRect()` and the existing integer `QRect::center()` convention.
It reads current geometry on each call. An area detached from the host or in
another window returns an empty rectangle; Cocoa leaves its last layout alone
until a usable area returns. Registration and updates do not create a native
window. Replacing the registration invalidates retained guarded callbacks.

The observer coalesces public item/ancestor property signals on the GUI event
loop and rebuilds connections after visual parent/window changes. For arbitrary
QQuickTransform-list edits (which have no general public change signal on Qt 5),
it compares geometry at `QQuickWindow::afterAnimating`, before scene-graph sync
on the GUI thread. Hidden/non-rendering windows need no frame notification:
the callback remains current and the next rendered frame refreshes the native
layout. There is no background timer or idle-window polling. Equal integer
rectangles in the same window do not generate redundant native updates.

The QML cases run actual software frames on the offscreen QPA and mutate a real
Translate plus its transform-list attachment on the item and its ancestor. They
assert notifications and explicit expected rectangles without calling observer
refresh methods. Other cases verify scaling, rotation, transform origins,
nested ancestors, mirroring, rounding, repeated reparenting, destruction and
notification reentrancy. The suite requires 29 Qt Test passes including
initialization/cleanup, with no skips. Physical display or OS input is not used.

## Portal appearance (AUDIT-018)

The portal suite compiles the production Qt DBus transport on every test host,
but substitutes only that transport at runtime. Its 25 Qt Test checks include
initialization/cleanup, with no skips. Normal conditions wait at most 1 second;
the automatic recovery case allows 6 seconds for the real 5-second retry timer.
The entire process retains its 8-second deadline. The worker case emits from a
worker and verifies delivery on the agent's thread. The suite does not contact
a session bus, change desktop settings or exercise real portal marshalling.
Real Linux portal changes, service/bus restart and X11/Wayland integration require
separate platform validation. The CMake discovery matrix covers 32 substitute
configurations, including static portal dependencies and missing Qt DBus; it is
not a Linux static-link test.
