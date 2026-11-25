# 开发说明

**语言：** 简体中文 | [English](./Development.md)

本文档记录 QWindowKit 的项目结构、构建系统的设计决定、测试方式与已知问题。

## 项目结构

QWindowKit 以应用自行绘制的标题栏替换系统标题栏，同时保留窗口的移动、缩放、系统菜单以及 Windows 的 Snap Layout 等系统行为。对外提供 QWKCore、QWKWidgets 与 QWKQuick 三个库。

| 目录 | 内容 |
|---|---|
| `src/core` | QWKCore，包含平台无关的部分与每个平台各一份的窗口上下文 |
| `src/core/shared` | 跨平台共用的小型组件 |
| `src/widgets` | QWKWidgets，将 core 的 agent 接到 `QWidget` 上的薄封装 |
| `src/quick` | QWKQuick，将 core 的 agent 接到 `QQuickWindow` 上的薄封装，与 `src/widgets` 互不依赖 |
| `cmake` | 构建函数，一个文件提供一项功能 |
| `share` | 面向不使用 CMake 的用户的集成文件，即 qmake 的 `.pri` 与 Visual Studio 的 `.props`，由 `share/install.cmake` 在安装时生成 |
| `examples` | 可运行的示例，使用其自身的 `qwk_add_example`，不经过 `src/` 的构建函数 |
| `tests/buildsystems` | 消费者构建测试，见「测试」 |
| `tests/auto`、`tests/manual` | 尚为空，留给库自身的用例 |
| `docs` | 教程、待办事项与本文档 |

日常构建不涉及 `share/` 中的文件，只有 `tests/buildsystems/` 使用这些文件。

### 窗口上下文

平台差异集中在 `AbstractWindowContext` 的各个实现中，实现位于 `src/core/contexts/`。

| 平台 | 实现 |
|---|---|
| Windows | `win32windowcontext.cpp` |
| macOS | `cocoawindowcontext.mm` |
| Linux X11 | `linuxx11context.cpp` |
| Linux Wayland | `linuxwaylandcontext.cpp` |
| 兜底 | `qtwindowcontext.cpp`，以纯 Qt 模拟 |

## 构建系统

构建所需的函数全部位于 `cmake/`，`src/conf.cmake` 只设置项目常量并在目录作用域查找 Qt。配置不需要任何外部的 CMake 包，也没有子模块。

构建输出的根目录为 `QWINDOWKIT_BUILD_DIR`，默认 `<构建目录>/out`。父项目改变输出位置的唯一方式是事先设置该变量。

### 设计决定

| 决定 | 日期 | 理由 |
|---|---|---|
| 移除 qmsetup，构建函数改由 `cmake/` 提供 | 2026-11-04 | 作者决定，同日完成 |
| `QWINDOWKIT_BUILD_DIR` 不跟随父项目的 `CMAKE_*_OUTPUT_DIRECTORY`，也不读取 `QMSETUP_BUILD_DIR` | 2026-11-04 | 作者决定 |
| `cmake/` 不随包安装 | 2026-11-04 | 消费者不调用本仓库的构建函数 |
| `examples/CMakeLists.txt` 中的 `qwk_add_example` 暂时保留 | 2026-11-04 | 作者决定 |
| `cmake_parse_arguments` 的关键字以三行 `set()` 声明，不直接写字面量 | 2026-11-04 | 作者要求 |
| 公开接口的文档注释保持 qdoc 风格，不迁移到 LLVM 的 `///` 风格 | 2026-11-04 | 作者决定 |

### CMake 函数的组织

新增或替换的 CMake 函数沿用 stdware/stdcorelib 的约定：一个文件提供一项功能，放在根目录的 `cmake/` 下，文件名为大驼峰并以功能命名，例如 `AddWinRC.cmake`、`AddInstallPackageRule.cmake`，由根目录的 `CMakeLists.txt` 显式 `include()`。

### 生成文件

生成文件在内容未变时不得重写，否则依赖该文件的翻译单元全部重新编译。实现方式是读取已有文件并比较，内容不同时才 `file(WRITE)`，不先删除目录再全部写出。`file(CONFIGURE)` 与 `configure_file()` 先写临时文件再比较，2000 个文件时实测耗时 6.6 s，因此不用于批量生成。

### CMake 文件头

