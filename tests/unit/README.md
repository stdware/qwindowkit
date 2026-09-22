# Fast, offline unit tests

These tests use only Qt Test, Qt Core/Gui (and Widgets/Quick when enabled), the
production QWindowKit code, and the existing CMake/CTest build tools. Test execution
does not download anything, access the network, install packages, show windows,
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
consumer-build, native-window or rendering tests. Normal unfiltered CTest runs also
include the fast tests. Use `--output-junit fast-unit.xml` for a combined report;
each process also writes Qt Test text/XML reports inside its test build directory.

Compilation time is separate from test execution time. Each test process has an
8-second execution deadline, with a 10-second outer CTest timeout. There are no
sleeps, polling loops or asynchronous GUI waits. Destructive dispatch cases run in
isolated child processes with bounded startup/completion waits. The runner requires the expected
pass count and rejects skips/expected failures, including accidental loss of data
rows. The expected counts include Qt Test initialization and cleanup; the business
case counts below do not.

## Coverage

| CTest name | Business cases | Checks |
| --- | ---: | --- |
| `core.eventdispatch.unit` | 22 | Shared/native dispatch arguments, result forwarding, ordering, short-circuit consumption, duplicate/foreign registration, transfer, removal and destruction during dispatch, appending filters, nested dispatch, dispatcher/filter destruction order, application-wide native filter cleanup/reinstallation |
| `core.dispatchlifetime.unit` | 8 | Dispatcher destruction in shared/native callbacks, both callback results, direct/nested dispatch, immediate address reuse, suppression of stale/replacement filters and subsequent self-removal in the replacement dispatcher |
| `core.objecteventfilters.unit` | 5 | Qt filter order after the current filter, receiver/event identity, consumption, missing/last current filter, destroyed filters and application receiver exclusion |
| `core.windowcontext.unit` | 26 | Attribute CRUD, rejected writes/deletes, replay order, adjacent replay failures, handle loss/reuse, title replacement, destroyed objects, visibility/exclusions/button priority, fixed-size constraints, setup guards, raise/restore state preservation, centering, notification order, host replacement and observer cleanup |
| `core.qtwindowcontext.unit` | 182 | Double-click maximize/restore with state preservation and eligibility guards, scene/global coordinate selection, system-menu requests, title/client press-release transitions, unrelated events and frameless flags across handle loss/recreation; 150 resize/visibility rows and 10 dynamic cursor transitions |
| `core.styleagent.unit` | 9 | Theme/color state, duplicate notification suppression, invalid colors, signal-time values, reentrant notification and hook lifetime |
| `agents.unit` | 5 per enabled UI module | Widgets/Quick setup rejection, title replacement/reset, signal counts/arguments/state, all system button roles, exclusion toggles and destroyed registrations |
| `quickgeometry.unit` (existing, Windows + Quick) | 12 | Quick transforms, precise containment, fractional bounds, singular transforms and dynamic geometry |

With Widgets, Quick and StyleAgent enabled on Windows there are 274 business cases
across eight CTest entries. The lifetime suite adds eight cases, each in a child process.
Core suites are available even
when Widgets and Quick are disabled. The StyleAgent suite is omitted when that
component is disabled. The existing geometry suite retains its Windows registration
condition; the new platform-independent suites are registered on all platforms.

The dispatch, context and agent tests link the production libraries. Context tests
substitute platform inputs and record callbacks; they do not reimplement attribute
storage, replay or hit-test decisions. Agent tests use the existing context factory
to bypass native hooks while testing real public methods and delegates. StyleAgent's
private notifications are not exported, so its test compiles the unchanged production
`styleagent.cpp` and moc output with deterministic platform subscription substitutes;
it does not link a second copy of StyleAgent from QWKCore.

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
