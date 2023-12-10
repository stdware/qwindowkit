#include "sharedeventfilter.h"

namespace QWK {

    class EventFilterForwarder : public QObject {
    public:
        explicit EventFilterForwarder(SharedEventDispatcherPrivate *master,
                                      QObject *parent = nullptr)
            : QObject(parent), master(master) {
        }

        bool eventFilter(QObject *obj, QEvent *event) override;

    protected:
        SharedEventDispatcherPrivate *master;
    };

    class SharedEventDispatcherPrivate {
    public:
        explicit SharedEventDispatcherPrivate(SharedEventDispatcher *q)
            : q(q), forwarder(new EventFilterForwarder(this)) {
        }
        ~SharedEventDispatcherPrivate() = default;

        bool dispatch(QObject *obj, QEvent *event) {
            for (const auto &ef : qAsConst(eventFilters)) {
                if (ef->eventFilter(obj, event)) {
                    return true;
                }
            }
            return false;
        }

        inline void install(SharedEventFilter *eventFilter) {
            bool empty = eventFilters.isEmpty();

            eventFilters.append(eventFilter);
            eventFilter->m_dispatcher = q;

            if (empty) {
                q->target()->installEventFilter(forwarder.get());
            }
        }

        inline void uninstall(SharedEventFilter *eventFilter) {
            if (!eventFilters.removeOne(eventFilter)) {
                return;
            }
            eventFilter->m_dispatcher = nullptr;

            if (eventFilters.isEmpty()) {
                q->target()->removeEventFilter(forwarder.get());
            }
        }

        SharedEventDispatcher *q;

        std::unique_ptr<EventFilterForwarder> forwarder;
        QVector<SharedEventFilter *> eventFilters;
    };

    bool EventFilterForwarder::eventFilter(QObject *obj, QEvent *event) {
        return master->dispatch(obj, event);
    }

    SharedEventFilter::SharedEventFilter() : m_dispatcher(nullptr) {
    }

    SharedEventFilter::~SharedEventFilter() {
        if (m_dispatcher)
            m_dispatcher->removeSharedEventFilter(this);
    }

    SharedEventDispatcher::SharedEventDispatcher() : d(new SharedEventDispatcherPrivate(this)) {
    }

    SharedEventDispatcher::~SharedEventDispatcher() {
        delete d;
    }

    void SharedEventDispatcher::installSharedEventFilter(SharedEventFilter *eventFilter) {
        d->install(eventFilter);
    }

    void SharedEventDispatcher::removeSharedEventFilter(SharedEventFilter *eventFilter) {
        d->uninstall(eventFilter);
    }


}