#ifndef NATIVEEVENTFILTER_P_H
#define NATIVEEVENTFILTER_P_H

#include <QWKCore/qwkglobal.h>

namespace QWK {

    class NativeEventFilter;

    class QWK_CORE_EXPORT NativeEventDispatcher {
    public:
        NativeEventDispatcher();
        virtual ~NativeEventDispatcher();

    public:
        virtual bool dispatch(const QByteArray &eventType, void *message,
                              QT_NATIVE_EVENT_RESULT_TYPE *result);

    public:
        void installNativeEventFilter(NativeEventFilter *filter);
        void removeNativeEventFilter(NativeEventFilter *filter);

    protected:
        QVector<NativeEventFilter *> m_nativeEventFilters;

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
        NativeEventDispatcher *m_dispatcher;

        Q_DISABLE_COPY(NativeEventFilter)

        friend class NativeEventDispatcher;
    };

    // Automatically install to QCoreApplication at construction
    class QWK_CORE_EXPORT AppNativeEventFilter : public NativeEventFilter {
    public:
        AppNativeEventFilter();
        ~AppNativeEventFilter() override;
    };

}

#endif // NATIVEEVENTFILTER_P_H
