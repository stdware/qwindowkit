# QWindowKit Tutorial

**Language:** English | [简体中文](./tutorial.zh-CN.md)

**Project README:** [English](../README.md) | [简体中文](../README.zh-CN.md)

This tutorial shows how to use QWindowKit in Qt Widgets and Qt Quick applications to build a custom title bar while keeping native behaviors such as moving, resizing, system menus, and Windows Snap Layout.

QWindowKit provides three modules:

- `QWindowKit::Core`: shared platform window infrastructure.
- `QWindowKit::Widgets`: integration for `QWidget` and `QMainWindow`.
- `QWindowKit::Quick`: integration for `QQuickWindow` and QML `Window`.

## Requirements

- Qt 5.12 or later. For real applications, Qt 5.15.2+ or Qt 6.6.2+ is recommended.
- A C++17 compiler.
- CMake 3.19 or later.
- Windows, macOS, or Linux.

## Build And Install

Build from source:

```sh
git clone --recursive https://github.com/stdware/qwindowkit
cd qwindowkit

cmake -S . -B build ^
  -DCMAKE_PREFIX_PATH=C:/Qt/6.7.3/msvc2019_64 ^
  -DCMAKE_INSTALL_PREFIX=C:/Libraries/QWindowKit ^
  -DQWINDOWKIT_BUILD_WIDGETS=ON ^
  -DQWINDOWKIT_BUILD_QUICK=ON ^
  -DQWINDOWKIT_BUILD_EXAMPLES=OFF

cmake --build build --config Release
cmake --install build --config Release
```

On Linux or macOS:

```sh
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/Qt \
  -DCMAKE_INSTALL_PREFIX=/opt/qwindowkit \
  -DQWINDOWKIT_BUILD_WIDGETS=ON \
  -DQWINDOWKIT_BUILD_QUICK=ON

cmake --build build --config Release
cmake --install build --config Release
```

Common options:

```cmake
-DQWINDOWKIT_BUILD_WIDGETS=ON
-DQWINDOWKIT_BUILD_QUICK=ON
-DQWINDOWKIT_BUILD_EXAMPLES=ON
-DQWINDOWKIT_BUILD_STATIC=OFF
-DQWINDOWKIT_FORCE_QT_WINDOW_CONTEXT=OFF
-DQWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS=ON
```

## Use From CMake

Widgets application:

```cmake
cmake_minimum_required(VERSION 3.19)
project(MyWidgetsApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(QWindowKit REQUIRED COMPONENTS Widgets)

qt_add_executable(MyWidgetsApp
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(MyWidgetsApp PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    QWindowKit::Widgets
)
```

Quick application:

```cmake
cmake_minimum_required(VERSION 3.19)
project(MyQuickApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml)
find_package(QWindowKit REQUIRED COMPONENTS Quick)

qt_add_executable(MyQuickApp
    main.cpp
)

qt_add_qml_module(MyQuickApp
    URI MyQuickApp
    VERSION 1.0
    QML_FILES Main.qml
)

target_link_libraries(MyQuickApp PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    QWindowKit::Quick
)
```

Configure your downstream project with the installed package path:

```sh
cmake -S . -B build -DQWindowKit_DIR=C:/Libraries/QWindowKit/lib/cmake/QWindowKit
```

## Widgets: Minimal Window

Widgets applications must set `Qt::AA_DontCreateNativeWidgetSiblings` before constructing `QApplication`.

`main.cpp`:

```cpp
#include <QtCore/QCoreApplication>
#include <QtWidgets/QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    QApplication app(argc, argv);

    MainWindow window;
    window.resize(900, 600);
    window.show();

    return app.exec();
}
```

`mainwindow.h`:

```cpp
#pragma once

#include <QtWidgets/QMainWindow>

namespace QWK {
class WidgetWindowAgent;
}

class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupWindowAgent();

    QWK::WidgetWindowAgent *m_windowAgent = nullptr;
};
```

`mainwindow.cpp`:

