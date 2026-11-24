# 与本仓库协作的约定

## 项目概述

QWindowKit 以应用自行绘制的标题栏替换系统标题栏，同时保留窗口的移动、缩放、系统菜单以及 Windows 的 Snap Layout 等系统行为。对外提供三个库。

`src/core` 产出 QWKCore，包含平台无关的部分与每个平台各一份的窗口上下文。`src/widgets` 与 `src/quick` 各是一层薄封装，分别将 core 的 agent 接到 `QWidget` 与 `QQuickWindow` 上，二者互不依赖。

平台差异集中在 `AbstractWindowContext` 的各个实现中：Windows 为 `win32windowcontext.cpp`，macOS 为 `cocoawindowcontext.mm`，Linux 分为 `linuxx11context.cpp` 与 `linuxwaylandcontext.cpp`，另有纯 Qt 模拟的 `qtwindowcontext.cpp` 作为兜底。**修改其中一处的行为之前，先确认其他实现是否需要同步修改，以及兜底实现是否会因此与它们不一致。**

`share/` 是面向不使用 CMake 的用户的集成文件，即 qmake 的 `.pri` 与 Visual Studio 的 `.props`，由 `share/install.cmake` 在安装时生成。日常构建不涉及这些文件，只有 `tests/buildsystems/` 会使用它们。

`examples/` 是可运行的示例，使用其自身的 `qwk_add_example`，不经过 `src/` 的构建函数。

## 文档位置

| 内容 | 位置 |
|---|---|
| 计划、交接、调研与临时分析 | `.cache/claude/YYYY-MM-DD-HHMM-<主题>.md` |
| 待办事项、已知未验证的问题、不测试的内容及其理由 | `docs/TODO.md` |
| 面向用户的说明 | `README.md`、`README.zh-CN.md`、`docs/tutorial.*.md` |
| 公开接口的参考文档 | 源文件中的 qdoc 注释，由文档 CI 发布到 <https://stdware.github.io/qwindowkit> |

`.cache/` 整体被 gitignore。`.cache/claude/` 中的笔记以中文书写，标题注明日期。**未来要做的事写入 `docs/TODO.md` 或 `.cache/claude/` 的笔记，不要只留在会话中。**

## 构建系统

**qmsetup 已移除**（作者 2026-11-04 决定，同日完成）。构建所需的函数全部位于 `cmake/`，`src/conf.cmake` 只设置项目常量并在目录作用域查找 Qt。配置不需要任何外部的 CMake 包，也没有子模块。

构建输出的根目录为 `QWINDOWKIT_BUILD_DIR`，默认 `<构建目录>/out`。父项目改变输出位置的唯一方式是事先设置该变量，不跟随父项目的 `CMAKE_*_OUTPUT_DIRECTORY`，也不读取 `QMSETUP_BUILD_DIR`（作者 2026-11-04 决定）。

新增或替换的 CMake 函数按 stdcorelib 的方式组织：

- **按功能分文件，放在根目录的 `cmake/` 下。** 一个文件提供一项功能，文件名为大驼峰，以功能命名，例如 `AddWinRC.cmake`、`AddInstallPackageRule.cmake`。由根目录的 `CMakeLists.txt` 显式 `include()`。
- **不要在 `src/CMakeLists.txt` 或各模块的 `CMakeLists.txt` 中定义函数或宏**，也不要为替换 qmsetup 而重新编写一个生成整套函数的 helper。`examples/CMakeLists.txt` 中的 `qwk_add_example` 暂时保留（作者 2026-11-04 决定）。
- **`cmake/` 不随包安装**（作者 2026-11-04 决定），因为消费者不调用本仓库的构建函数。
- **生成文件在内容未变时不得重写**，否则依赖它的翻译单元全部重新编译。做法是读取已有文件、比较、不同时才 `file(WRITE)`，不要先删除目录再全部写出。不要用 `file(CONFIGURE)` 或 `configure_file()` 批量生成，二者先写临时文件再比较，2000 个文件时实测为 6.6 s。
- 文件开头以 `#[[ ... ]]` 写明调用形式与使用前提，格式参照 stdcorelib 的 `cmake/AddWinRC.cmake`。
- `cmake_parse_arguments` 之前以 `set(options ...)`、`set(oneValueArgs ...)`、`set(multiValueArgs ...)` 三行声明关键字，再以 `"${options}" "${oneValueArgs}" "${multiValueArgs}"` 传入，不直接写字面量（作者 2026-11-04 要求）。函数与宏均适用。
- `cmake_parse_arguments` 一律使用 `FUNC` 前缀。**`FUNC_UNPARSED_ARGUMENTS` 非空时以 `FATAL_ERROR` 拒绝**，必需参数缺失时同样报错，原因见「已知问题」的第一条。

