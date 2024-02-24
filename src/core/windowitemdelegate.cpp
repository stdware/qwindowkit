// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "windowitemdelegate_p.h"

namespace QWK {

    WindowItemDelegate::WindowItemDelegate() = default;

    WindowItemDelegate::~WindowItemDelegate() = default;

    void WindowItemDelegate::resetQtGrabbedControl(QObject *host) const {
        Q_UNUSED(host);
    }

    WinIdChangeEventFilter *
        WindowItemDelegate::createWinIdEventFilter(QObject *host,
                                                   AbstractWindowContext *context) const {
        return new WindowWinIdChangeEventFilter(static_cast<QWindow *>(host), context);
    }

}