```cpp
#include "mainwindow.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

#include <QWKWidgets/widgetwindowagent.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupWindowAgent();

    auto *content = new QLabel(tr("Application content"), this);
    content->setAlignment(Qt::AlignCenter);
    setCentralWidget(content);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupWindowAgent()
{
    m_windowAgent = new QWK::WidgetWindowAgent(this);
    m_windowAgent->setup(this);

    auto *titleBar = new QWidget(this);
    titleBar->setFixedHeight(40);

    auto *iconButton = new QPushButton(QStringLiteral("App"), this);
    auto *menuBar = new QMenuBar(this);
    menuBar->addMenu(tr("&File"))->addAction(tr("E&xit"), this, &QWidget::close);

    auto *titleLabel = new QLabel(windowTitle(), this);
    titleLabel->setAlignment(Qt::AlignCenter);

    auto *minButton = new QPushButton(QStringLiteral("-"), this);
    auto *maxButton = new QPushButton(QStringLiteral("□"), this);
    auto *closeButton = new QPushButton(QStringLiteral("x"), this);

    auto *layout = new QHBoxLayout(titleBar);
    layout->setContentsMargins(8, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(iconButton);
    layout->addWidget(menuBar);
    layout->addWidget(titleLabel, 1);
    layout->addWidget(minButton);
    layout->addWidget(maxButton);
    layout->addWidget(closeButton);

    setMenuWidget(titleBar);

    m_windowAgent->setTitleBar(titleBar);
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::WindowIcon, iconButton);
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Minimize, minButton);
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Maximize, maxButton);
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Close, closeButton);
    m_windowAgent->setHitTestVisible(menuBar, true);

    connect(minButton, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(maxButton, &QPushButton::clicked, this, [this] {
        isMaximized() ? showNormal() : showMaximized();
    });
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
}
```

Key points:

- Each top-level window needs its own `WidgetWindowAgent`.
- Call `setup(this)` as early as possible, especially before applying size constraints.
- `setTitleBar()` marks the draggable title bar area.
- Controls inside the title bar that need mouse input must be registered with `setHitTestVisible(widget, true)`.
- System button hints do not connect click behavior. You still connect the buttons yourself.

## Widgets: Interactive Title Bar Controls

Menus should be hit-test visible:

```cpp
auto *menuBar = new QMenuBar(this);
menuBar->addMenu(tr("&File"));

windowAgent->setHitTestVisible(menuBar, true);
```

Use the same rule for search boxes, tab bars, and tool buttons:

```cpp
windowAgent->setHitTestVisible(searchEdit, true);
windowAgent->setHitTestVisible(tabBar, true);
windowAgent->setHitTestVisible(settingsButton, true);
```

Without this, QWindowKit treats those areas as draggable title bar space, so the controls will not receive normal mouse interaction.

## Widgets: Window Buttons

Connect button behavior explicitly:

```cpp
connect(minimizeButton, &QPushButton::clicked, window, &QWidget::showMinimized);

connect(maximizeButton, &QPushButton::clicked, window, [window] {
    if (window->isMaximized()) {
        window->showNormal();
    } else {
        window->showMaximized();
    }
});

connect(closeButton, &QPushButton::clicked, window, &QWidget::close);
```

Disable maximization:

```cpp
setWindowFlag(Qt::WindowMaximizeButtonHint, false);
```

Create a fixed-size window:

```cpp
setFixedSize(640, 420);
```

## Quick: Register The QML Types

`main.cpp`:

```cpp
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQuick/QQuickWindow>

#include <QWKQuick/qwkquickglobal.h>

int main(int argc, char *argv[])
{
    QQuickWindow::setDefaultAlphaBuffer(true);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QWK::registerTypes(&engine);

    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return app.exec();
}
```

`QQuickWindow::setDefaultAlphaBuffer(true)` is usually required for transparency, blur, Mica, and glass effects. Call it before creating Quick windows.

## Quick: Minimal QML Window

`Main.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QWindowKit 1.0

Window {
    id: window
    width: 900
    height: 600
    visible: false
    title: "QWindowKit Quick Window"

    WindowAgent {
        id: windowAgent
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: titleBar
            Layout.fillWidth: true
            height: 42
            color: "#242936"

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Text {
                    text: window.title
                    color: "white"
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    Layout.leftMargin: 12
                    Layout.fillWidth: true
                }

                Button {
                    id: minButton
                    text: "-"
                    onClicked: window.showMinimized()
                }

                Button {
                    id: maxButton
                    text: "□"
                    onClicked: {
                        if (window.visibility === Window.Maximized)
                            window.showNormal()
                        else
                            window.showMaximized()
                    }
                }

                Button {
                    id: closeButton
                    text: "x"
                    onClicked: window.close()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#f5f6f8"

            Text {
                anchors.centerIn: parent
                text: "Application content"
            }
        }
    }

    Component.onCompleted: {
        windowAgent.setup(window)
        windowAgent.setTitleBar(titleBar)
        windowAgent.setSystemButton(WindowAgent.Minimize, minButton)
        windowAgent.setSystemButton(WindowAgent.Maximize, maxButton)
        windowAgent.setSystemButton(WindowAgent.Close, closeButton)
        window.visible = true
    }
}
```

If a control in the title bar needs mouse input, mark it as hit-test visible:

```qml
Button {
    id: menuButton
    text: "Menu"
}

Component.onCompleted: {
    windowAgent.setHitTestVisible(menuButton, true)
}
```

