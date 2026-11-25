# 与本仓库协作的约定

**语言：** 简体中文 | [English](../AGENTS.md)

本文档只列出规则。项目结构、设计决定、规则的理由与已知问题见 [Development.zh-CN.md](./Development.zh-CN.md)。

## 项目概述

QWindowKit 以应用自行绘制的标题栏替换系统标题栏，同时保留窗口的移动、缩放、系统菜单以及 Windows 的 Snap Layout 等系统行为。目录结构与各平台的窗口上下文见 Development.zh-CN.md 的「项目结构」。

- **修改一个窗口上下文的行为之前，先确认其他平台的实现是否需要同步修改，以及兜底实现 `qtwindowcontext.cpp` 是否会因此与它们不一致。**

## 文档位置

| 内容 | 位置 |
|---|---|
| 协作规则 | `AGENTS.md`（英文）、`docs/AGENTS.zh-CN.md`（中文） |
| 项目结构、设计决定、规则的理由、已知问题 | `docs/Development.md`（英文）、`docs/Development.zh-CN.md`（中文） |
| 计划、交接、调研与临时分析 | `.cache/claude/YYYY-MM-DD-HHMM-<主题>.md` |
| 待办事项、已知未验证的问题、不测试的内容及其理由 | `docs/TODO.md` |
| 面向用户的说明 | `README.md`、`docs/README.zh-CN.md`、`docs/Tutorial.md`、`docs/Tutorial.zh-CN.md` |
| 公开接口的参考文档 | 源文件中的 qdoc 注释，由文档 CI 发布到 <https://stdware.github.io/qwindowkit> |

`.cache/` 整体被 gitignore。`.cache/claude/` 中的笔记以中文书写，标题注明日期。**未来要做的事写入 `docs/TODO.md` 或 `.cache/claude/` 的笔记，不要只留在会话中。** **修改有多个语言版本的文档时，同步修改其他语言版本。**

## 构建系统

背景与设计决定见 Development.zh-CN.md 的「构建系统」。

- **CMake 函数按功能分文件，放在根目录的 `cmake/` 下**，由根目录的 `CMakeLists.txt` 显式 `include()`。
- **不要在 `src/CMakeLists.txt` 或各模块的 `CMakeLists.txt` 中定义函数或宏**，也不要为替换 qmsetup 而重新编写一个生成整套函数的 helper。
- **生成文件在内容未变时不得重写。** 不要先删除目录再全部写出，不要用 `file(CONFIGURE)` 或 `configure_file()` 批量生成。
- `cmake/` 中的文件开头以 `#[[ ... ]]` 写明调用形式与使用前提，格式见 Development.zh-CN.md 的「CMake 文件头」。
- `cmake_parse_arguments` 之前以三行 `set()` 声明关键字，前缀一律为 `FUNC`。**`FUNC_UNPARSED_ARGUMENTS` 非空时以 `FATAL_ERROR` 拒绝**，必需参数缺失时同样报错。
- 选项定义在根目录的 `CMakeLists.txt` 中。默认值必须使不传任何参数的配置保持原有行为。
- 函数和不显然的赋值上方须写明其作用，以及不能使用默认值的理由。`src/conf.cmake` 中的每一处显式赋值都附有理由，删除之前先阅读该注释。
- 作为参数的变量展开须加引号。
- configure 期写入磁盘的路径不能含生成器表达式。
- `qwk_configure_target` 中不得调用 `find_package`。
- 判断平台而非编译器。
- **修改构建系统之后，须将安装树逐文件比对**，不能只确认能够编译。

## C++

- 命名空间为 `QWK`。
- PImpl 采用 Qt 的写法，即 `Q_DECLARE_PRIVATE` 加 `d_ptr`。**私有头文件会被安装**，修改私有头文件同样影响下游。
- 导出宏与编译定义见 Development.zh-CN.md 的「导出宏」。
- 平台相关的实现放在各自的文件中，不要在一个文件中堆积 `#ifdef`。跨平台共用的小型组件放在 `src/core/shared/`。
- 编译期开关以 `#if QWINDOWKIT_CONFIG(ENABLE_XXX)` 读取，**不要绕过它直接使用 `#ifdef`**。
- 格式由 clang-format 处理：100 列，4 空格缩进，命名空间内缩进。不要手动排版。**只对自己改动过的文件运行 clang-format，不要用 `sed -i` 处理整个目录。**

## 命名

