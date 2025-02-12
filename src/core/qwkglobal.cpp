// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "qwkglobal_p.h"

#include <QtCore/QCoreApplication>

#include <QtCore/private/qobject_p.h>

namespace QWK {

    bool forwardObjectEventFilters(QObject *currentFilter, QObject *receiver, QEvent *event) {
        // https://github.com/qt/qtbase/blob/e26a87f1ecc40bc8c6aa5b889fce67410a57a702/src/corelib/kernel/qcoreapplication.cpp#L1244
        // Send the event through the rest event filters
        auto d = QObjectPrivate::get(receiver);
        bool findCurrent = false;
        if (receiver != QCoreApplication::instance() && d->extraData) {
            for (qsizetype i = 0; i < d->extraData->eventFilters.size(); ++i) {
                QObject *obj = d->extraData->eventFilters.at(i);
                if (!findCurrent) {
                    if (obj == currentFilter) {
                        findCurrent = true; // Will start to filter from the next one
                    }
                    continue;
                }

                if (!obj)
                    continue;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                if (QObjectPrivate::get(obj)->threadData.loadRelaxed() !=
                    d->threadData.loadRelaxed()) {
#else
                if (QObjectPrivate::get(obj)->threadData != d->threadData) {
#endif
                    qWarning("QCoreApplication: Object event filter cannot be in a different "
                             "thread.");
                    continue;
                }
                if (obj->eventFilter(receiver, event))
                    return true;
            }
        }
        return false;
    }

}
