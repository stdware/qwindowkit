#ifndef NATIVEEVENTFILTER_H
#define NATIVEEVENTFILTER_H

#include <QWKCore/qwkglobal.h>

namespace QWK {

    class QWK_CORE_EXPORT NativeEventFilter {
    public:
        NativeEventFilter();
        virtual ~NativeEventFilter();

    public:
        virtual bool nativeEventFilter(const QByteArray &eventType, void *message,
                                       QT_NATIVE_EVENT_RESULT_TYPE *result) = 0;

    private:
        Q_DISABLE_COPY(NativeEventFilter)
    };

}

#endif // NATIVEEVENTFILTER_H