文件开头以 `#[[ ... ]]` 写明调用形式与使用前提。第一行以动词开头说明功能，其后给出调用形式，可选参数加方括号，再说明默认值、相对路径的解析方式、不生效的情况及其理由。依赖其他 `cmake/` 文件时，最后以 `Requires` 逐一列出。以 `cmake/AddWinRC.cmake` 为例：

```cmake
#[[
# Adds Windows version information and an optional icon to an executable or a shared library.
#
# qwk_add_win_rc(<target>
#     [NAME <name>]
#     [VERSION <version>]
#     [DESCRIPTION <description>]
#     [COPYRIGHT <copyright>]
#     [ICON <ico>]
# )
#
# NAME defaults to the target name, VERSION to PROJECT_VERSION, DESCRIPTION to
# PROJECT_DESCRIPTION, and COPYRIGHT to NAME. A relative ICON is resolved against the current
# source directory. The function has no effect on other platforms and on other target types,
# because a static library has no binary of its own to carry the resource.
#
# A changed icon compiles the resource again at the next build without a configure.
#
# Requires cmake/WriteIfDifferent.cmake.
#]]
```

### 参数解析

`cmake_parse_arguments` 之前以 `set(options ...)`、`set(oneValueArgs ...)`、`set(multiValueArgs ...)` 三行声明关键字，再以 `"${options}" "${oneValueArgs}" "${multiValueArgs}"` 传入。函数与宏均适用。前缀一律为 `FUNC`。`FUNC_UNPARSED_ARGUMENTS` 非空时以 `FATAL_ERROR` 拒绝，必需参数缺失时同样报错，原因见「已知问题」中关于 `cmake_parse_arguments` 的条目。

### 安装布局

安装布局是对外的承诺。下游的 `find_package`、`.pri` 与 `.props` 都依赖安装树中的路径，因此修改构建系统之后须将安装树逐文件比对，不能只确认能够编译。

## C++

### 导出宏

导出宏定义在各模块的 global 头文件中。`qwk_add_library` 按 `MACRO_PREFIX` 定义 `<前缀>_LIBRARY` 与 `<前缀>_STATIC` 两个编译定义。

| 模块 | 头文件 | 导出宏 | 编译定义 |
|---|---|---|---|
| QWKCore | `src/core/qwkglobal.h` | `QWK_CORE_EXPORT` | `QWK_CORE_LIBRARY`、`QWK_CORE_STATIC` |
| QWKWidgets | `src/widgets/qwkwidgetsglobal.h` | `QWK_WIDGETS_EXPORT` | `QWK_WIDGETS_LIBRARY`、`QWK_WIDGETS_STATIC` |
| QWKQuick | `src/quick/qwkquickglobal.h` | `QWK_QUICK_EXPORT` | `QWK_QUICK_LIBRARY`、`QWK_QUICK_STATIC` |

### 私有头文件

私有类与私有头文件以 `_p.h` 结尾。私有头文件会被安装到 `include/QWindowKit/<模块>/private/` 下，因此修改私有头文件同样影响下游。

### 编译期开关

编译期开关在 `src/CMakeLists.txt` 中声明，生成到 `QWKCore/qwkconfig.h`，值为 1 或 -1。源码中以 `#if QWINDOWKIT_CONFIG(ENABLE_XXX)` 读取。`QWINDOWKIT_CONFIG` 定义在 `qwkglobal.h` 中，展开为 `((1 / QWINDOWKIT_##feature) == 1)`，因此对未定义的名称报错。

## 测试

`QWINDOWKIT_BUILD_TESTS` 启用测试，由 CTest 运行。

`tests/buildsystems/` 是消费者构建测试。测试先安装一份到构建树中的前缀目录，再分别以 CMake、qmake 和 MSBuild 编译一个小程序并链接到该安装树，验证包配置、`.pri` 与 `.props` 中的路径和库名。

测试规则的理由如下：

| 规则 | 理由 |
|---|---|
| 确认测试通过之前，先确认构建的退出码为 0 | 构建失败时 ctest 运行上一次构建的旧程序，给出虚假的通过结果 |
| 新增测试后确认断言确实被执行 | 空的测试集同样报告通过 |
| 完成一项行为的测试后，将缺陷放回并确认测试失败 | 测试通过不等于测试有效，测试可能只是将缺陷固化 |
| 批量修改后核对修改的处数 | `grep -o <pattern> \| wc -l` 统计实际出现次数，`grep -c` 统计行数 |

