// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef QWKWIDGETSGLOBAL_H
#define QWKWIDGETSGLOBAL_H

#include <QtCore/QtGlobal>

#ifndef QWK_WIDGETS_EXPORT
#  ifdef QWK_WIDGETS_STATIC
#    define QWK_WIDGETS_EXPORT
#  else
#    ifdef QWK_WIDGETS_LIBRARY
#      define QWK_WIDGETS_EXPORT Q_DECL_EXPORT
#    else
#      define QWK_WIDGETS_EXPORT Q_DECL_IMPORT
#    endif
#  endif
#endif

#endif // QWKWIDGETSGLOBAL_H