选项定义在根目录的 `CMakeLists.txt` 中，使用 `QWINDOWKIT_` 前缀。默认值必须使不传任何参数的配置保持原有行为。

CMake 中，函数和不显然的赋值上方须写明其作用，以及不能使用默认值的理由。`src/conf.cmake` 中的每一处显式赋值都附有理由，删除之前先阅读该注释。

**安装布局是对外的承诺。** 下游的 `find_package`、`.pri` 与 `.props` 都依赖这些路径。修改构建系统之后，须将安装树逐文件比对，不能只确认能够编译。

## C++

- 命名空间为 `QWK`。
- PImpl 采用 Qt 的写法，即 `Q_DECLARE_PRIVATE` 加 `d_ptr`，私有类与私有头文件以 `_p.h` 结尾。**私有头文件会被安装**，位于 `include/QWindowKit/<模块>/private/` 下，修改私有头文件同样影响下游。
- 导出宏为 `QWK_CORE_EXPORT`、`QWK_WIDGETS_EXPORT`、`QWK_QUICK_EXPORT`，按 `MACRO_PREFIX` 定义 `<前缀>_LIBRARY` 与 `<前缀>_STATIC` 两个编译定义，宏本身定义在各模块的 global 头文件中。
- 平台相关的实现放在各自的文件中，不要在一个文件中堆积 `#ifdef`。跨平台共用的小型组件放在 `src/core/shared/`。
- 编译期开关在 `src/CMakeLists.txt` 中声明，生成到 `QWKCore/qwkconfig.h`，值为 1 或 -1。源码中以 `#if QWINDOWKIT_CONFIG(ENABLE_XXX)` 读取。该宏定义在 `qwkglobal.h` 中，有意设计为对未定义的名称报错，**不要绕过它直接使用 `#ifdef`**。
- 格式由 clang-format 处理：100 列，4 空格缩进，命名空间内缩进。不要手动排版。**只对自己改动过的文件运行 clang-format，不要用 `sed -i` 处理整个目录。**

## 文体

本节适用于注释、文档、README、诊断消息与提交信息。

