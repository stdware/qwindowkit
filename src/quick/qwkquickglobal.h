// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef QWKQUICKGLOBAL_H
#define QWKQUICKGLOBAL_H

#include <QtCore/QtGlobal>

#ifndef QWK_QUICK_EXPORT
#  ifdef QWK_QUICK_STATIC
#    define QWK_QUICK_EXPORT
#  else
#    ifdef QWK_QUICK_LIBRARY
#      define QWK_QUICK_EXPORT Q_DECL_EXPORT
#    else
#      define QWK_QUICK_EXPORT Q_DECL_IMPORT
#    endif
#  endif
#endif

QT_BEGIN_NAMESPACE
class QQmlEngine;
QT_END_NAMESPACE

namespace QWK {

    QWK_QUICK_EXPORT void registerTypes(QQmlEngine *engine);

}

#endif // QWKQUICKGLOBAL_H