## Windows Platform Effects

Windows supports several platform attributes. Set them after `setup()` succeeds.

```cpp
windowAgent->setWindowAttribute(QStringLiteral("dark-mode"), true);
windowAgent->setWindowAttribute(QStringLiteral("mica"), true);
windowAgent->setWindowAttribute(QStringLiteral("mica-alt"), false);
windowAgent->setWindowAttribute(QStringLiteral("acrylic-material"), false);
windowAgent->setWindowAttribute(QStringLiteral("dwm-blur"), false);
```

Set the Windows 11 border color:

```cpp
windowAgent->setWindowAttribute(QStringLiteral("dwm-border-color"), QColor("#3367d6"));
```

Disable the system menu:

```cpp
windowAgent->setWindowAttribute(QStringLiteral("no-system-menu"), true);
```

Show the system menu:

```cpp
windowAgent->showSystemMenu(QCursor::pos());
```

For Qt Quick on Windows 10/11 with D3D11 or D3D12, this workaround may help with a top white line. Keep it tied to the documented backend and Qt version constraints:

```cpp
#if defined(Q_OS_WIN) && QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
qputenv("QSG_RHI_BACKEND", "d3d11");
qputenv("QT_QPA_DISABLE_REDIRECTION_SURFACE", "1");
#endif
```

Do not enable `QT_QPA_DISABLE_REDIRECTION_SURFACE` with OpenGL or Vulkan.

## macOS Platform Effects

On macOS, prefer native system buttons unless you have a strong reason to draw your own.

Show native system buttons:

```cpp
windowAgent->setWindowAttribute(QStringLiteral("no-system-buttons"), false);
```

Hide native system buttons:

```cpp
windowAgent->setWindowAttribute(QStringLiteral("no-system-buttons"), true);
```

Enable blur:

```cpp
windowAgent->setWindowAttribute(QStringLiteral("blur-effect"), QStringLiteral("dark"));
windowAgent->setWindowAttribute(QStringLiteral("blur-effect"), QStringLiteral("light"));
windowAgent->setWindowAttribute(QStringLiteral("blur-effect"), QStringLiteral("none"));
```

On macOS 26 and later, Liquid Glass can be enabled when available:

```cpp
windowAgent->setWindowAttribute(QStringLiteral("glass-effect"), QStringLiteral("regular"));
windowAgent->setWindowAttribute(QStringLiteral("glass-corner-radius"), 24.0);
windowAgent->setWindowAttribute(QStringLiteral("glass-tint-color"), QColor(255, 255, 255, 46));
```

If the platform does not support an attribute, `setWindowAttribute()` returns `false`.

## Linux Notes

On Linux, QWindowKit can use the Qt fallback window context and provides partial system menu support on Qt 6 Wayland/X11. Behavior varies by desktop environment and window manager.

For compatibility-first applications, consider:

```cmake
-DQWINDOWKIT_FORCE_QT_WINDOW_CONTEXT=ON
```

## Troubleshooting

### Title bar controls cannot be clicked

Mark the control or its parent container as hit-test visible:

```cpp
windowAgent->setHitTestVisible(button, true);
```

System buttons do not need an extra hit-test-visible call, but they do need a role:

```cpp
windowAgent->setSystemButton(QWK::WindowAgentBase::Close, closeButton);
```

### Windows Snap Layout does not appear

Windows 11 Snap Layout depends on the maximize button role:

```cpp
windowAgent->setSystemButton(QWK::WindowAgentBase::Maximize, maxButton);
```

The window must also keep the maximize capability:

```cpp
setWindowFlag(Qt::WindowMaximizeButtonHint, true);
```

### When should `setup()` be called?

Call it early in the window constructor, before complex child widgets and size constraints when possible:

```cpp
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_agent = new QWK::WidgetWindowAgent(this);
    m_agent->setup(this);

    setMinimumSize(640, 480);
    setupUi();
}
```

In QML, call it in `Component.onCompleted`:

```qml
Component.onCompleted: {
    windowAgent.setup(window)
    windowAgent.setTitleBar(titleBar)
    window.visible = true
}
```

### Can an application switch back to the native frame at runtime?

Do not rely on hot switching. Once QWindowKit has taken over the window frame, recreate the window if you need a different framing mode.

### Can `QDockWidget` use `WidgetWindowAgent`?

No. Install `WidgetWindowAgent` only on top-level windows.

## Recommended Minimal Production Setup

Widgets:

```cpp
QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
QApplication app(argc, argv);
```

Quick:

```cpp
QQuickWindow::setDefaultAlphaBuffer(true);
QGuiApplication app(argc, argv);
QWK::registerTypes(&engine);
```

Do not copy debug environment variables from the examples into production code unless you explicitly need them.