| 对象 | 规则 | 示例 |
|---|---|---|
| 构建函数 | `qwk_` 前缀，包括只供 `cmake/` 中其他文件使用的工具函数 | `qwk_add_win_rc` |
| 文件内部的构建函数 | `_qwk_` 前缀，只在定义它的文件内调用 | `cmake/ConfigureTarget.cmake` 的 `_qwk_configure_target_include_qt_private` |
| 选项与缓存变量 | `QWINDOWKIT_` 前缀 | `QWINDOWKIT_BUILD_QUICK` |
| 预处理宏 | 按模块分前缀 | `QWK_CORE`、`QWK_WIDGETS`、`QWK_QUICK` |
| 类 | 大驼峰，私有类加 `Private` 后缀 | `WidgetWindowAgentPrivate` |
| 成员 | 小驼峰 | `systemButtonAreaWidget` |
| 私有头文件 | 以 `_p.h` 结尾 | `widgetwindowagent_p.h` |
| `cmake/` 中的文件 | 大驼峰，以功能命名 | `AddWinRC.cmake` |
| `docs/` 中的文档 | 大驼峰。英文版不加语言后缀，其他语言加 `.<语言>` 后缀。`TODO.md`、`README.<语言>.md` 与 `AGENTS.<语言>.md` 除外 | `VisualStudioGuide.md`、`Tutorial.md`、`Tutorial.zh-CN.md` |

qmsetup 的 `qm_`、`_qm_` 与 helper 内部的 `_repo_` 前缀不应再出现于本仓库。

## 文体

本节适用于注释、文档、README、诊断消息与提交信息。

- **采用正式的技术写作文体。** 注释与文档是规范性文本而非叙述。每句陈述一项事实、约束或理由，不写铺垫、感想和修辞。
- **不拟人。** 代码、文件、格式、程序和测试不作为有意志的主语。用户、作者、调用方等真实行为主体可以作主语。
- **使用术语，不用描述性转述。** 没有通用术语时，首次出现给出定义，之后始终沿用同一名称。
- **标题、分组名和列表标签使用名词或名词短语。** **主语从句不可用**，能用名词短语时一律改用名词短语。**宾语从句少用**，从句比名词短语明显冗长时改用名词短语。**定语从句不设限制。**
- **条件用 if。** `where` 作关系副词时照常使用，作条件状语时写 `if`。判据是该处能否换成 `if` 而意思不变。
- **`one` 回指只用于同一个句子。** 回指前一句或更早句子的名词时，须直接重复该名词。
- **不使用口语短语**，改为准确的书面表达。
- **句子完整。** 不写片段句、逗号粘连句（两个独立分句仅以逗号连接）和反问句。不以 So、And so、Which is why、That is why、Hence 开头叙述因果，改为在同一句中用 because、therefore 表明。不对读者使用第二人称。
- **函数说明以动词开头**（Returns、Sets、Installs、Rejects）。返回值写明每种情况。布尔查询写 Returns whether …。
- 中文文本同样适用以上规则。「不要」「必须」等规范性祈使句不属于口语，照常使用。
- **记录曾经的实现错误。** 一句话说明某处曾经如何实现错误、因此现在为什么必须如此的记述不属于考古式叙述，必须保留，因为它防止同一处再错一次。要删的是与当前实现无关的历史沿革。
- **有明确重要理由而否定的做法同样保留。** 写明被否定的做法、否定的理由，以及当前做法与之的对照。理由不重要、或无法说清理由的，不写。

| 规则 | 不写 | 改写为 |
|---|---|---|
| 不拟人 | says、tells、knows、asks、wants、means、cares、decides、promises、is told，「它说」「它知道」「它不认」「它想要」 | returns、indicates、records、specifies、reports、detects、requires、rejects，「返回」「表示」「记录」「规定」「报告」「拒绝」 |
| 使用术语 | the part of the window that is not drawn by the app | non-client area |
| 使用术语 | the check for what is under the mouse | hit test |
| 使用术语 | the function that gets the messages | window procedure |
| 使用术语 | the header that points at the real one | forwarding header |
| 名词短语作标题 | Why、What it does、How it works | Motivation、Behavior、Rationale |
| 名词短语代替主语从句 | what the caller asked for | the requested geometry |
| 条件用 if | empty where there is none | empty if absent |
| 不使用口语短语 | whatever else、for good、as it stands、on its own、on the way out、at a glance、there and back、is given up on、the rest of why、and all | 准确的书面表达 |
| 中文书面语 | 「别」「搞」「就行」「得（表必须）」「啥」「拿来」「反正」「其实」「说白了」「这玩意儿」 | 书面语 |
| 中文标题 | 「为什么」「怎么做」 | 「动机」「设计理由」「实现方式」 |

## 注释