- **采用正式的技术写作文体。** 注释与文档是规范性文本而非叙述。每句陈述一项事实、约束或理由，不写铺垫、感想和修辞。
- **不拟人。** 代码、文件、格式、程序和测试不作为有意志的主语：不写 says、tells、knows、asks、wants、means、cares、decides、promises、is told，也不写「它说」「它知道」「它不认」「它想要」。改用 returns、indicates、records、specifies、reports、detects、requires、rejects，或「返回」「表示」「记录」「规定」「报告」「拒绝」。用户、作者、调用方等真实行为主体可以作主语。
- **使用术语，不用描述性转述。** 写 non-client area，不写 the part of the window that is not drawn by the app。写 hit test、window procedure、forwarding header，不写 the check for what is under the mouse、the function that gets the messages、the header that points at the real one。没有通用术语时，首次出现给出定义，之后始终沿用同一名称。
- **标题、分组名和列表标签使用名词或名词短语。** 写 Motivation、Behavior、Rationale，不写 Why、What it does、How it works。**主语从句不可用**，能用名词短语时一律改用名词短语：写 the requested geometry，不写 what the caller asked for。**宾语从句少用**，从句比名词短语明显冗长时改用名词短语。**定语从句不设限制。**
- **条件用 if。** `where` 作关系副词时照常使用，作条件状语时写 `if`：不写 empty where there is none，写 empty if absent。判据是该处能否换成 `if` 而意思不变。
- **`one` 回指只用于同一个句子。** 同一句内用 `one` 回指前文名词是正常的英语，不必改写。回指前一句或更早句子的名词不可用，须直接重复该名词。
- **不使用口语短语。** 不写 whatever else、for good、as it stands、on its own、on the way out、at a glance、there and back、is given up on、the rest of why、and all 等说法，改为准确的书面表达。
- **句子完整。** 不写片段句、逗号粘连句（两个独立分句仅以逗号连接）和反问句。不以 So、And so、Which is why、That is why、Hence 开头叙述因果，改为在同一句中用 because、therefore 表明。不对读者使用第二人称。
- **函数说明以动词开头**（Returns、Sets、Installs、Rejects）。返回值写明每种情况。布尔查询写 Returns whether …。
- 中文文本同样适用：使用书面语，不用「别」「搞」「就行」「得（表必须）」「啥」「拿来」「反正」「其实」「说白了」「这玩意儿」等口语词。标题不用「为什么」「怎么做」，改用「动机」「设计理由」「实现方式」。「不要」「必须」等规范性祈使句不属于口语，照常使用。
- **记录曾经的实现错误。** 一句话说明某处曾经如何实现错误、因此现在为什么必须如此的记述不属于考古式叙述，必须保留，因为它防止同一处再错一次。要删的是与当前实现无关的历史沿革。
- **有明确重要理由而否定的做法同样保留。** 写明被否定的做法、否定的理由，以及当前做法与之的对照。理由不重要、或无法说清理由的，不写。

## 注释

- **公开接口的文档注释采用 qdoc 风格**，以 `/*! ... */` 写在 `.cpp` 中定义的上方，块内缩进 4 格，由 Doxygen 解析并发布。类的说明以 `\class` 加 `\brief` 开头，没有定义可以依附的声明使用 `\fn`。参数写 `\a`，标识符写 `\c`，引用其他声明写 `\sa`。不要迁移到 LLVM 的 `///` 风格（作者 2026-11-04 决定），新增的文档注释与现有的保持一致。
- 公开头文件中不写文档注释。调用方据以行动的内容写在 qdoc 块中，实现理由以 `//` 写在实现内部，或不写注释。
- **注释中不要使用破折号，也不要用分号连接从句。** 应断句处即断句。使用美式拼写。
- **几个词能说清的内容不写成一段。** 注释说明约束、所有权、生命周期，以及「为何只能如此实现」。不复述签名已经表明的内容，也不将一句话拆成三句。
- **不写考古式注释。** 保留「为何现在必须如此实现」，删除「以前如何、后来修正」。记录曾经的实现错误的注释，以及有明确重要理由而否定的做法，见「文体」，不在删除之列。
- 设计理由中篇幅较长的部分写入 `docs/`，不要写入公开的 qdoc 块。
- 全项目通用的约定写在本文档中，不要在每个使用处重复解释。

## Markdown

- **一段即一行**，不要按 80 或 100 列手动折行。代码块、表格、列表项照常处理。此规则仅适用于 `.md`，C++ 注释仍为 100 列。
- 文件末尾不留多余的空行。

## 命名

- 构建函数使用 `qwk_` 前缀，包括只供 `cmake/` 中其他文件使用的工具函数。`_qwk_` 前缀只用于仅在定义它的文件内调用的函数，如 `cmake/ConfigureTarget.cmake` 的 `_qwk_configure_target_include_qt_private`。qmsetup 的 `qm_`、`_qm_` 与 helper 内部的 `_repo_` 不应再出现于本仓库。
- 选项与缓存变量使用 `QWINDOWKIT_` 前缀。
- 预处理宏按模块分前缀：`QWK_CORE`、`QWK_WIDGETS`、`QWK_QUICK`。
- 类名大驼峰，成员小驼峰，私有类加 `Private` 后缀，与 Qt 一致。

