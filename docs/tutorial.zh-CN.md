# QWindowKit 使用教程

**语言：** 简体中文 | [English](./tutorial.en-US.md)

**项目 README：** [简体中文](../README.zh-CN.md) | [English](../README.md)

本文面向希望在 Qt Widgets 或 Qt Quick 应用中自定义系统标题栏、实现无边框窗口、保留拖拽/缩放/系统菜单等原生行为的用户。

QWindowKit 分为三个模块：

- `QWindowKit::Core`：公共基础和平台窗口上下文。
- `QWindowKit::Widgets`：用于 `QWidget`/`QMainWindow`。
- `QWindowKit::Quick`：用于 `QQuickWindow`/QML `Window`。

## 环境要求

- Qt 5.12 或更高版本。实际项目建议 Qt 5.15.2+ 或 Qt 6.6.2+。
- C++17 编译器。
- CMake 3.19 或更高版本。
- Windows、macOS 或 Linux。

## 构建和安装

从源码构建并安装：

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

Linux/macOS 写法通常是：

```sh
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/Qt \
  -DCMAKE_INSTALL_PREFIX=/opt/qwindowkit \
  -DQWINDOWKIT_BUILD_WIDGETS=ON \
  -DQWINDOWKIT_BUILD_QUICK=ON

cmake --build build --config Release
cmake --install build --config Release
```

常用构建选项：

```cmake
-DQWINDOWKIT_BUILD_WIDGETS=ON
-DQWINDOWKIT_BUILD_QUICK=ON
-DQWINDOWKIT_BUILD_EXAMPLES=ON
-DQWINDOWKIT_BUILD_STATIC=OFF
-DQWINDOWKIT_FORCE_QT_WINDOW_CONTEXT=OFF
-DQWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS=ON
```

## 在 CMake 项目中使用

Widgets 应用：

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

Quick 应用：

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

配置下游项目时指定安装路径：

```sh
cmake -S . -B build -DQWindowKit_DIR=C:/Libraries/QWindowKit/lib/cmake/QWindowKit
```

## Widgets：最小可用窗口

Widgets 应用必须在创建 `QApplication` 前设置 `Qt::AA_DontCreateNativeWidgetSiblings`。

`main.cpp`：

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

`mainwindow.h`：

```cpp
#pragma once

#include <QtWidgets/QMainWindow>

namespace QWK {
class WidgetWindowAgent;
}

class QPushButton;
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

`mainwindow.cpp`：

```cpp
#include "mainwindow.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
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
    titleBar->setObjectName(QStringLiteral("TitleBar"));
    titleBar->setFixedHeight(40);

    auto *iconButton = new QPushButton(this);
    iconButton->setText(QStringLiteral("App"));

    auto *titleLabel = new QLabel(windowTitle(), this);
    titleLabel->setAlignment(Qt::AlignCenter);

    auto *menuBar = new QMenuBar(this);
    auto *fileMenu = menuBar->addMenu(tr("&File"));
    fileMenu->addAction(tr("E&xit"), this, &QWidget::close);

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

要点：

- 每个顶层窗口都需要自己的 `WidgetWindowAgent`。
- `setup(this)` 越早调用越好，尤其是窗口有最小/最大尺寸约束时。
- `setTitleBar()` 指定可拖拽标题栏区域。
- 标题栏内需要接收鼠标事件的控件必须调用 `setHitTestVisible(widget, true)`。
- 系统按钮提示只告诉 QWindowKit 这些控件的角色；按钮点击行为仍需要自己连接。

## Widgets：常见标题栏控件

菜单栏应设置为 hit-test visible：

```cpp
auto *menuBar = new QMenuBar(this);
menuBar->addMenu(tr("&File"));

windowAgent->setHitTestVisible(menuBar, true);
```

搜索框、标签页、工具按钮也一样：

```cpp
windowAgent->setHitTestVisible(searchEdit, true);
windowAgent->setHitTestVisible(tabBar, true);
windowAgent->setHitTestVisible(settingsButton, true);
```

如果不这样做，这些控件所在区域会被当成拖拽区域，用户无法正常点击控件。

## Widgets：窗口按钮行为

QWindowKit 不会自动连接按钮行为。建议显式连接：

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

禁用最大化：

```cpp
setWindowFlag(Qt::WindowMaximizeButtonHint, false);
```

固定大小窗口：

```cpp
setFixedSize(640, 420);
```

## Quick：注册 QML 类型

`main.cpp`：

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

`QQuickWindow::setDefaultAlphaBuffer(true)` 对 Windows/macOS 的透明、模糊、Mica、玻璃等效果通常是必要的，应该在创建 Quick 窗口前调用。

## Quick：最小 QML 窗口

`Main.qml`：

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
                    text: window.visibility === Window.Maximized ? "□" : "□"
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

如果标题栏内有可点击控件，例如菜单按钮或搜索框，需要设置 hit-test visible：

