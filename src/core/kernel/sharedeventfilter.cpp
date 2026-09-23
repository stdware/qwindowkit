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
        m_sharedDispatch.detach(&SharedEventFilter::m_sharedDispatcher);
    }

    bool SharedEventDispatcher::sharedDispatch(QObject *obj, QEvent *event) {
        return m_sharedDispatch.dispatch([obj, event](SharedEventFilter *filter) {
            return filter->sharedEventFilter(obj, event);
        });
    }

    void SharedEventDispatcher::installSharedEventFilter(SharedEventFilter *filter) {
        m_sharedDispatch.install(filter, this, &SharedEventFilter::m_sharedDispatcher);
    }

    void SharedEventDispatcher::removeSharedEventFilter(SharedEventFilter *filter) {
        m_sharedDispatch.remove(filter, &SharedEventFilter::m_sharedDispatcher);
    }

}
