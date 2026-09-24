// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

// The program the build system tests compile against an installed QWindowKit. It is built and
// linked, never run: what is under test is whether the include and library paths carried by the
// generated qmake and MSBuild files lead to the installed headers and import libraries.

// Qt 6.9's qyieldcpu.h uses __yield() without including its declaration. Recent Apple Clang
// diagnoses this when qmake includes Qt headers as non-system headers. Include the ARM intrinsics
// before any Qt headers, and only for ARM64 so Intel and universal macOS builds remain valid.
#if defined(__APPLE__) && defined(__aarch64__) && defined(__clang__)
#  include <arm_acle.h>
#endif

#include <QWKCore/qwkglobal.h>
#include <QWKCore/windowagentbase.h>
#include <QWKCore/private/nativeeventfilter_p.h>
#include <QWKCore/private/sharedeventfilter_p.h>

#ifdef CONSUMER_USE_WIDGETS
#  include <QWKWidgets/widgetwindowagent.h>
#endif
#ifdef CONSUMER_USE_QUICK
#  include <QWKQuick/quickwindowagent.h>
#endif

int main(int argc, char *argv[]) {
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    // Private dispatcher headers depend on the shared state header being installed too.
    QWK::SharedEventDispatcher sharedDispatcher;
    QWK::NativeEventDispatcher nativeDispatcher;

    // A reference to an exported symbol of every module linked, so that a library that was not
    // found is a link error rather than a program that builds and does nothing.
    const QMetaObject *core = &QWK::WindowAgentBase::staticMetaObject;

#ifdef CONSUMER_USE_WIDGETS
    const QMetaObject *widgets = &QWK::WidgetWindowAgent::staticMetaObject;
#else
    const QMetaObject *widgets = nullptr;
#endif

#ifdef CONSUMER_USE_QUICK
    const QMetaObject *quick = &QWK::QuickWindowAgent::staticMetaObject;
#else
    const QMetaObject *quick = nullptr;
#endif

    return (core != nullptr && widgets != core && quick != core) ? 0 : 1;
}
