#ifndef EVENTFILTER_P_H
#define EVENTFILTER_P_H

#include <QtGui/QtEvents>

#include <QWKCore/qwkglobal.h>

namespace QWK {

    class EventDispatcher;

    class QWK_CORE_EXPORT EventObserver {
    public:
        EventObserver();
        virtual ~EventObserver();

    protected:
        virtual bool observe(QEvent *event) = 0;

    protected:
        EventDispatcher *m_dispatcher;

        Q_DISABLE_COPY(EventObserver)

        friend class EventDispatcher;
    };

    class QWK_CORE_EXPORT EventDispatcher {
    public:
        EventDispatcher();
        virtual ~EventDispatcher();

        virtual bool dispatch(QEvent *event);

    public:
        void addObserver(EventObserver *observer);
        void removeObserver(EventObserver *observer);

    protected:
        QVector<EventObserver *> m_observers;

        Q_DISABLE_COPY(EventDispatcher)
    };

}

#endif // EVENTFILTER_P_H
