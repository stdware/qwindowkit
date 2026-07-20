# QWindowKit

**语言：** 简体中文 | [English](./README.md)

**教程：** [简体中文教程](./docs/tutorial.zh-CN.md) | [English tutorial](./docs/tutorial.en-US.md)

QWindowKit 是一个面向 Qt Widgets 和 Qt Quick 的跨平台窗口定制框架。它可以帮助应用使用自定义标题栏替换系统标题栏，同时尽量保留窗口移动、缩放、系统菜单、Windows Snap Layout 等重要系统行为。

本项目继承了 [wangwenx190 FramelessHelper](https://github.com/wangwenx190/framelesshelper) 的核心思路和部分实现脉络，并在架构和 API 设计上进行了重构。

## 功能特性

- 支持 Qt Widgets 和 Qt Quick 自定义标题栏。
- 配置系统按钮角色后，支持 Windows 11 Snap Layout。
- 支持 Windows 10/11 无边框窗口处理，并提供可选的系统边框 workaround。
- 支持 macOS 原生系统按钮位置控制，以及可选的模糊和玻璃效果。
- 支持 Linux fallback 实现，并在 Qt 6 Wayland/X11 环境中提供部分原生系统菜单能力。
- 提供 CMake package、qmake 集成文件和 Visual Studio property 文件。

## 支持平台

- Microsoft Windows。
- Apple macOS 11 或更高版本。
- GNU/Linux。

## 截图

### Windows 11

![Windows 11](./docs/images/win11.png)

### Windows 10

![Windows 10](./docs/images/win10.png)

### macOS

![macOS](./docs/images/mac.png)

| 默认 | 玻璃效果 - regular |
|:--:|:--:|
| ![Default](./docs/images/macos/01%20default.png) | ![Glass regular](./docs/images/macos/02%20glass%20-%20regular.png) |
| 玻璃效果 - clear | 玻璃效果 - regular, rounded |
| ![Glass clear](./docs/images/macos/03%20glass%20-%20clear.png) | ![Glass regular rounded](./docs/images/macos/04%20glass%20-%20regular,%20rounded.png) |
| 玻璃效果 - regular, dark tint | 玻璃效果 - regular, light tint |
| ![Glass dark tint](./docs/images/macos/05%20glass%20-%20regular,%20dark%20tint.png) | ![Glass light tint](./docs/images/macos/06%20glass%20-%20regular,%20light%20tint.png) |
| 旧版效果 - dark blur | 旧版效果 - light blur |
| ![Legacy dark blur](./docs/images/macos/07%20legacy%20-%20dark%20blur.png) | ![Legacy light blur](./docs/images/macos/08%20legacy%20-%20light%20blur.png) |

### Linux

![Linux](./docs/images/linux.png)

## 环境要求

| 组件 | 要求 | 说明 |
|:--|:--|:--|
| Qt | 5.12 或更高版本 | 必需 Qt Core 和 Gui。Widgets 与 Quick 是可选模块。 |
| C++ | C++17 或更高版本 | 已测试 MSVC、GCC 和 Clang。 |
| CMake | 3.19 或更高版本 | 推荐 CMake 3.20 或更高版本。 |

推荐 Qt 版本：

- Qt 5：5.15.2 或更高版本。
- Qt 6：6.6.2 或更高版本。

QWindowKit 使用了 Qt 私有 API 和平台相关窗口行为。较旧 Qt 版本可能可以编译，但运行时仍可能受到 Qt 内部实现或平台插件问题影响。

已测试编译器：

- Windows：MSVC 2019、MSVC 2022、MinGW GCC 13.2.0。
- macOS：Clang 14.0.3。
- Ubuntu：GCC 9.4.0。

## 模块

| 模块 | Target | 用途 |
|:--|:--|:--|
| Core | `QWindowKit::Core` | 共享的平台窗口基础设施。 |
| Widgets | `QWindowKit::Widgets` | 用于 `QWidget` 和 `QMainWindow`。 |
| Quick | `QWindowKit::Quick` | 用于 `QQuickWindow` 和 QML `Window`。 |

`QWINDOWKIT_BUILD_WIDGETS` 默认启用。`QWINDOWKIT_BUILD_QUICK` 默认关闭，如果需要 Qt Quick 支持，需要显式启用。

## 构建和安装

克隆仓库及 submodule：

```sh
git clone --recursive https://github.com/stdware/qwindowkit
cd qwindowkit
```

配置、构建并安装：

```sh
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=<QT_DIR> \
  -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> \
  -DQWINDOWKIT_BUILD_WIDGETS=ON \
  -DQWINDOWKIT_BUILD_QUICK=ON

cmake --build build --config Release
cmake --install build --config Release
```

项目依赖 `qmsetup`。如果 CMake 找不到已安装的 `qmsetup` package，会自动使用仓库中的 submodule。

## 集成到你的项目

### CMake

配置应用时指定安装后的 package 路径：

```sh
cmake -S . -B build -DQWindowKit_DIR=<INSTALL_DIR>/lib/cmake/QWindowKit
```

Widgets 应用：

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(QWindowKit REQUIRED COMPONENTS Widgets)

target_link_libraries(my_widgets_app PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    QWindowKit::Widgets
)
```

Quick 应用：

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml)
find_package(QWindowKit REQUIRED COMPONENTS Quick)

target_link_libraries(my_quick_app PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    QWindowKit::Quick
)
```

如果你将 QWindowKit 作为源码依赖放入项目，也可以通过 CMake `add_subdirectory()` 集成。

### qmake

先使用 CMake 安装 QWindowKit，然后包含生成的 `.pri` 文件：

```qmake
# Widgets
include("<INSTALL_DIR>/share/QWindowKit/qmake/QWKWidgets.pri")

# Quick
include("<INSTALL_DIR>/share/QWindowKit/qmake/QWKQuick.pri")
```

### Visual Studio

MSBuild 项目请参考 [Visual Studio Guide](./docs/visual-studio-guide.md)。

## 快速开始：Qt Widgets

在构造 `QApplication` 前设置 `Qt::AA_DontCreateNativeWidgetSiblings`：

```cpp
#include <QtCore/QCoreApplication>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    QApplication app(argc, argv);
    // 创建并显示主窗口。
    return app.exec();
}
```

为每个需要自定义边框的顶层 widget 安装 `WidgetWindowAgent`：

```cpp
#include <QWKWidgets/widgetwindowagent.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto *agent = new QWK::WidgetWindowAgent(this);
    agent->setup(this);

    auto *titleBar = new QWidget(this);
    setMenuWidget(titleBar);

    agent->setTitleBar(titleBar);
}
```

注册系统按钮角色，使 Windows Snap Layout 等原生能力可以正常工作：

```cpp
agent->setSystemButton(QWK::WindowAgentBase::WindowIcon, iconButton);
agent->setSystemButton(QWK::WindowAgentBase::Minimize, minimizeButton);
agent->setSystemButton(QWK::WindowAgentBase::Maximize, maximizeButton);
agent->setSystemButton(QWK::WindowAgentBase::Close, closeButton);
```

按钮点击行为需要自行连接：

```cpp
connect(minimizeButton, &QPushButton::clicked, this, &QWidget::showMinimized);
connect(maximizeButton, &QPushButton::clicked, this, [this] {
    isMaximized() ? showNormal() : showMaximized();
});
connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
```

标题栏中的交互控件需要标记为 hit-test visible：

```cpp
agent->setHitTestVisible(menuBar, true);
agent->setHitTestVisible(searchBox, true);
```

标题栏中未标记为 hit-test visible 的区域会被视为窗口拖拽区域。

## 快速开始：Qt Quick

加载 QML 前注册 QML 类型：

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

    return app.exec();
}
```

在 QML 中使用 `WindowAgent`：

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QWindowKit 1.0

Window {
    id: window
    width: 900
    height: 600
    visible: false

    WindowAgent {
        id: windowAgent
    }

    Rectangle {
        id: titleBar
        anchors.top: parent.top
        width: parent.width
        height: 40
    }

    Component.onCompleted: {
        windowAgent.setup(window)
        windowAgent.setTitleBar(titleBar)
        window.visible = true
    }
}
```

