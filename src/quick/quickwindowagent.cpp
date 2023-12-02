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

    bool QuickWindowAgent::setup(QQuickWindow *window) {
        Q_ASSERT(window);
        if (!window) {
            return false;
        }

        Q_D(QuickWindowAgent);
        if (d->host) {
            return false;
        }

        if (!d->setup(window, new QuickItemDelegate())) {
            return true;
        }
        d->host = window;
        return true;
    }

    bool QuickWindowAgent::isHitTestVisible(QQuickItem *item) const {
        Q_D(const QuickWindowAgent);
        return d->eventHandler->isHitTestVisible(item);
    }

    void QuickWindowAgent::setHitTestVisible(QQuickItem *item, bool visible) {
        Q_D(QuickWindowAgent);
        d->eventHandler->setHitTestVisible(item, visible);
    }

    void QuickWindowAgent::setHitTestVisible(const QRect &rect, bool visible) {
        Q_D(QuickWindowAgent);
        d->eventHandler->setHitTestVisible(rect, visible);
    }

    QQuickItem *QuickWindowAgent::systemButton(SystemButton button) const {
        Q_D(const QuickWindowAgent);
        return qobject_cast<QQuickItem *>(d->eventHandler->systemButton(button));
    }

    void QuickWindowAgent::setSystemButton(SystemButton button, QQuickItem *item) {
        Q_D(QuickWindowAgent);
        if (!d->eventHandler->setSystemButton(button, item)) {
            return;
        }
        Q_EMIT systemButtonChanged(button, item);
    }

    QQuickItem *QuickWindowAgent::titleBar() const {
        Q_D(const QuickWindowAgent);
        return qobject_cast<QQuickItem *>(d->eventHandler->titleBar());
    }

    void QuickWindowAgent::setTitleBar(QQuickItem *item) {
        Q_D(QuickWindowAgent);
        if (!d->eventHandler->setTitleBar(item)) {
            return;
        }
        Q_EMIT titleBarWidgetChanged(item);
    }

    QuickWindowAgent::QuickWindowAgent(QuickWindowAgentPrivate &d, QObject *parent)
        : CoreWindowAgent(d, parent) {
        d.init();
    }
}
