# QWindowKit

Cross-platform window customization framework for Qt Widgets and Qt Quick.

This project inherited most of [FramelessHelper](https://github.com/wangwenx190/framelesshelper)'s implementation, with a complete refactoring and upgrading of the architecture.

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

+ [qmsetup](https://github.com/stdware/qmsetup)

## Quick Start

### Initialization

First of all, you're supposed to add the following code in your `main` function in a very early stage (MUST before the construction of any `Q(Gui|Core)Application` objects).

```c++
int main(int argc, char *argv[]) {
#ifdef Q_OS_WINDOWS
    qApp->setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
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

See [examples](examples) for more demo use cases.

## Documentatons

+ Examples (TODO)
+ [Framelesshelper Related](docs/framelesshelper-related.md)

## License

QWindowKit is licensed under the [Apache 2.0 License](LICENSE).