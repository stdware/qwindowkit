# Conventions for Working in This Repository

**Language:** English | [简体中文](./docs/AGENTS.zh-CN.md)

This document lists rules only. The project structure, design decisions, rationale for the rules, and known issues are in [docs/Development.md](./docs/Development.md).

## Project Overview

QWindowKit replaces the system title bar with a title bar drawn by the application, while preserving system behaviors such as window moving, resizing, the system menu, and Snap Layout on Windows. The directory structure and the window context of each platform are described in "Project Structure" of Development.md.

- **Before changing the behavior of one window context, check whether the implementations for the other platforms require the same change, and whether the fallback implementation `qtwindowcontext.cpp` becomes inconsistent with them.**

## Document Locations

| Content | Location |
|---|---|
| Collaboration rules | `AGENTS.md` (English), `docs/AGENTS.zh-CN.md` (Chinese) |
| Project structure, design decisions, rationale for the rules, known issues | `docs/Development.md` (English), `docs/Development.zh-CN.md` (Chinese) |
| Plans, handovers, investigations, and temporary analyses | `.cache/claude/YYYY-MM-DD-HHMM-<topic>.md` |
| To-do items, known unverified issues, untested content and the reasons | `docs/TODO.md` |
| User documentation | `README.md`, `docs/README.zh-CN.md`, `docs/Tutorial.md`, `docs/Tutorial.zh-CN.md` |
| Reference documentation of the public interface | qdoc comments in the sources, published by the documentation CI to <https://stdware.github.io/qwindowkit> |

`.cache/` is gitignored as a whole. Notes in `.cache/claude/` are written in Chinese, and their titles carry the date. **Record future work in `docs/TODO.md` or in a note in `.cache/claude/`, not only in the conversation.** **If a document exists in several languages, update all language versions together.**

## Build System

The background and the design decisions are described in "Build System" of Development.md.

- **CMake functions are split into files by feature and placed in `cmake/` at the repository root**, and the root `CMakeLists.txt` includes each file explicitly with `include()`.
- **Do not define functions or macros in `src/CMakeLists.txt` or in the `CMakeLists.txt` of a module**, and do not write a helper that generates a whole set of functions as a replacement for qmsetup.
- **A generated file must not be rewritten if its content is unchanged.** Do not delete a directory and rewrite it as a whole, and do not use `file(CONFIGURE)` or `configure_file()` for bulk generation.
- Each file in `cmake/` begins with a `#[[ ... ]]` block that specifies the calling form and the preconditions, in the format described in "CMake File Header" of Development.md.
- Before `cmake_parse_arguments`, three `set()` lines declare the keywords, and the prefix is always `FUNC`. **Reject a non-empty `FUNC_UNPARSED_ARGUMENTS` with `FATAL_ERROR`**, and report a missing required argument in the same way.
- Options are defined in the root `CMakeLists.txt`. Default values must preserve the existing behavior of a configuration without any arguments.
- A function and a non-obvious assignment carry a comment above them that states their purpose and the reason why the default value is not used. Every explicit assignment in `src/conf.cmake` has a reason in its comment. Read the comment before removing the assignment.
- Quote variable expansions used as arguments.
- Paths written to disk at configure time must not contain generator expressions.
- `qwk_configure_target` must not call `find_package`.
- Test the platform instead of the compiler.
- **After a change to the build system, compare the install tree file by file.** A successful build alone is not sufficient.

## C++

- The namespace is `QWK`.
- PImpl follows the Qt pattern of `Q_DECLARE_PRIVATE` with `d_ptr`. **Private headers are installed**, therefore a change to a private header affects consumers as well.
- The export macros and compile definitions are listed in "Export Macros" of Development.md.
- Platform-specific implementations reside in their own files. Do not accumulate `#ifdef` blocks in one file. Small components shared across platforms reside in `src/core/shared/`.
- Read compile-time switches with `#if QWINDOWKIT_CONFIG(ENABLE_XXX)`. **Do not bypass the macro with `#ifdef`.**
- clang-format handles the formatting: 100 columns, 4-space indentation, and indentation inside namespaces. Do not format by hand. **Run clang-format only on changed files, and do not apply `sed -i` to a whole directory.**

## Naming