```qml
Button {
    id: menuButton
    text: "Menu"
}

Component.onCompleted: {
    windowAgent.setHitTestVisible(menuButton, true)
}
```

## Windows 平台效果

Windows 支持一些平台属性。使用前应先完成 `setup()`。

```cpp
windowAgent->setWindowAttribute(QStringLiteral("dark-mode"), true);
windowAgent->setWindowAttribute(QStringLiteral("mica"), true);
windowAgent->setWindowAttribute(QStringLiteral("mica-alt"), false);
windowAgent->setWindowAttribute(QStringLiteral("acrylic-material"), false);
windowAgent->setWindowAttribute(QStringLiteral("dwm-blur"), false);
```

设置 Windows 11 边框颜色：

```cpp
windowAgent->setWindowAttribute(QStringLiteral("dwm-border-color"), QColor("#3367d6"));
```

关闭系统菜单：

```cpp
windowAgent->setWindowAttribute(QStringLiteral("no-system-menu"), true);
```

显示系统菜单：

```cpp
windowAgent->showSystemMenu(QCursor::pos());
```

Qt Quick 在 Windows 10/11 使用 D3D11/D3D12 时，如果遇到顶部白线，可按 README 的限制条件设置：

```cpp
#if defined(Q_OS_WIN) && QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
qputenv("QSG_RHI_BACKEND", "d3d11");
qputenv("QT_QPA_DISABLE_REDIRECTION_SURFACE", "1");
#endif
```

不要在 OpenGL 或 Vulkan 后端下启用 `QT_QPA_DISABLE_REDIRECTION_SURFACE`。

## macOS 平台效果

默认建议使用 macOS 系统按钮，不必设置自绘关闭/最小化/缩放按钮。

显示系统按钮：

```cpp
windowAgent->setWindowAttribute(QStringLiteral("no-system-buttons"), false);
```

隐藏系统按钮：

```cpp
windowAgent->setWindowAttribute(QStringLiteral("no-system-buttons"), true);
```

启用模糊：

```cpp
windowAgent->setWindowAttribute(QStringLiteral("blur-effect"), QStringLiteral("dark"));
windowAgent->setWindowAttribute(QStringLiteral("blur-effect"), QStringLiteral("light"));
windowAgent->setWindowAttribute(QStringLiteral("blur-effect"), QStringLiteral("none"));
```

macOS 26 及更高版本可尝试 Liquid Glass：

```cpp
windowAgent->setWindowAttribute(QStringLiteral("glass-effect"), QStringLiteral("regular"));
windowAgent->setWindowAttribute(QStringLiteral("glass-corner-radius"), 24.0);
windowAgent->setWindowAttribute(QStringLiteral("glass-tint-color"), QColor(255, 255, 255, 46));
```

如果平台不支持，`setWindowAttribute()` 会返回 `false`。

## Linux 说明

Linux 下 QWindowKit 优先使用 Qt 窗口上下文，并在 Qt 6 的 Wayland/X11 环境中提供部分系统菜单支持。不同桌面环境和窗口管理器行为差异较大，建议：

```cmake
-DQWINDOWKIT_FORCE_QT_WINDOW_CONTEXT=ON
```

在兼容性优先的应用中可以强制使用 Qt fallback 实现。

## 常见问题

### 标题栏按钮无法点击

把按钮或其父容器设置为 hit-test visible：

```cpp
windowAgent->setHitTestVisible(button, true);
```

系统按钮不需要额外设置 hit-test visible，但需要设置 system button 角色：

```cpp
windowAgent->setSystemButton(QWK::WindowAgentBase::Close, closeButton);
```

### Snap Layout 不显示

Windows 11 的 Snap Layout 依赖最大化按钮的 system button 提示：

```cpp
windowAgent->setSystemButton(QWK::WindowAgentBase::Maximize, maxButton);
```

同时窗口必须保留最大化能力：

```cpp
setWindowFlag(Qt::WindowMaximizeButtonHint, true);
```

### 什么时候调用 `setup()`

尽量在窗口构造函数前半段调用，早于尺寸约束和复杂子控件初始化：

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

Quick 中通常在 `Component.onCompleted` 调用：

```qml
Component.onCompleted: {
    windowAgent.setup(window)
    windowAgent.setTitleBar(titleBar)
    window.visible = true
}
```

### 可以在运行时切回系统标题栏吗

不建议。QWindowKit 接管窗口后，不支持无损热切换回系统标题栏。需要销毁并重建窗口。

### 可以对 `QDockWidget` 使用吗

不建议。`WidgetWindowAgent` 只应安装在顶层窗口上。

## 推荐的最小生产配置

Widgets：

```cpp
QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
QApplication app(argc, argv);
```

Quick：

```cpp
QQuickWindow::setDefaultAlphaBuffer(true);
QGuiApplication app(argc, argv);
QWK::registerTypes(&engine);
```

不要把示例中的调试环境变量直接复制到生产应用，除非你明确需要它们。