## 测试

`QWINDOWKIT_BUILD_TESTS` 启用测试，由 CTest 运行。

`tests/buildsystems/` 是消费者构建测试：先安装一份到构建树中的前缀目录，再分别以 CMake、qmake 和 MSBuild 编译一个小程序并链接到该安装树，验证的是包配置、`.pri` 与 `.props` 中的路径和库名。`tests/auto` 与 `tests/manual` 尚为空，留给库自身的用例。

- 路径、工具位置与配置一律由 `-D` 传入，测试脚本中不写死。
- 每个用例在构建树中使用独立的工作目录，运行前先清空，用例之间不共享状态。
- **平台或工具不具备时跳过并说明原因。** 不要硬编码平台判断，也不要保留必然失败的用例。
- 不测试的内容及其理由写入 `docs/TODO.md`。
- **确认测试通过之前，先确认构建的退出码为 0。** 构建失败时 ctest 运行的是上一次构建的旧程序，会给出虚假的通过结果。
- 新增测试后确认断言确实被执行，空的测试集同样会「通过」。
- 区分断言的是当前行为还是设计意图。测试可能只是将缺陷固化了下来。
- **完成一项行为的测试后，将缺陷放回，确认测试会失败。** 修改一处期望或故意传入错误路径，重新构建、运行、还原。测试通过不等于测试有效。
- **批量修改后核对「修改了几处」，而不只是「能否编译」。** `grep -o <pattern> | wc -l` 统计的是实际出现次数，`grep -c` 统计的是行数。

## 提交

- **提交信息不带任何 AI 署名**，不写 `Co-Authored-By`，不写 `Generated with`。
- **只写一行**，格式为 `<类型>(<范围>): <描述>`，范围可省略，例如 `fix(win32): preserve left-button state when forwarding caption clicks`、`ci: check out submodules recursively`。类型使用 `feat`、`fix`、`refactor`、`style`、`docs`、`build`、`ci`、`test`、`chore`，范围为平台或模块，如 `win32`、`macOS`、`linux`、`core`、`widgets`、`quick`、`example`。描述使用英文、小写开头的祈使句和美式拼写，不写正文。设计理由写在代码注释和 `docs/` 中，不要在提交信息中长篇叙述。
- **一个提交只做一件事。** 同一个文件包含两批改动时，用 `git show HEAD:<path>` 取出旧内容，只叠加其中一批改动后提交，再恢复完整版本，不要为了省去拆分步骤而暂存整个文件。
- 每个提交自身必须能够构建并通过测试，拆分出的中间状态同样如此。
- **未经授权不要 commit，更不要 push。** 修改完成后保留工作区，待作者同意后再提交。

## 判断与沟通

- **先测量，再下结论。** 结论必须来自编译器或一次真实的构建与测试，不要凭记忆断言。测量后发现自己有误并无问题，未经测量即下定论才会被纠正。
- **断言之前先验证。** 「公开 API 是这样」不等于「它确实能这样使用」。
- **探针本身也可能出错。** 结果异常时先怀疑探针。
- 用户的质疑通常是正确的，**应先重新验证，而不是辩护**。
- 发现自己有误时直接更正，将结论写回 `docs/TODO.md` 或 `.cache/claude/` 笔记的对应条目，并注明原判断的错误所在。
- 不确定时明确说明不确定，不要以推测填补。
- **只有当前机器所在平台的实现参与编译。** 作者在 Windows（MSVC）与 macOS（Apple Clang）上开发，Linux 的实现在两者上均不参与编译。其他平台的实现修改后只能依靠阅读确认。报告「验证通过」时须写明哪些平台未经验证。

## 已知问题

