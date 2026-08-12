// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

// The program the build system tests compile against an installed QWindowKit. It is built and
// linked, never run: what is under test is whether the include and library paths carried by the
// generated qmake and MSBuild files lead to the installed headers and import libraries.

#include <QWKCore/qwkglobal.h>
#include <QWKCore/windowagentbase.h>

#ifdef CONSUMER_USE_WIDGETS
#  include <QWKWidgets/widgetwindowagent.h>
#endif

int main(int argc, char *argv[]) {
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    // A reference to an exported symbol of every module linked, so that a library that was not
    // found is a link error rather than a program that builds and does nothing.
    const QMetaObject *core = &QWK::WindowAgentBase::staticMetaObject;

#ifdef CONSUMER_USE_WIDGETS
    const QMetaObject *widgets = &QWK::WidgetWindowAgent::staticMetaObject;
#else
    const QMetaObject *widgets = nullptr;
#endif

    return (core != nullptr && widgets != core) ? 0 : 1;
}