## 已知问题

### CMake

- **`cmake_parse_arguments` 对不认识的关键字不报错**，全部归入 `UNPARSED_ARGUMENTS`。迁移构建 API 时写错一个关键字名，对应的行为即静默丢失。检查 `UNPARSED_ARGUMENTS` 也只能发现一部分错误：写错的关键字位于多值关键字之后时，被当作该关键字的值，不进入 `UNPARSED_ARGUMENTS`。实测（2026-11-04）：`qwk_add_example` 的调用中 `SOURCES` 之后的 `QT_LINK` 并入 `SOURCES`，错误由 `target_sources` 以「找不到源文件 QT_LINK」报出。值不被进一步检查的多值关键字（如 `PREDEFINED`）中，这类错误不会被发现。
- **未加引号的变量展开会被参数解析截断。** `DESCRIPTION ${VAR}` 中的 `VAR` 含空格时被拆为多个参数，`oneValueArgs` 只接收第一个词。Windows 资源中的描述曾因此只剩第一个词，长期未被发现。
- **构建树的 `out/include/QWKCore/` 同时存放转发头与 `qwkconfig.h`。** `qwk_sync_headers` 只删除自身上次写出的转发头，从不清空目录。曾经的 `qm_sync_include(... FORCE)` 先删除整个目录，`qwkconfig.h` 随之被删除并在每次配置时重新写出，导致每次重新配置后 6 个翻译单元重新编译。
- **configure 期写入磁盘的路径不能含生成器表达式**，否则生成名称中带尖括号的目录。qmsetup 的 `QMSETUP_BUILD_DIR` 默认含 `$<CONFIG>`，曾因此需要回退分支。`QWINDOWKIT_BUILD_DIR` 下的转发头、配置头与 qmake、MSBuild 文件都在 configure 期写出，因此根目录对含生成器表达式的值报错。
- **`qwk_configure_target` 是 function，其中不得调用 `find_package`。** Qt 5 的私有头目录变量只存在于创建其目标的那一次 `find_package` 的作用域中。在 function 中查找时，变量随 function 返回而消失，目标却保留，第二个目标因此既不再查找，也取不到这些变量。Qt 的模块在使用它的目录或其上级目录的目录作用域中查找：库在 `src/conf.cmake`，示例在 `examples/CMakeLists.txt`，测试在 `tests/buildsystems/CMakeLists.txt`。`src/` 中找到的导入目标与 `QT_VERSION_MAJOR` 不传到同级目录，因此三处各自查找。`qwk_configure_target` 对 `QT_LINKS` 中未找到的模块报错，Private 模块未找到时改用私有头目录。
- **CMake 中判断平台而非编译器。** clang 以 `x86_64-pc-windows-msvc` 为目标时，CMake 的 `MSVC` 与 `MINGW` 均为假，写在 `else()` 分支中的非 Windows 选项会被错误地应用。
- **CMake 的 `file(WRITE)` 在 Windows 上以文本模式写出 CRLF，`file(READ)` 读回时还原为 LF。** 因此生成文件的「读取并比较」在 Windows 上仍然成立，生成的头文件与 `qmcorecmd` 的输出同样是 CRLF。比较两个生成文件的内容时，须对两侧做同样的行尾处理。

### Windows 工具链

- **非交互 shell 中不要拼接 `cmd /c "xxx.bat && ..."`。** 引号经过 Git Bash 或 PowerShell 后会被重新拆分，cmd 收到不完整的命令后打开交互式 shell 并停在该处。应将命令写入 `.bat` 文件再执行。
- **`.bat` 中不要写中文。** cmd 按 ANSI 代码页读取文件，UTF-8 的注释会被解码为乱码并被当作命令执行。
- **Qt 的头文件要求 `/Zc:__cplusplus` 与 `/permissive-`。** qmake 的 mkspec 会自动添加这两个选项，手写 vcxproj 或其他 MSBuild 工程时须自行添加，否则编译会在 Qt 自身的头文件中失败。

### 文本处理

- **`sed -i` 会将 CRLF 转换为 LF。** 修改前用 `grep -qU $'\r'` 判断行尾，CRLF 文件改用编辑工具修改。
- **`sed` 的替换表达式必须带行号地址**，否则会进行全局替换，且后续表达式会匹配前面已修改的文本。
