# QWindowKit

Cross-platform window customization framework for Qt Widgets and Qt Quick.

This project inherited most of [wangwenx190 FramelessHelper](https://github.com/wangwenx190/framelesshelper)
implementation, with a complete refactoring and upgrading of the architecture.

Feature requests are welcome.

## Join with Us :triangular_flag_on_post:

You can join our [Discord channel](https://discord.gg/grrM4Tmesy). You can share your findings, thoughts and ideas on improving / implementing FramelessHelper functionalities on more platforms and apps!

## Supported Platforms

+ Microsoft Windows
+ Apple macOS (11+)
+ GNU/Linux

## Features

+ Full support of Windows 11 Snap Layout
+ Better workaround to handle Windows 10 top border issue
+ Support Mac system buttons geometry customization
+ Simpler APIs, more detailed documentations and comments

## Gallery

### Windows 11 (With Snap Layout)

![image](./docs/images/win11.png)

### Windows 10 (And 7, Vista)

![image](./docs/images/win10.png)

### macOS & Linux

|              macOS              |       Linux (Ubuntu 20.04)        |
|:-------------------------------:|:---------------------------------:|
| ![image](./docs/images/mac.png) | ![image](./docs/images/linux.png) |

## Requirements

| Component | Requirement |          Details          |
|:---------:|:-----------:|:-------------------------:|
|    Qt     |   \>=5.12   | Core, Gui, Widgets, Quick |
| Compiler  |  \>=C++17   |   MSVC 2019, GCC, Clang   |
|   CMake   |   \>=3.19   |   >=3.20 is recommended   |

### Tested Compilers

+ Windows
    + MSVC: 2019, 2022
    + MinGW (GCC): 13.2.0
+ macOS
    + Clang 14.0.3
+ Ubuntu
    + GCC: 9.4.0

## Dependencies

+ Qt 5.12 or higher
+ [qmsetup](https://github.com/stdware/qmsetup)

## Integrate

### Configure Options

+ `QWINDOWKIT_BUILD_DOCUMENTATIONS`
    + If you have installed `Doxygen`, you can **enable** this option so that the documentations will also be built and installed.
    + If not, you can read the comments in *qdoc* style in `cpp` files to get detailed usages of the public APIs.

+ `QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS`
    + If you don't want the system borders on Windows 10/11, you can **disable** this option.
    + If so, the Windows 10 top border issue will disappear. However, part of the client edge area will be occupied as the resizing margins.

+ `QWINDOWKIT_ENABLE_QT_WINDOW_CONTEXT`
    + If you want to use pure Qt emulated frameless implementation, you can **enable** this option.
    + If so, all system native features will be lost.

+ `QWINDOWKIT_ENABLE_STYLE_AGENT`
    + Select whether to exclude the style component by **disabling** this option according to your requirements and your Qt version.

### Build & Install

```sh
git clone --recursive https://github.com/stdware/qwindowkit
cd qwindowkit

cmake -B build -S . \
  -Dqmsetup_DIR=<dir> \ # Optional
  -DCMAKE_INSTALL_PREFIX=/path/install \
  -G "Ninja Multi-Config"

cmake --build build --target install --config Debug
cmake --build build --target install --config Release
```

You can also include this directory as a subproject if you choose CMake as your build system.

For other build systems, you need to install with CMake first and include the corresponding configuration files in your
project.

### Import

#### CMake Project

```sh
cmake -B build -DQWindowKit_DIR=/path/install/lib/cmake/QWindowKit
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

#### Initialization

The following initialization should be done before any widget constructs.

```cpp
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings)
    
    // ...
}
```

#### Setup Window Agent

First, setup `WidgetWindowAgent` for your top `QWidget` instance. (Each window needs its own agent.)

```c++
#include <QWKWidgets/widgetwindowagent.h>

MyWidget::MyWidget(QWidget *parent) {
    // ...
    auto agent = new QWK::WidgetWindowAgent(this);
    agent->setup(this);
    // ...
}
```

If you don't want to derive a new widget class or change the constructor, you can initialize the agent after the window
constructs.

```c++
auto w = new MyWidget();
auto agent = new QWK::WidgetWindowAgent(w);
agent->setup(w);
```

#### Construct Title bar

Then, construct your title bar widget, without which the window lacks the basic interaction feature, and it's better to
put it into the window's layout.

You can use the [`WindowBar`](examples/shared/widgetframe/windowbar.h) provided by `WidgetFrame` in the examples as the
container of your title bar components.

Let `WidgetWindowAgent` know which widget the title bar is.

```c++
agent->setTitleBar(myTitleBar);
```

Next, set system button hints to let `WidgetWindowAgent` know the role of the child widgets, which is important for the
Snap Layout to work.

```c++
agent->setSystemButton(QWK::WindowAgentBase::WindowIcon, myTitleBar->iconButton());
agent->setSystemButton(QWK::WindowAgentBase::Minimize, myTitleBar->minButton());
agent->setSystemButton(QWK::WindowAgentBase::Maximize, myTitleBar->maxButton());
agent->setSystemButton(QWK::WindowAgentBase::Close, myTitleBar->closeButton());
```

Doing this does not mean that these buttons' click events are automatically associated with window actions, you still need to manually connect the signals and slots to emulate the native window behaviors.

On macOS, this step can be skipped because it is better to use the buttons provided by the system.

Last but not least, set hit-test visible hint to let `WidgetWindowAgent` know other widgets that desire to receive mouse events.

```c++
agent->setHitTestVisible(myTitleBar->menuBar(), true);
```

The rest region within the title bar will be regarded as the draggable area for the user to move the window.

<!-- #### Window Attributes (Experimental)

On Windows 11, you can use this API to enable system effects.

```c++
agent->setWindowAttribute("mica", true);
```

Available keys: `mica`, `mica-alt`, `acrylic`, `dark-mode`. -->

### Qt Quick Application

#### Initialization

Make sure you have registered `QWindowKit` into QtQuick:

```cpp
#include <QWKQuick/qwkquickglobal.h>

int main(int argc, char *argv[])
{
    // ...
    QQmlApplicationEngine engine;
    // ...
    QWK::registerTypes(&engine);
    // ...
}
```

#### Setup Window Components

Then you can use `QWindowKit` data types and classes by importing it's URI:

```qml
import QtQuick 2.15
import QtQuick.Window 2.15
import QWindowKit 1.0

Window {
    id: window
    visible: false // We hide it first, so we can move the window to our desired position silently.
    Component.onCompleted: {
        windowAgent.setup(window)
        window.visible = true
    }
    WindowAgent {
        id: windowAgent
        // ...
    }
}
```

You can omit the version number or use "auto" instead of "1.0" for the module URI if you are using Qt6.

### Learn More

See [examples](examples) for more demo use cases. The examples have no High DPI support.

+ QWindowKit Internals [TODO]
+ [FramelessHelper Related](docs/framelesshelper-related.md)


### Vulnerabilities

+ Once you have made the window frameless, it will not be able to switch back to the system border.
+ There must not be any internal child widget with `Qt::WA_NativeWindow` property enabled, otherwise the native features and display may be abnormal. Therefore, do not set any widget that has called `QWidget::winId()` or `QWidget::setAttribute(Qt::WA_NativeWindow)` as a descendant of a frameless window.
    + If you really need to move widgets between different windows, make sure that the widget is not a top-level window and wrap it with a frameless container.

## TODO

+ Fix 5.15 window abnormal behavior
+ More documentations
+ When do we support Linux native features?

## Special Thanks

+ [Maplespe](https://github.com/Maplespe)
+ [zhiyiYo](https://github.com/zhiyiYo)

## License

QWindowKit is licensed under the [Apache 2.0 License](./LICENSE).

<!--

**You MUST keep a copyright notice of QWindowKit in a prominent place on your project, such as the README document and the About Dialog.**

**You MUST NOT remove the license text from the header files and source files of QWindowKit.**

-->
