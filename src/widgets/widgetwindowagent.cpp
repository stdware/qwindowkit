#include "widgetwindowagent.h"
#include "widgetwindowagent_p.h"

#include "widgetitemdelegate_p.h"

namespace QWK {

    WidgetWindowAgentPrivate::WidgetWindowAgentPrivate() {
    }

    WidgetWindowAgentPrivate::~WidgetWindowAgentPrivate() {
    }

    void WidgetWindowAgentPrivate::init() {
    }

    WidgetWindowAgent::WidgetWindowAgent(QObject *parent)
        : WidgetWindowAgent(*new WidgetWindowAgentPrivate(), parent) {
    }

    WidgetWindowAgent::~WidgetWindowAgent() {
    }

    bool WidgetWindowAgent::setup(QWidget *w) {
        Q_ASSERT(w);
        if (!w) {
            return false;
        }

        Q_D(WidgetWindowAgent);
        if (d->host) {
            return false;
        }

        std::ignore = w->winId(); // Make sure the window handle is created
        if (!d->setup(w->windowHandle(), new WidgetItemDelegate())) {
            return true;
        }
        d->host = w;
        return true;
    }

    bool WidgetWindowAgent::isHitTestVisible(QWidget *w) const {
        Q_D(const WidgetWindowAgent);
        return d->m_eventHandler->isHitTestVisible(w);
    }

    void WidgetWindowAgent::setHitTestVisible(QWidget *w, bool visible) {
        Q_D(WidgetWindowAgent);
        d->m_eventHandler->setHitTestVisible(w, visible);
    }

    void WidgetWindowAgent::setHitTestVisible(const QRect &rect, bool visible) {
        Q_D(WidgetWindowAgent);
        d->m_eventHandler->setHitTestVisible(rect, visible);
    }

    QWidget *WidgetWindowAgent::systemButton(CoreWindowAgent::SystemButton button) const {
        Q_D(const WidgetWindowAgent);
        return static_cast<QWidget *>(d->m_eventHandler->systemButton(button));
    }

    void WidgetWindowAgent::setSystemButton(CoreWindowAgent::SystemButton button, QWidget *w) {
        Q_D(WidgetWindowAgent);
        if (!d->m_eventHandler->setSystemButton(button, w)) {
            return;
        }
        Q_EMIT systemButtonChanged(button, w);
    }

    QWidget *WidgetWindowAgent::titleBar() const {
        Q_D(const WidgetWindowAgent);
        return static_cast<QWidget *>(d->m_eventHandler->titleBar());
    }

    void WidgetWindowAgent::setTitleBar(QWidget *w) {
        Q_D(WidgetWindowAgent);
        if (!d->m_eventHandler->setTitleBar(w)) {
            return;
        }
        Q_EMIT titleBarWidgetChanged(w);
    }

    WidgetWindowAgent::WidgetWindowAgent(WidgetWindowAgentPrivate &d, QObject *parent)
        : CoreWindowAgent(d, parent) {
        d.init();
    }
}
