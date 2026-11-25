# Conventions for Working in This Repository

**Language:** English | [简体中文](./docs/AGENTS.zh-CN.md)

This document lists the rules for working in this repository. Detailed conventions, the project structure, design rationale, and known issues are in [docs/Development.md](./docs/Development.md).

## Project Overview

QWindowKit replaces the system title bar with a title bar drawn by the application, while preserving system behaviors such as window moving, resizing, the system menu, and Snap Layout on Windows. The directory structure and the window context of each platform are described in "Project Structure" of Development.md.

- **Before changing the behavior of one window context, check whether the implementations for the other platforms require the same change, and whether the fallback implementation `qtwindowcontext.cpp` becomes inconsistent with them.**

## Document Locations

| Content | Location |
|---|---|
| Collaboration rules | `AGENTS.md`, `docs/AGENTS.<lang>.md` |
| Detailed conventions, project structure, design rationale, known issues | `docs/Development[.<lang>].md` |
| Plans, handovers, investigations, and temporary analyses | `.cache/claude/YYYY-MM-DD-HHMM-<topic>.md` |
| To-do items, known unverified issues, untested content and the reasons | `docs/TODO.md` |
| User documentation | `README.md`, `docs/README.<lang>.md`, `docs/Tutorial[.<lang>].md` |
| Reference documentation of the public interface | qdoc comments in the sources, published by the documentation CI to <https://stdware.github.io/qwindowkit> |

`<lang>` is a language tag such as `zh-CN`, and `[.<lang>]` denotes an optional suffix. A document without a language tag is the English version.

`.cache/` is gitignored as a whole. Notes in `.cache/claude/` are written in the language of the conversation with the user, and their titles carry the date. **Record future work in `docs/TODO.md` or in a note in `.cache/claude/`, not only in the conversation.** **If a document exists in several languages, update all language versions together.**

## Code Conventions

Before changing code, read the corresponding section of Development.md.

| Change | Section |
|---|---|
| CMake functions, options, generated files, install layout | "Build System" |
| C++ sources, export macros, compile-time switches | "C++" |
| Names of functions, variables, macros, classes, and files | "Naming" |
| Comments and documentation comments | "Comments" |
| Tests | "Tests" |
| `sed`, `cmd`, `.bat` files, MSBuild projects | "Known Issues" |

The following rules apply to every change:

- **Run clang-format only on changed files.** Do not format by hand, and do not apply `sed -i` to a whole directory.
- **Check the line endings with `grep -qU $'\r'` before changing a file**, and change CRLF files with an editing tool instead of `sed -i`.
- **Private headers are installed.** A change to a private header affects consumers.
- **After a change to the build system, compare the install tree file by file.** A successful build alone is not sufficient.
- **Before confirming that tests pass, confirm that the build exit code is 0.** After testing a behavior, restore the defect and confirm that the test fails.

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

## Markdown

- **One paragraph per line.** Do not wrap lines manually at 80 or 100 columns. Code blocks, tables, and list items are handled normally. The rule applies only to `.md` files. C++ comments remain at 100 columns.
- Files end without extra blank lines.

## Commits

- **Commit messages carry no AI attribution**: no `Co-authored-by` trailer in any letter case that names an AI or a tool, such as `Co-authored-by: Cursor <cursoragent@cursor.com>` or `Co-Authored-By: Claude <noreply@anthropic.com>` with or without a model name after `Claude`, and no `Generated with` line such as `Generated with [Claude Code](...)`. Tools can append such a trailer automatically, therefore check the full message with `git log -1 --format=%B` after every commit and remove the trailer before pushing.
- **One line only**, in the format `<type>(<scope>): <description>` with an optional scope, for example `fix(win32): preserve left-button state when forwarding caption clicks` and `ci: check out submodules recursively`. Types are `feat`, `fix`, `refactor`, `style`, `docs`, `build`, `ci`, `test`, and `chore`. The scope is a platform or module, such as `win32`, `macOS`, `linux`, `core`, `widgets`, `quick`, or `example`. The description is an English imperative starting with a lowercase letter, in American spelling, without a body. Design rationale belongs in code comments and `docs/`, not in long commit messages.
- **One commit does one thing.** If one file contains two sets of changes, take the old content with `git show HEAD:<path>`, apply only one set of changes, commit, and then restore the full version. Do not stage the whole file to avoid the split.
- Every commit must build and pass the tests on its own, including intermediate states produced by a split.
- **Do not commit without authorization, and do not push.** Leave the working tree after a change and commit only after the author agrees.

## Judgment and Communication

- **Measure before concluding.** Conclusions must come from the compiler or from a real build and test run, not from memory. A mistake found through measurement is acceptable. Only a conclusion drawn without measurement is subject to correction.
- **Verify before asserting.** A documented public API does not guarantee that the API works in the intended way.
- **A probe can be wrong as well.** If a result looks abnormal, suspect the probe first.
- A challenge from the user is usually correct. **Verify again instead of defending.**
- If a mistake is found, correct it directly, write the conclusion back to the corresponding entry in `docs/TODO.md` or in a `.cache/claude/` note, and state the error in the original judgment.
- If uncertain, state the uncertainty explicitly instead of filling the gap with speculation.
- **Only the implementation for the platform of the current machine is compiled.** The author develops on Windows (MSVC) and macOS (Apple Clang). The Linux implementations are compiled on neither. Changes to implementations for other platforms can only be confirmed by reading. A report of successful verification must state which platforms remain unverified.
