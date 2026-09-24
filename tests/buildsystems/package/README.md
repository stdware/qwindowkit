# Installed CMake package contracts

`buildsystems.qtmajor` generates the production config template with Qt 5 and Qt 6
build metadata, for both shared and static packages. Twelve fresh consumer caches
test the matching major alone, both majors with the wrong one first, and only the
wrong major. Consumers start with a conflicting `QT_VERSION_MAJOR`. Assertions check
the selected major, public targets, and Qt 6 private targets needed for static links.
Wrong-major-only configurations must fail while looking for the matching Qt package.

The Qt packages and export targets in this matrix are discovery substitutes. It
does not compile or link Qt 5 binaries, check ABI compatibility, or require two Qt
installations. The ordinary `buildsystems.cmake` test compiles and links a separate
consumer against the real staged installation and local Qt toolchain. Test execution
is offline; each configuration has a five-second timeout and CTest caps the matrix
at 45 seconds.

```sh
ctest --test-dir build -C Release -R '^buildsystems\.(qtmajor|cmake)$' --output-on-failure --no-tests=error
```

The package records its build-time major as `QWindowKit_QT_VERSION_MAJOR` and requests
that version's Qt targets directly. It does not force consumers to use the same Qt
patch version or make a fresh Qt5/Qt6 selection from their search paths.

The installed component names are `Core`, `Widgets`, and `Quick` (case-sensitive).
`QWindowKit_AVAILABLE_COMPONENTS` lists the modules built into the installation.
Missing required or unknown components reject the package before Qt discovery,
with a diagnostic naming the missing and installed components. Missing optional
components, including installed modules whose Qt dependencies cannot be found,
have a false `QWindowKit_<Component>_FOUND` without rejecting usable required modules.
With no component selection, the config resolves dependencies for every installed
module. With an explicit selection, consumers should only link the requested,
found components: the common export file still declares all installed targets.

`buildsystems.components.*` configures independent C++ consumers against the real
staged installation. Its nine cases cover all present modules, default selection,
Core alone, missing and unknown required components, missing optional components,
a quiet failure, a successful lookup after failure, and a missing optional Qt
dependency. Successful consumers link the selected libraries; they are not executed.
Required-component failures must be normal configure errors with the package's
diagnostic, not timeouts. Each configure and build has a 15-second timeout; each
CTest case is capped at 35 seconds. The fixture locates the installed config directly
so custom library directories such as `lib64` work on Windows too.

Run these tests in separate Core-only, Core+Widgets, and Core+Quick build trees to
exercise real missing modules, as well as with all modules enabled. Both shared and
static builds use the same cases. Build all installable targets before testing;
static Quick installations also need their generated QML resource object target.

```sh
ctest --test-dir build -C Release -R '^buildsystems\.(qtmajor|components\.|cmake)' --output-on-failure --no-tests=error
```

These are package discovery and compile/link checks. They do not test window
behavior, QML runtime registration, binary compatibility across Qt majors, or
platforms/toolchains absent from the machine executing them.