| Item | Rule | Example |
|---|---|---|
| Build function | `qwk_` prefix, including utility functions used only by other files in `cmake/` | `qwk_add_win_rc` |
| File-internal build function | `_qwk_` prefix, called only within the defining file | `_qwk_configure_target_include_qt_private` in `cmake/ConfigureTarget.cmake` |
| Option and cache variable | `QWINDOWKIT_` prefix | `QWINDOWKIT_BUILD_QUICK` |
| Preprocessor macro | Prefix by module | `QWK_CORE`, `QWK_WIDGETS`, `QWK_QUICK` |
| Class | Upper camel case, `Private` suffix for private classes | `WidgetWindowAgentPrivate` |
| Member | Lower camel case | `systemButtonAreaWidget` |
| Private header | `_p.h` suffix | `widgetwindowagent_p.h` |
| File in `cmake/` | Upper camel case, named after the feature | `AddWinRC.cmake` |
| Document in `docs/` | Upper camel case. The English version has no language suffix, and other languages add a `.<language>` suffix. `TODO.md`, `README.<language>.md`, and `AGENTS.<language>.md` are exceptions | `VisualStudioGuide.md`, `Tutorial.md`, `Tutorial.zh-CN.md` |

The qmsetup prefixes `qm_` and `_qm_` and the helper-internal prefix `_repo_` must not appear in this repository again.

## Writing Style

This section applies to comments, documents, READMEs, diagnostic messages, and commit messages.

- **Use formal technical writing.** Comments and documents are normative texts, not narratives. Each sentence states one fact, constraint, or reason, without preamble, impressions, or rhetoric.
- **No personification.** Code, files, formats, programs, and tests are not subjects with intent. Real actors such as users, authors, and callers can be subjects.
- **Use terms instead of descriptive paraphrases.** If no common term exists, define the term at its first occurrence and use the same name consistently afterwards.
- **Headings, group names, and list labels are nouns or noun phrases.** **Subject clauses are not allowed**, and a noun phrase replaces a subject clause wherever possible. **Object clauses are used sparingly**, and a noun phrase replaces an object clause if the clause is noticeably longer. **Relative clauses are not restricted.**
- **Use if for conditions.** `where` as a relative adverb is used normally. As a conditional adverbial, write `if` instead. The criterion is whether `if` can replace the word without changing the meaning.
- **`one` refers back only within the same sentence.** To refer to a noun in a previous sentence, repeat the noun.
- **No colloquial phrases.** Use precise written expressions instead.
- **Complete sentences.** No sentence fragments, comma splices (two independent clauses joined only by a comma), or rhetorical questions. Do not begin a causal statement with So, And so, Which is why, That is why, or Hence. State the cause in the same sentence with because or therefore. Do not address the reader in the second person.
- **Function descriptions begin with a verb** (Returns, Sets, Installs, Rejects). State the return value for every case. Boolean queries use Returns whether ….
- The rules above apply to Chinese text as well. Normative imperatives such as 「不要」 and 「必须」 are not colloquial and are used normally.
- **Record former implementation errors.** A sentence that states how something was once implemented incorrectly and therefore why the current implementation must be as it is does not count as archaeological narrative. Such a sentence must be kept, because it prevents the same error from recurring. Historical background unrelated to the current implementation is removed.
- **Rejected approaches with a clear and important reason are kept as well.** State the rejected approach, the reason for the rejection, and the contrast with the current approach. If the reason is unimportant or cannot be stated clearly, omit the approach.

| Rule | Avoid | Write instead |
|---|---|---|
| No personification | says, tells, knows, asks, wants, means, cares, decides, promises, is told, 「它说」「它知道」「它不认」「它想要」 | returns, indicates, records, specifies, reports, detects, requires, rejects, 「返回」「表示」「记录」「规定」「报告」「拒绝」 |
| Terms | the part of the window that is not drawn by the app | non-client area |
| Terms | the check for what is under the mouse | hit test |
| Terms | the function that gets the messages | window procedure |
| Terms | the header that points at the real one | forwarding header |
| Noun phrases as headings | Why, What it does, How it works | Motivation, Behavior, Rationale |
| Noun phrases instead of subject clauses | what the caller asked for | the requested geometry |
| if for conditions | empty where there is none | empty if absent |
| No colloquial phrases | whatever else, for good, as it stands, on its own, on the way out, at a glance, there and back, is given up on, the rest of why, and all | precise written expressions |
| Written Chinese | 「别」「搞」「就行」「得（表必须）」「啥」「拿来」「反正」「其实」「说白了」「这玩意儿」 | written-register Chinese |
| Chinese headings | 「为什么」「怎么做」 | 「动机」「设计理由」「实现方式」 |

## Comments

