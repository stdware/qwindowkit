// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "nativeeventfilter_p.h"

#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>

namespace QWK {

    NativeEventFilter::NativeEventFilter() : m_nativeDispatcher(nullptr) {
    }

    NativeEventFilter::~NativeEventFilter() {
        if (m_nativeDispatcher)
            m_nativeDispatcher->removeNativeEventFilter(this);
    }

    NativeEventDispatcher::NativeEventDispatcher() = default;

    NativeEventDispatcher::~NativeEventDispatcher() {
        for (const auto &observer : std::as_const(m_nativeEventFilters)) {
            observer->m_nativeDispatcher = nullptr;
        }
    }

    bool NativeEventDispatcher::nativeDispatch(const QByteArray &eventType, void *message,
                                               QT_NATIVE_EVENT_RESULT_TYPE *result) {
        for (const auto &ef : std::as_const(m_nativeEventFilters)) {
            if (ef->nativeEventFilter(eventType, message, result))
                return true;
        }
        return false;
    }

    void NativeEventDispatcher::installNativeEventFilter(NativeEventFilter *filter) {
        if (!filter || filter->m_nativeDispatcher)
            return;

        m_nativeEventFilters.append(filter);
        filter->m_nativeDispatcher = this;
    }

    void NativeEventDispatcher::removeNativeEventFilter(NativeEventFilter *filter) {
        if (!m_nativeEventFilters.removeOne(filter)) {
            return;
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
        AppMasterNativeEventFilter::instance->removeNativeEventFilter(this);
        if (AppMasterNativeEventFilter::instance->m_nativeEventFilters.isEmpty()) {
            delete AppMasterNativeEventFilter::instance;
            AppMasterNativeEventFilter::instance = nullptr;
        }
    }

}