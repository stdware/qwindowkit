#include "quickwindowagent.h"
#include "quickwindowagent_p.h"

#include "quickitemdelegate_p.h"

#include <QtQuick/QQuickWindow>

namespace QWK {

    QuickWindowAgentPrivate::QuickWindowAgentPrivate() {
    }

    QuickWindowAgentPrivate::~QuickWindowAgentPrivate() {
    }

    void QuickWindowAgentPrivate::init() {
    }

    QuickWindowAgent::QuickWindowAgent(QObject *parent)
        : QuickWindowAgent(*new QuickWindowAgentPrivate(), parent) {
    }

    QuickWindowAgent::~QuickWindowAgent() {
    }

    void QuickWindowAgent::setup(QQuickWindow *window) {
        Q_ASSERT(window);
        if (!window) {
            return;
        }

        Q_D(QuickWindowAgent);
        if (d->host) {
            return;
        }
        d->host = window;

        d->setup(window, new QuickItemDelegate());
    }

    bool QuickWindowAgent::isHitTestVisible(QQuickItem *item) const {
        Q_D(const QuickWindowAgent);
        return d->m_eventHandler->isHitTestVisible(item);
    }

    void QuickWindowAgent::setHitTestVisible(QQuickItem *item, bool visible) {
        Q_D(QuickWindowAgent);
        d->m_eventHandler->setHitTestVisible(item, visible);
    }

    void QuickWindowAgent::setHitTestVisible(const QRect &rect, bool visible) {
        Q_D(QuickWindowAgent);
        d->m_eventHandler->setHitTestVisible(rect, visible);
    }

    QQuickItem *QuickWindowAgent::systemButton(SystemButton button) const {
        Q_D(const QuickWindowAgent);
        return static_cast<QQuickItem *>(d->m_eventHandler->systemButton(button));
    }

    void QuickWindowAgent::setSystemButton(SystemButton button, QQuickItem *item) {
        Q_D(QuickWindowAgent);
        if (!d->m_eventHandler->setSystemButton(button, item)) {
            return;
        }
        Q_EMIT systemButtonChanged(button, item);
    }

    QQuickItem *QuickWindowAgent::titleBar() const {
        Q_D(const QuickWindowAgent);
        return static_cast<QQuickItem *>(d->m_eventHandler->titleBar());
    }

    void QuickWindowAgent::setTitleBar(QQuickItem *item) {
        Q_D(QuickWindowAgent);
        if (!d->m_eventHandler->setTitleBar(item)) {
            return;
        }
        Q_EMIT titleBarWidgetChanged(item);
    }

    QuickWindowAgent::QuickWindowAgent(QuickWindowAgentPrivate &d, QObject *parent)
        : CoreWindowAgent(d, parent) {
        d.init();
    }
}
