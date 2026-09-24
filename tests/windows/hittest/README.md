# Native hit-test regression

Build with `QWINDOWKIT_BUILD_TESTS=ON` and Widgets and/or Quick enabled, then run
`ctest --test-dir <build> -C Release -R windows.hittest --output-on-failure`.
Repeat in a separate build with `QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS=OFF`.
The test requires native Windows QPA and the production Win32 context; it is not
registered when `QWINDOWKIT_FORCE_QT_WINDOW_CONTEXT=ON`.

Each data row launches a fresh fixture process using the production window agent.
The driver queries the real HWND with time-limited cross-process `WM_NCHITTEST`
messages. It checks title bars, excluded controls and client content for fixed
width, fixed height, both fixed and unconstrained windows. Four edges and four
corners are also checked for each single-axis constraint.

Quick adds 30 QML transform rows: title bar, excluded control and system button,
each transformed directly or through an ancestor, with enlargement, shrinkage,
rotation, nonuniform `transform` scaling and zero scale. Expected points are
calculated independently of the production mapping, including an empty corner
inside a rotated bounding box. With both modules enabled there are 42 data rows
(plus Qt Test initialization and cleanup).

`quickgeometry.unit` separately checks the production Quick delegate using items
without windows and the offscreen QPA. Its 12 cases cover complete mapped bounds,
mirroring, precise containment, fractional edges, singular transforms and dynamic
changes. It is a unit test, not evidence of native input or rendering.

These are native message integration tests. They do not inject mouse input or
prove actual dragging/resizing, compositor appearance, or GitHub-hosted desktop
availability. Fixtures communicate over stdout and need no network or external
assets. CTest serializes desktop access; fixtures have a shutdown deadline and
the driver closes or kills each child on failure.