在 Qt 6 中，如果你的 QML 风格允许无版本 import，可以省略 import 版本号。

## 平台属性

`WindowAgentBase::setWindowAttribute()` 提供平台相关能力。不受支持的属性会返回 `false`。

Windows 示例：

```cpp
agent->setWindowAttribute(QStringLiteral("dark-mode"), true);
agent->setWindowAttribute(QStringLiteral("mica"), true);
agent->setWindowAttribute(QStringLiteral("mica-alt"), false);
agent->setWindowAttribute(QStringLiteral("acrylic-material"), false);
agent->setWindowAttribute(QStringLiteral("dwm-blur"), false);
agent->setWindowAttribute(QStringLiteral("dwm-border-color"), QColor("#3367d6"));
```

macOS 示例：

```cpp
agent->setWindowAttribute(QStringLiteral("no-system-buttons"), false);
agent->setWindowAttribute(QStringLiteral("blur-effect"), QStringLiteral("dark"));
agent->setWindowAttribute(QStringLiteral("glass-effect"), QStringLiteral("regular"));
agent->setWindowAttribute(QStringLiteral("glass-corner-radius"), 24.0);
agent->setWindowAttribute(QStringLiteral("glass-tint-color"), QColor(255, 255, 255, 46));
```

更完整的示例请参考[简体中文教程](./docs/tutorial.zh-CN.md)。