- **`cmake_parse_arguments` 对不认识的关键字不报错**，全部归入 `UNPARSED_ARGUMENTS`。迁移构建 API 时写错一个关键字名，对应的行为即静默丢失。**检查 `UNPARSED_ARGUMENTS` 也只能发现一部分错误：** 写错的关键字位于多值关键字之后时，被当作该关键字的值，不进入 `UNPARSED_ARGUMENTS`。实测（2026-11-04）：`qwk_add_example` 的调用中 `SOURCES` 之后的 `QT_LINK` 并入 `SOURCES`，错误由 `target_sources` 以「找不到源文件 QT_LINK」报出。值不被进一步检查的多值关键字（如 `PREDEFINED`）中，这类错误不会被发现。
- **未加引号的变量展开会被参数解析截断。** `DESCRIPTION ${VAR}` 中的 `VAR` 含空格时被拆为多个参数，`oneValueArgs` 只接收第一个词。Windows 资源中的描述曾因此只剩第一个词，长期未被发现。
- **构建树的 `out/include/QWKCore/` 同时存放转发头与 `qwkconfig.h`。** `qwk_sync_headers` 只删除自己上次写出的转发头，从不清空目录。曾经的 `qm_sync_include(... FORCE)` 先删除整个目录，`qwkconfig.h` 随之被删除并在每次配置时重新写出，导致每次重新配置后 6 个翻译单元重新编译。
- **configure 期写入磁盘的路径不能含生成器表达式**，否则生成名称中带尖括号的目录。qmsetup 的 `QMSETUP_BUILD_DIR` 默认含 `$<CONFIG>`，曾因此需要回退分支。`QWINDOWKIT_BUILD_DIR` 下的转发头、配置头与 qmake、MSBuild 文件都在 configure 期写出，因此根目录对含生成器表达式的值报错。
- **`qwk_configure_target` 是 function，其中不得调用 `find_package`。** Qt 5 的私有头目录变量只存在于创建其目标的那一次 `find_package` 的作用域中。在 function 中查找时，变量随 function 返回而消失，目标却保留，第二个目标因此既不再查找，也取不到这些变量。Qt 的模块在使用它的目录或其上级目录的目录作用域中查找：库在 `src/conf.cmake`，示例在 `examples/CMakeLists.txt`，测试在 `tests/buildsystems/CMakeLists.txt`。`src/` 中找到的导入目标与 `QT_VERSION_MAJOR` 不传到同级目录，因此三处各自查找。`qwk_configure_target` 对 `QT_LINKS` 中未找到的模块报错，Private 模块未找到时改用私有头目录。
- **非交互 shell 中不要拼接 `cmd /c "xxx.bat && ..."`。** 引号经过 Git Bash 或 PowerShell 后会被重新拆分，cmd 收到不完整的命令后打开交互式 shell 并停在那里。应将命令写入 `.bat` 文件再执行。
- **`.bat` 中不要写中文。** cmd 按 ANSI 代码页读取文件，UTF-8 的注释会被解码为乱码并被当作命令执行。
- **Qt 的头文件要求 `/Zc:__cplusplus` 与 `/permissive-`。** qmake 的 mkspec 会自动添加，手写 vcxproj 或其他 MSBuild 工程时须自行添加，否则编译会在 Qt 自身的头文件中失败。
- **CMake 的 `file(WRITE)` 在 Windows 上以文本模式写出 CRLF，`file(READ)` 读回时还原为 LF。** 因此生成文件的「读取并比较」在 Windows 上仍然成立，生成的头文件与 `qmcorecmd` 的输出同样是 CRLF。比较两个生成文件的内容时，须对两侧做同样的行尾处理。
- **`sed -i` 会将 CRLF 转换为 LF。** 修改前用 `grep -qU $'\r'` 判断行尾，CRLF 文件改用编辑工具修改。
- **`sed` 的替换表达式必须带行号地址**，否则会进行全局替换，且后续表达式会匹配前面已修改的文本。
- **CMake 中判断平台而非编译器。** clang 以 `x86_64-pc-windows-msvc` 为目标时，CMake 的 `MSVC` 与 `MINGW` 均为假，写在 `else()` 分支中的非 Windows 选项会被错误地应用。
