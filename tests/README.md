# Test time limits

Every QWindowKit CTest entry must have an explicit, positive `TIMEOUT` of at
most 60 seconds. `testpolicy.timeouts` inspects CTest's generated JSON manifest
for all enabled test directories and fails on missing, unlimited (`0`), or
excessive timeouts. It does not execute the tests it inspects. The check itself
has a five-second subprocess deadline and a ten-second CTest limit.

| Tests | CTest limit | Inner process limits |
| --- | ---: | --- |
| Fast unit suites, including Quick geometry | 10 s each | 8 s |
| `widgetborder.lifetime` | 10 s | 8 s overall; each child: 2 s startup, 5 s execution, 1 s kill wait |
| `buildsystems.install` | 15 s | Direct install process bounded by CTest |
| `buildsystems.cmake` | 60 s | Configure 25 s, build 25 s |
| `buildsystems.qtmajor` | 45 s | Each isolated configuration: 5 s |
| `buildsystems.components.*` | 35 s each | Configure 15 s, build 15 s |
| `buildsystems.qmake` | 60 s | qmake 15 s, make 25 s |
| `buildsystems.msbuild` | 60 s | MSBuild 25 s |
| `qmltypes.installed` | 60 s total | Install 10 s, configure/build 20 s each, runtime 8 s, each lint invocation 5 s |
| `quickborder.software.*`, `quickborder.native.d3d11` | 20 s each | 15 s |
| `windows.hittest` | 45 s | Driver 40 s; existing fixture/message/cleanup deadlines also apply |
| `windows.windowproc`, `windows.windowlifetime` | 20 s each | 15 s overall; each child: 2 s startup, 5 s execution, 1 s kill wait |

Inner deadlines apply to individual operations. The outer CTest deadline caps
the entire test even when several operations are slow. Timeout is a failure;
it is never converted to a skip, success, or automatic retry. Test configuration's
qmake prefix query is also bounded to five seconds and fails if it cannot finish.

Full CI test steps have a separate three-minute aggregate deadline. The static
unit/MSBuild consumer CI step is limited to two minutes and the no-system-borders
hit-test step to one minute. These CI deadlines exclude dependency setup and
building the main project. Local CTest runs enforce the per-test limits above;
they do not inherit the CI step's aggregate deadline.

Run all enabled tests from a configured, built tree, using the compiler's developer
environment for build consumers:

```sh
ctest --test-dir build -C Release --output-on-failure --no-tests=error
```

For the short, offline unit-only loop, see [unit/README.md](unit/README.md) and
select `-L fast`. To inspect the timeout policy alone, select
`-R '^testpolicy.timeouts$'`. All helpers use the existing CMake/CTest tools;
no external testing framework or network access is added.

Before the lifetime regression additions, local validation on Windows/MSVC and Qt 6.12.0: all 19 registered Release tests,
including qmake and the expanded unit suites, passed in approximately 20.11 seconds
under these limits.
Negative probes confirmed rejection of absent/zero/61-second timeouts and
acceptance of a ten-second timeout. A deliberately sleeping consumer process
failed after approximately 1.03 seconds with a one-second step budget. Hosted
CI and other platforms have not been executed with the tighter limits.

The independently built `qmsetup` and nested `stdcorelib` submodule suites are
not part of QWindowKit's CTest invocation. Their registered tests already have
their own 60/120-second limits; this change does not modify those repositories.
