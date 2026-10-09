# qmake installation and reconfiguration

`buildsystems.qmake` builds and links a consumer against the installed enabled
modules, including Quick. The consumer references each selected module's exported
meta-object; it is not a GUI/runtime test.

When both Widgets and Quick are available, the nine
`buildsystems.qmake.reconfigure.*` tests use one isolated, real QWindowKit build
tree under the current test binary directory:

1. `prepare`: configure all modules with tests/examples disabled.
2. `build_core`, `build_widgets`, `build_quick`: build each actual library in
   dependency order, giving each cold build its own deadline.
3. `all`: install/consume all modules.
4. `core`: turn both optional modules off, install/consume Core.
5. `widgets`: enable Widgets alone, install/consume Core and Widgets.
6. `quick`: switch to Quick alone, install/consume Core and Quick.
7. `restored`: enable both again, install/consume all modules.

Each transition reconfigures and builds in the same directory. Its install prefix
and consumer build directory are new. Stale Widgets/Quick pri files must still
exist in the library build output after disabling both modules, so deleting the
output directory cannot make the regression pass. The installed pri filenames
must exactly match the enabled targets, and library artifacts must be present
only for enabled modules. All transitions build/link a real qmake consumer.

The fixture uses nested custom data/library/binary/include installation paths.
It inherits the parent Qt, generator/compiler, StyleAgent setting, shared/static
setting and Debug/Release configuration. Installed libraries, headers and install
rules come from the production project; no fake libraries or install-script copies
are used.
The prebuilt qmsetup dependency is reused, and test execution downloads nothing.
Tests do not claim independently enforced network isolation.

CTest fixtures order the stages and prevent later execution after a prerequisite
fails; a resource lock prevents parallel mutation of this test's tree. Every test
has a 60-second limit. Configuration, build, install and consumer operations also
have explicit subprocess deadlines. No retries or skip-on-failure behavior is
used. Only `prepare` cleans the dedicated test run directory, after resolving its
path; transitions preserve all historical library build outputs. Existing user
installation prefixes are never cleaned. Removing disabled-module files from an
already populated installation prefix is outside the install contract.

Run from the compiler's developer environment:

```sh
ctest --test-dir build -C Release -L reconfigure --output-on-failure --no-tests=error
```

Repeat with Debug or a parent configured with `QWINDOWKIT_BUILD_STATIC=ON` to
exercise the matching library names, definitions and static dependencies.