## 重要说明

### Qt 版本兼容性

QWindowKit 依赖 Qt 私有实现细节来提供更好的无边框窗口行为。项目可能可以在较旧 Qt 版本上编译，但不受支持的 Qt 版本仍可能因为 Qt 内部实现或平台插件问题导致运行时异常。

建议尽量使用 Qt 5.15.2+ 或 Qt 6.6.2+。

### setup 调用时机

尽量尽早调用 `setup()`，通常应放在顶层窗口构造函数的前半段。如果窗口需要最小尺寸、最大尺寸或固定尺寸约束，建议在 QWindowKit 初始化后再设置。

### 运行时切换窗口边框

不要在运行时切回系统原生边框。如果应用需要在原生边框和自定义边框之间切换，应销毁并重建窗口。

`WidgetWindowAgent::setup()` 之后不要再通过 `QWidget::setWindowFlags()` 修改窗口 flags。

### 原生子控件

如果子 widget 使用 `Qt::WA_NativeWindow`，应在创建原生祖先窗口前启用 `Qt::WA_DontCreateNativeAncestors`。

### 尺寸约束

固定尺寸窗口是受支持的。避免最大化设置了最大宽度或最大高度约束的窗口，因为 Qt 可能无法正确报告原生最大化几何信息。

### Windows 10 和 Qt Quick

Windows 10 顶部边框 workaround 适用于 Qt Widgets，也适用于使用 OpenGL、D3D11 或 D3D12 渲染的 Qt Quick。Vulkan 存在已知限制。

对于使用 D3D11/D3D12 的 Qt Quick，部分环境中窗口顶部可能出现白线。在 Qt 6.7 或更高版本中，可以在创建 `QCoreApplication` 前设置 `QT_QPA_DISABLE_REDIRECTION_SURFACE=1` 尝试规避。不要在 OpenGL 或 Vulkan 后端启用该变量。

## 文档

- [English tutorial](./docs/tutorial.en-US.md)
- [简体中文教程](./docs/tutorial.zh-CN.md)
- [Visual Studio Guide](./docs/visual-studio-guide.md)
- [FramelessHelper Related](./docs/framelesshelper-related.md)
- [示例](./examples)

## 社区

- Discord：<https://discord.gg/grrM4Tmesy>
- 中文用户 QQ 群：876419693

## 鸣谢

- [Maplespe](https://github.com/Maplespe)
- [zhiyiYo](https://github.com/zhiyiYo)

## 许可证

QWindowKit 使用 [Apache License 2.0](./LICENSE) 许可证。
