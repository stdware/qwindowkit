#include "eventobserver_p.h"

namespace QWK {

    EventObserver::EventObserver() : m_dispatcher(nullptr) {
    }

    EventObserver::~EventObserver() {
        if (m_dispatcher)
            m_dispatcher->removeObserver(this);
    }

    EventDispatcher::EventDispatcher() = default;

    EventDispatcher::~EventDispatcher() {
        for (const auto &observer : std::as_const(m_observers)) {
            observer->m_dispatcher = nullptr;
        }
    }

    bool EventDispatcher::dispatch(QEvent *event) {
        for (const auto &observer : std::as_const(m_observers)) {
            if (observer->observe(event))
                return true;
        }
        return true;
    }

    void EventDispatcher::addObserver(EventObserver *observer) {
        if (!observer || observer->m_dispatcher)
            return;

        m_observers.append(observer);
        observer->m_dispatcher = this;
    }

    void EventDispatcher::removeObserver(EventObserver *observer) {
        if (!m_observers.removeOne(observer)) {
            return;
        }
        observer->m_dispatcher = nullptr;
    }

}