- **Documentation comments of the public interface use qdoc style**: `/*! ... */` above the definition in the `.cpp` file, with 4-space indentation inside the block, parsed and published by Doxygen. A class description begins with `\class` and `\brief`, and a declaration without a definition to attach to uses `\fn`. Parameters use `\a`, identifiers use `\c`, and references to other declarations use `\sa`. Do not migrate to the LLVM `///` style. New documentation comments follow the existing ones.
- Public headers contain no documentation comments. Content that callers act upon belongs in the qdoc block. Implementation rationale is written with `//` inside the implementation, or omitted.
- **Comments contain no dashes and do not join clauses with semicolons.** Break the sentence instead. Use American spelling.
- **Content that a few words can express is not written as a paragraph.** Comments state constraints, ownership, lifetime, and the reason why the implementation can only be as it is. Comments do not repeat what the signature already states, and do not split one sentence into three.
- **No archaeological comments.** Keep the reason why the current implementation must be as it is, and remove accounts of former implementations and later corrections. Comments that record former implementation errors, and rejected approaches with a clear and important reason, are not removed, as described in "Writing Style".
- Long parts of design rationale belong in `docs/`, not in public qdoc blocks.
- Conventions common to the whole project are written in this document and are not explained again at each place of use.

## Markdown

- **One paragraph per line.** Do not wrap lines manually at 80 or 100 columns. Code blocks, tables, and list items are handled normally. The rule applies only to `.md` files. C++ comments remain at 100 columns.
- Files end without extra blank lines.

## Tests

The rationale for the rules is described in "Tests" of Development.md.

- Paths, tool locations, and configurations are always passed with `-D` and are never hardcoded in test scripts.
- Each test case uses its own working directory in the build tree, cleared before the run. Test cases share no state.
- **If a platform or tool is unavailable, skip the test and state the reason.** Do not hardcode platform checks, and do not keep test cases that always fail.
- Record untested content and the reasons in `docs/TODO.md`.
- **Before confirming that tests pass, confirm that the build exit code is 0.**
- After adding a test, confirm that its assertions are executed.
- Distinguish whether an assertion checks the current behavior or the design intent.
- **After testing a behavior, restore the defect and confirm that the test fails.** Change one expectation or pass a wrong path deliberately, then rebuild, run, and revert.
- **After a bulk change, verify the number of changed occurrences, not only that the build succeeds.**

## Commits

- **Commit messages carry no AI attribution**: no `Co-Authored-By` and no `Generated with`.
- **One line only**, in the format `<type>(<scope>): <description>` with an optional scope, for example `fix(win32): preserve left-button state when forwarding caption clicks` and `ci: check out submodules recursively`. Types are `feat`, `fix`, `refactor`, `style`, `docs`, `build`, `ci`, `test`, and `chore`. The scope is a platform or module, such as `win32`, `macOS`, `linux`, `core`, `widgets`, `quick`, or `example`. The description is an English imperative starting with a lowercase letter, in American spelling, without a body. Design rationale belongs in code comments and `docs/`, not in long commit messages.
- **One commit does one thing.** If one file contains two sets of changes, take the old content with `git show HEAD:<path>`, apply only one set of changes, commit, and then restore the full version. Do not stage the whole file to avoid the split.
- Every commit must build and pass the tests on its own, including intermediate states produced by a split.
- **Do not commit without authorization, and do not push.** Leave the working tree after a change and commit only after the author agrees.

## Tool Usage

The rationale is described in "Known Issues" of Development.md.

- Before changing a file, check the line endings with `grep -qU $'\r'`. Change CRLF files with an editing tool instead of `sed -i`.
- Every `sed` substitution must carry a line address.
- Do not compose `cmd /c "xxx.bat && ..."` in a non-interactive shell. Write the commands into a `.bat` file and run the file.
- Do not write Chinese text in `.bat` files.
- A handwritten vcxproj or other MSBuild project must add `/Zc:__cplusplus` and `/permissive-`.

## Judgment and Communication

- **Measure before concluding.** Conclusions must come from the compiler or from a real build and test run, not from memory. A mistake found through measurement is acceptable. Only a conclusion drawn without measurement is subject to correction.
- **Verify before asserting.** A documented public API does not guarantee that the API works in the intended way.
- **A probe can be wrong as well.** If a result looks abnormal, suspect the probe first.
- A challenge from the user is usually correct. **Verify again instead of defending.**
- If a mistake is found, correct it directly, write the conclusion back to the corresponding entry in `docs/TODO.md` or in a `.cache/claude/` note, and state the error in the original judgment.
- If uncertain, state the uncertainty explicitly instead of filling the gap with speculation.
- **Only the implementation for the platform of the current machine is compiled.** The author develops on Windows (MSVC) and macOS (Apple Clang). The Linux implementations are compiled on neither. Changes to implementations for other platforms can only be confirmed by reading. A report of successful verification must state which platforms remain unverified.
