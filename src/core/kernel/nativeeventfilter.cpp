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
        m_nativeDispatch.detach(&NativeEventFilter::m_nativeDispatcher);
    }

    bool NativeEventDispatcher::nativeDispatch(const QByteArray &eventType, void *message,
                                               QT_NATIVE_EVENT_RESULT_TYPE *result) {
        return m_nativeDispatch.dispatch([&eventType, message, result](NativeEventFilter *filter) {
            return filter->nativeEventFilter(eventType, message, result);
        });
    }

    void NativeEventDispatcher::installNativeEventFilter(NativeEventFilter *filter) {
        m_nativeDispatch.install(filter, this, &NativeEventFilter::m_nativeDispatcher);
    }

    void NativeEventDispatcher::removeNativeEventFilter(NativeEventFilter *filter) {
        m_nativeDispatch.remove(filter, &NativeEventFilter::m_nativeDispatcher);
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
        if (master->m_nativeDispatch.isIdleAndEmpty()) {
            delete std::exchange(AppMasterNativeEventFilter::instance, nullptr);
        }
    }

}
