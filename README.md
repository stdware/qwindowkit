# QWindowKit

Cross-platform window customization framework for Qt Widgets and Qt Quick. Support Windows, macOS, Linux.

This project inherited most of [wangwenx190 FramelessHelper](https://github.com/wangwenx190/framelesshelper) implementation, with a complete refactoring and upgrading of the architecture.

## TODO

+ Fix 5.15 window unsupported behavior
+ Fix window 10 top border color in dark background
+ Fix `isFixedSize` code
+ Support customized system button area on Mac
+ Make Linux system move/resize more robust
+ Fix unhandled WinIdChange when adding a QWebEngineView as sub-widget (Win32 and Qt fixed)

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

You can also include this directory as a subproject if you choose CMake as your build system.

For other build systems, you need to install with CMake first and include the corresponding configuration files in your project.

### Import

#### CMake Project

```sh
cmake -B build -DQWindowKit_DIR=/path/install/cmake/QWindowKit
```
```cmake
find_package(QWindowKit REQUIRED)
target_link_libraries(widgets_app PUBLIC QWindowKit::Widgets)
target_link_libraries(quick_app PUBLIC QWindowKit::Quick)
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

### Qt Widgets Application

First, setup `WidgetWindowAgent` for your QWidget instance. (Each widget needs its own agent.)
```c++
auto w = new MyWidget();
auto agent = new WidgetWindowAgent(w);
agent->setup(w);
```

You can also initialize the agent in the widget constructor.
```c++
MyWidget::MyWidget(QWidget *parent) {
    // ...
    auto agent = new WidgetWindowAgent(w);
    agent->setup(w);
}
```

Then, construct your title bar widget, without which the window is lacking in basic interaction feature. You can use the [`WindowBar`](examples/shared/widgetframe/windowbar.h) provided by `WidgetFrame` in the examples as the container for your title bar components.

Let `WidgetWindowAgent` know which widget the title bar is.
```c++
agent->setTitleBarWidget(myTitleBar);
```

Set system button hints to let `WidgetWindowAgent` know the role of the widgets, which is important for the Snap Layout to work.
```c++
agent->setSystemButton(QWK::WindowAgent::Base::Maximize, maxButton);
```

Set hit-test visible hint to let `WidgetWindowAgent` know the widgets that desire to receive mouse events. 
```c++
agent->setHitTestVisible(myTitleBar->menuBar(), true);
```
Other region inside the title bar will be regarded as the draggable area for user to move the window.

Check [`MainWindow`](examples/mainwindow/mainwindow.cpp#L108) example to get detailed information.

### Qt Quick Application

TODO

### Learn More

See [examples](examples) for more demo use cases. The examples have no High DPI support.

## Documentations

+ Examples (TODO)
+ [FramelessHelper Related](docs/framelesshelper-related.md)

## License

QWindowKit is licensed under the Apache 2.0 License.