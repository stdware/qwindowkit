# QWindowKit

Cross-platform window customization framework for Qt Widgets and Qt Quick. Support Windows, macOS, Linux.

This project inherited most of [wangwenx190 FramelessHelper](https://github.com/wangwenx190/framelesshelper) implementation, with a complete refactoring and upgrading of the architecture.

## TODO

+ Fix 5.15 window unsupported behavior
+ Fix window 10 top border color in dark background
+ Fix `isFixedSize` code
+ Support customized system button area on Mac
+ Make Linux system move/resize more robust

## Supported Platforms

+ Microsoft Windows (Vista ~ 11)
+ Apple Mac OSX (11+)
+ GNU/Linux (Tested on Ubuntu)

## Requirements

| Component | Requirement |               Detailed               |
|:---------:|:-----------:|:------------------------------------:|
|    Qt     |   \>=5.15   |      Core, Gui, Widgets, Quick       |
| Compiler  |  \>=C++17   |        MSVC 2019, GCC, Clang         |
|   CMake   |   \>=3.17   |        >=3.20 is recommended         |

### Tested Compilers

+ Windows
  + MSVC: 2019, 2022
  + MinGW: 13.2.0
+ MacOSX
  + Clang 14.0.3
+ Ubuntu
  + GCC: 9.4.0

## Dependencies

+ Qt 5.15 or higher
+ [qmsetup](https://github.com/stdware/qmsetup)

## Integrate

### Build & Install

```sh
cmake -B build \
  -Dqmsetup_DIR=<dir> \ # Optional
  -DCMAKE_INSTALL_PREFIX=/path/install \
  -G "Ninja Multi-Config"

cmake --build build --target install --config Debug
cmake --build build --target install --config Release
```

You can also include this directory as a sub-project if you choose CMake as your build system.

For other build systems, you need to install with CMake first and include the corresponding configuration files in your project.

### Import

#### CMake Project

```cmake
cmake -B build -DQWindowKit_DIR=/path/install/cmake/QWindowKit
```
```cmake
find_package(QWindowKit REQUIRED)
taraget_link_libraries(widgets_app PUBLIC QWindowKit::Widgets)
taraget_link_libraries(quick_app PUBLIC QWindowKit::Quick)
```

#### QMake Project
```cmake
# WidgetsApp.pro
include("/path/install/share/QWindowKit/qmake/QWKWidgets.pri")

# QuickApp.pro
include("/path/install/share/QWindowKit/qmake/QWKQuick.pri")
```

#### Visual Studio Project

TODO

## Quick Start

### Initialization

First of all, you're supposed to add the following code in your `main` function in a very early stage (MUST before the construction of any `Q(Gui|Core)Application` objects).

```c++
int main(int argc, char *argv[]) {
#ifdef Q_OS_WINDOWS
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#elif defined(Q_OS_MAC)
# if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qputenv("QT_MAC_WANTS_LAYER", "1");
# endif
#endif
}
```

### Qt Widgets Application

TODO

### Qt Quick Application

TODO

### Learn More

See [examples](examples) for more demo use cases. The examples has no High DPI support.

## Documentatons

+ Examples (TODO)
+ [Framelesshelper Related](docs/framelesshelper-related.md)

## License

QWindowKit is licensed under the Apache 2.0 License.