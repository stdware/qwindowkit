// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef QWK_QMLTYPES_P_H
#define QWK_QMLTYPES_P_H

#include <QtQml/qqmlregistration.h>
#include <QWKCore/windowagentbase.h>

// Describe the inherited API without making Core depend on Qt Qml.
struct WindowAgentBaseForeign {
    Q_GADGET
    QML_FOREIGN(QWK::WindowAgentBase)
    QML_ANONYMOUS
};

#endif // QWK_QMLTYPES_P_H
