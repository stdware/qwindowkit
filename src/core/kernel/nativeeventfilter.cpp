// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "nativeeventfilter_p.h"

#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>

namespace QWK {

    NativeEventFilter::NativeEventFilter() = default;

    NativeEventFilter::~NativeEventFilter() {
        if (m_nativeDispatcher)
            m_nativeDispatcher->removeNativeEventFilter(this);
    }

    NativeEventDispatcher::NativeEventDispatcher() = default;

    NativeEventDispatcher::~NativeEventDispatcher() {
        for (const auto &observer : std::as_const(m_nativeEventFilters)) {
            if (!observer)
                continue;
            observer->m_nativeDispatcher = nullptr;
        }
    }

    bool NativeEventDispatcher::nativeDispatch(const QByteArray &eventType, void *message,
                                               QT_NATIVE_EVENT_RESULT_TYPE *result) {
        // A callback is free to install or remove filters, including itself, and it may even
        // re-enter this function because handling a native event can pump more native events.
        // Iterate by index and re-read the size on every step, and rely on removals leaving a
        // null tombstone behind so that the indexes of the outer dispatches stay valid. This
        // mirrors how QObject's event filter list is walked.
        ++m_nativeDispatchDepth;
        bool filtered = false;
        for (qsizetype i = 0; i < m_nativeEventFilters.size(); ++i) {
            NativeEventFilter *ef = m_nativeEventFilters.at(i);
            if (!ef)
                continue;
            if (ef->nativeEventFilter(eventType, message, result)) {
                filtered = true;
                break;
            }
        }
        if (--m_nativeDispatchDepth == 0) {
            m_nativeEventFilters.removeAll(nullptr);
        }
        return filtered;
    }

    void NativeEventDispatcher::installNativeEventFilter(NativeEventFilter *filter) {
        if (!filter || filter->m_nativeDispatcher)
            return;

        m_nativeEventFilters.append(filter);
        filter->m_nativeDispatcher = this;
    }

    void NativeEventDispatcher::removeNativeEventFilter(NativeEventFilter *filter) {
        const qsizetype index = m_nativeEventFilters.indexOf(filter);
        if (index < 0) {
            return;
        }
        if (m_nativeDispatchDepth > 0) {
            m_nativeEventFilters[index] = nullptr;
        } else {
            m_nativeEventFilters.removeAt(index);
        }
        filter->m_nativeDispatcher = nullptr;
    }


    // Avoid adding multiple global native event filters to QGuiApplication
    // in this library.
    class AppMasterNativeEventFilter : public QAbstractNativeEventFilter,
                                       public NativeEventDispatcher {
    public:
        AppMasterNativeEventFilter() {
            qApp->installNativeEventFilter(this);
        }

        // The base class removes automatically
        ~AppMasterNativeEventFilter() override = default;

        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            return nativeDispatch(eventType, message, result);
        }

        static inline AppMasterNativeEventFilter *instance = nullptr;

        friend class AppNativeEventFilter;
    };

    AppNativeEventFilter::AppNativeEventFilter() {
        if (!AppMasterNativeEventFilter::instance) {
            AppMasterNativeEventFilter::instance = new AppMasterNativeEventFilter();
        }
        AppMasterNativeEventFilter::instance->installNativeEventFilter(this);
    }

    AppNativeEventFilter::~AppNativeEventFilter() {
        auto master = AppMasterNativeEventFilter::instance;
        master->removeNativeEventFilter(this);
        // Never destroy the master from inside its own dispatch, its stack frame is still
        // alive. It stays registered with an empty filter list instead, and the next
        // AppNativeEventFilter simply picks it up again.
        if (master->m_nativeDispatchDepth == 0 && master->m_nativeEventFilters.isEmpty()) {
            delete std::exchange(AppMasterNativeEventFilter::instance, nullptr);
        }
    }

}
