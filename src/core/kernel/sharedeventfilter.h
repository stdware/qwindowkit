#ifndef SHAREDEVENTFILTER_H
#define SHAREDEVENTFILTER_H

#include <QtCore/QObject>
#include <QtCore/QVector>

#include <QWKCore/qwkcoreglobal.h>

namespace QWK {

    class SharedEventDispatcherPrivate;

    class SharedEventDispatcher;

    class QWK_CORE_EXPORT SharedEventFilter {
    public:
        SharedEventFilter();
        virtual ~SharedEventFilter();

    public:
        virtual bool eventFilter(QObject *obj, QEvent *event) = 0;

    private:
        SharedEventDispatcher *m_dispatcher;

        Q_DISABLE_COPY(SharedEventFilter)

        friend class SharedEventDispatcher;
        friend class SharedEventDispatcherPrivate;
    };

    class QWK_CORE_EXPORT SharedEventDispatcher {
    public:
        SharedEventDispatcher();
        virtual ~SharedEventDispatcher();

    public:
        virtual QObject *target() const = 0;

        void installSharedEventFilter(SharedEventFilter *eventFilter);
        void removeSharedEventFilter(SharedEventFilter *eventFilter);

    protected:
        SharedEventDispatcherPrivate *d;

        Q_DISABLE_COPY(SharedEventDispatcher)
    };

}

#endif // SHAREDEVENTFILTER_H
