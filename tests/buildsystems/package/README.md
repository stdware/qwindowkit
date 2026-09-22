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