- **公开接口的文档注释采用 qdoc 风格**，以 `/*! ... */` 写在 `.cpp` 中定义的上方，块内缩进 4 格，由 Doxygen 解析并发布。类的说明以 `\class` 加 `\brief` 开头，没有定义可以依附的声明使用 `\fn`。参数写 `\a`，标识符写 `\c`，引用其他声明写 `\sa`。不要迁移到 LLVM 的 `///` 风格，新增的文档注释与现有的保持一致。
- 公开头文件中不写文档注释。调用方据以行动的内容写在 qdoc 块中，实现理由以 `//` 写在实现内部，或不写注释。
- **注释中不要使用破折号，也不要用分号连接从句。** 应断句处即断句。使用美式拼写。
- **几个词能说清的内容不写成一段。** 注释说明约束、所有权、生命周期，以及「为何只能如此实现」。不复述签名已经表明的内容，也不将一句话拆成三句。
- **不写考古式注释。** 保留「为何现在必须如此实现」，删除「以前如何、后来修正」。记录曾经的实现错误的注释，以及有明确重要理由而否定的做法，见「文体」，不在删除之列。
- 设计理由中篇幅较长的部分写入 `docs/`，不要写入公开的 qdoc 块。
- 全项目通用的约定写在本文档中，不要在每个使用处重复解释。

## Markdown

- **一段即一行**，不要按 80 或 100 列手动折行。代码块、表格、列表项照常处理。此规则仅适用于 `.md`，C++ 注释仍为 100 列。
- 文件末尾不留多余的空行。

## 测试

规则的理由见 Development.zh-CN.md 的「测试」。

- 路径、工具位置与配置一律由 `-D` 传入，测试脚本中不写死。
- 每个用例在构建树中使用独立的工作目录，运行前先清空，用例之间不共享状态。
- **平台或工具不具备时跳过并说明原因。** 不要硬编码平台判断，也不要保留必然失败的用例。
- 不测试的内容及其理由写入 `docs/TODO.md`。
- **确认测试通过之前，先确认构建的退出码为 0。**
- 新增测试后确认断言确实被执行。
- 区分断言的是当前行为还是设计意图。
- **完成一项行为的测试后，将缺陷放回，确认测试会失败。** 修改一处期望或故意传入错误路径，重新构建、运行、还原。
- **批量修改后核对「修改了几处」，而不只是「能否编译」。**

## 提交

- **提交信息不带任何 AI 署名**，不写 `Co-Authored-By`，不写 `Generated with`。
- **只写一行**，格式为 `<类型>(<范围>): <描述>`，范围可省略，例如 `fix(win32): preserve left-button state when forwarding caption clicks`、`ci: check out submodules recursively`。类型使用 `feat`、`fix`、`refactor`、`style`、`docs`、`build`、`ci`、`test`、`chore`，范围为平台或模块，如 `win32`、`macOS`、`linux`、`core`、`widgets`、`quick`、`example`。描述使用英文、小写开头的祈使句和美式拼写，不写正文。设计理由写在代码注释和 `docs/` 中，不要在提交信息中长篇叙述。
- **一个提交只做一件事。** 同一个文件包含两批改动时，用 `git show HEAD:<path>` 取出旧内容，只叠加其中一批改动后提交，再恢复完整版本，不要为了省去拆分步骤而暂存整个文件。
- 每个提交自身必须能够构建并通过测试，拆分出的中间状态同样如此。
- **未经授权不要 commit，更不要 push。** 修改完成后保留工作区，待作者同意后再提交。

## 工具使用

理由见 Development.zh-CN.md 的「已知问题」。

- 修改文件前用 `grep -qU $'\r'` 判断行尾，CRLF 文件改用编辑工具修改，不要用 `sed -i`。
- `sed` 的替换表达式必须带行号地址。
- 非交互 shell 中不要拼接 `cmd /c "xxx.bat && ..."`，应将命令写入 `.bat` 文件再执行。
- `.bat` 中不要写中文。
- 手写 vcxproj 或其他 MSBuild 工程时须添加 `/Zc:__cplusplus` 与 `/permissive-`。

## 判断与沟通

- **先测量，再下结论。** 结论必须来自编译器或一次真实的构建与测试，不要凭记忆断言。测量后发现自己有误并无问题，未经测量即下定论才会被纠正。
- **断言之前先验证。** 「公开 API 是这样」不等于「它确实能这样使用」。
- **探针本身也可能出错。** 结果异常时先怀疑探针。
- 用户的质疑通常是正确的，**应先重新验证，而不是辩护**。
- 发现自己有误时直接更正，将结论写回 `docs/TODO.md` 或 `.cache/claude/` 笔记的对应条目，并注明原判断的错误所在。
- 不确定时明确说明不确定，不要以推测填补。
- **只有当前机器所在平台的实现参与编译。** 作者在 Windows（MSVC）与 macOS（Apple Clang）上开发，Linux 的实现在两者上均不参与编译。其他平台的实现修改后只能依靠阅读确认。报告「验证通过」时须写明哪些平台未经验证。
