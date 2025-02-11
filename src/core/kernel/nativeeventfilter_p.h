// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef NATIVEEVENTFILTER_P_H
#define NATIVEEVENTFILTER_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QWKCore/qwkglobal.h>

namespace QWK {

    class NativeEventFilter;

    class QWK_CORE_EXPORT NativeEventDispatcher {
    public:
        NativeEventDispatcher();
        virtual ~NativeEventDispatcher();

    public:
        virtual bool nativeDispatch(const QByteArray &eventType, void *message,
                                    QT_NATIVE_EVENT_RESULT_TYPE *result);

    public:
        void installNativeEventFilter(NativeEventFilter *filter);
        void removeNativeEventFilter(NativeEventFilter *filter);

    protected:
        QList<NativeEventFilter *> m_nativeEventFilters;

        friend class NativeEventFilter;

        Q_DISABLE_COPY(NativeEventDispatcher)
    };

    class QWK_CORE_EXPORT NativeEventFilter {
    public:
        NativeEventFilter();
        virtual ~NativeEventFilter();

    public:
        virtual bool nativeEventFilter(const QByteArray &eventType, void *message,
                                       QT_NATIVE_EVENT_RESULT_TYPE *result) = 0;

    protected:
        NativeEventDispatcher *m_nativeDispatcher;

        friend class NativeEventDispatcher;

        Q_DISABLE_COPY(NativeEventFilter)
    };

    // Automatically install to QCoreApplication at construction
    class QWK_CORE_EXPORT AppNativeEventFilter : public NativeEventFilter {
    public:
        AppNativeEventFilter();
        ~AppNativeEventFilter() override;
    };

}

#endif // NATIVEEVENTFILTER_P_H
