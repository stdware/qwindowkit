// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "sharedeventfilter_p.h"

namespace QWK {

    SharedEventFilter::SharedEventFilter() = default;

    SharedEventFilter::~SharedEventFilter() {
        if (m_sharedDispatcher)
            m_sharedDispatcher->removeSharedEventFilter(this);
    }

    SharedEventDispatcher::SharedEventDispatcher() = default;

    SharedEventDispatcher::~SharedEventDispatcher() {
        for (const auto &observer : std::as_const(m_sharedEventFilters)) {
            if (!observer)
                continue;
            observer->m_sharedDispatcher = nullptr;
        }
    }

    bool SharedEventDispatcher::sharedDispatch(QObject *obj, QEvent *event) {
        // A callback is free to install or remove filters, including itself, and it may even
        // re-enter this function because handling an event can deliver more events. Iterate by
        // index and re-read the size on every step, and rely on removals leaving a null
        // tombstone behind so that the indexes of the outer dispatches stay valid. This mirrors
        // how QObject's event filter list is walked.
        ++m_sharedDispatchDepth;
        bool filtered = false;
        for (qsizetype i = 0; i < m_sharedEventFilters.size(); ++i) {
            SharedEventFilter *ef = m_sharedEventFilters.at(i);
            if (!ef)
                continue;
            if (ef->sharedEventFilter(obj, event)) {
                filtered = true;
                break;
            }
        }
        if (--m_sharedDispatchDepth == 0) {
            m_sharedEventFilters.removeAll(nullptr);
        }
        return filtered;
    }

    void SharedEventDispatcher::installSharedEventFilter(SharedEventFilter *filter) {
        if (!filter || filter->m_sharedDispatcher)
            return;

        m_sharedEventFilters.append(filter);
        filter->m_sharedDispatcher = this;
    }

    void SharedEventDispatcher::removeSharedEventFilter(SharedEventFilter *filter) {
        const qsizetype index = m_sharedEventFilters.indexOf(filter);
        if (index < 0) {
            return;
        }
        if (m_sharedDispatchDepth > 0) {
            m_sharedEventFilters[index] = nullptr;
        } else {
            m_sharedEventFilters.removeAt(index);
        }
        filter->m_sharedDispatcher = nullptr;
    }

}
