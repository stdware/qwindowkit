// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "sharedeventfilter_p.h"

namespace QWK {

    SharedEventFilter::SharedEventFilter() : m_sharedDispatcher(nullptr) {
    }

    SharedEventFilter::~SharedEventFilter() {
        if (m_sharedDispatcher)
            m_sharedDispatcher->removeSharedEventFilter(this);
    }

    SharedEventDispatcher::SharedEventDispatcher() = default;

    SharedEventDispatcher::~SharedEventDispatcher() {
        for (const auto &observer : std::as_const(m_sharedEventFilters)) {
            observer->m_sharedDispatcher = nullptr;
        }
    }

    bool SharedEventDispatcher::sharedDispatch(QObject *obj, QEvent *event) {
        for (const auto &ef : std::as_const(m_sharedEventFilters)) {
            if (ef->sharedEventFilter(obj, event))
                return true;
        }
        return false;
    }

    void SharedEventDispatcher::installSharedEventFilter(SharedEventFilter *filter) {
        if (!filter || filter->m_sharedDispatcher)
            return;

        m_sharedEventFilters.append(filter);
        filter->m_sharedDispatcher = this;
    }

    void SharedEventDispatcher::removeSharedEventFilter(SharedEventFilter *filter) {
        if (!m_sharedEventFilters.removeOne(filter)) {
            return;
        }
        filter->m_sharedDispatcher = nullptr;
    }

}