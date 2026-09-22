// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

// Compile and link only: exercise the installed property sheet, including static imports.
#include <QWKCore/windowagentbase.h>

#ifdef CONSUMER_USE_WIDGETS
#  include <QWKWidgets/widgetwindowagent.h>
#endif
#ifdef CONSUMER_USE_QUICK
#  include <QWKQuick/quickwindowagent.h>
#endif

#ifndef CONSUMER_INHERITED_DEFINITION
#  error The property sheet discarded inherited compiler definitions.
#endif

#ifdef CONSUMER_EXPECT_STATIC
#  ifndef QWK_CORE_STATIC
#    error The static property sheet did not define QWK_CORE_STATIC.
#  endif
#  if defined(CONSUMER_USE_WIDGETS) && !defined(QWK_WIDGETS_STATIC)
#    error The static property sheet did not define QWK_WIDGETS_STATIC.
#  endif
#  if defined(CONSUMER_USE_QUICK) && !defined(QWK_QUICK_STATIC)
#    error The static property sheet did not define QWK_QUICK_STATIC.
#  endif
#else
#  if defined(QWK_CORE_STATIC) || defined(QWK_WIDGETS_STATIC) || defined(QWK_QUICK_STATIC)
#    error The shared property sheet must not define static import macros.
#  endif
#endif

int main() {
    // Reading metaobjects and calling exported methods keeps the linker references alive
    // even in optimized builds; merely comparing their addresses can be optimized away.
    int result = QWK::WindowAgentBase::staticMetaObject.className()[0];
#ifdef CONSUMER_USE_WIDGETS
    QWK::WidgetWindowAgent widgets;
    result += QWK::WidgetWindowAgent::staticMetaObject.className()[0];
    result += widgets.titleBar() != nullptr;
#endif
#ifdef CONSUMER_USE_QUICK
    QWK::QuickWindowAgent quick;
    result += QWK::QuickWindowAgent::staticMetaObject.className()[0];
    result += quick.titleBar() != nullptr;
#endif
    return result;
}
