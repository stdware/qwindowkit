// Copyright (C) 2023-2024 Stdware Collections
// SPDX-License-Identifier: Apache-2.0

#include "windowitemdelegate_p.h"

namespace QWK {

    WindowItemDelegate::WindowItemDelegate() = default;

    WindowItemDelegate::~WindowItemDelegate() = default;

    void WindowItemDelegate::resetQtGrabbedControl(QObject *host) const {
        Q_UNUSED(host);
    }

}