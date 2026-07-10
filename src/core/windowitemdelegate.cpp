// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "windowitemdelegate_p.h"

#include <QtWidgets/QWidget>

namespace QWK {

    WindowItemDelegate::WindowItemDelegate() = default;

    WindowItemDelegate::~WindowItemDelegate() = default;

    bool WindowItemDelegate::isSameOrAncestorOf(const QObject *ancestor, const QObject *child) const
    {
        const auto *ancestorWidget = qobject_cast<const QWidget *>(ancestor);
        const auto *childWidget = qobject_cast<const QWidget *>(child);

        if (!ancestorWidget || !childWidget) {
            return false;
        }

        return ancestorWidget == childWidget || ancestorWidget->isAncestorOf(childWidget);
    }

    void WindowItemDelegate::resetQtGrabbedControl(QObject *host) const {
        Q_UNUSED(host);
    }

    WinIdChangeEventFilter *
        WindowItemDelegate::createWinIdEventFilter(QObject *host,
                                                   AbstractWindowContext *context) const {
        return new WindowWinIdChangeEventFilter(static_cast<QWindow *>(host), context);
    }

}
