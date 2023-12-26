#include "widgetwindowagent.h"
#include "widgetwindowagent_p.h"

#include <QtGui/QtEvents>
#include <QtGui/QPainter>
#include <QtCore/QDebug>

#include "widgetitemdelegate_p.h"

namespace QWK {

    WidgetWindowAgentPrivate::WidgetWindowAgentPrivate() = default;

    WidgetWindowAgentPrivate::~WidgetWindowAgentPrivate() = default;

    void WidgetWindowAgentPrivate::init() {
    }

    WidgetWindowAgent::WidgetWindowAgent(QObject *parent)
        : WidgetWindowAgent(*new WidgetWindowAgentPrivate(), parent) {
    }

    WidgetWindowAgent::~WidgetWindowAgent() = default;

    bool WidgetWindowAgent::setup(QWidget *w) {
        Q_ASSERT(w);
        if (!w) {
            return false;
        }

        Q_D(WidgetWindowAgent);
        if (d->hostWidget) {
            return false;
        }

        w->setAttribute(Qt::WA_DontCreateNativeAncestors);
        w->setAttribute(Qt::WA_NativeWindow);

        d->setup(w, new WidgetItemDelegate());
        d->hostWidget = w;

#if defined(Q_OS_WINDOWS) && QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)
        d->setupWindows10BorderWorkaround();
#endif
        return true;
    }

    QWidget *WidgetWindowAgent::titleBar() const {
        Q_D(const WidgetWindowAgent);
        return static_cast<QWidget *>(d->context->titleBar());
    }

    void WidgetWindowAgent::setTitleBar(QWidget *w) {
        Q_D(WidgetWindowAgent);
        if (!d->context->setTitleBar(w)) {
            return;
        }
        Q_EMIT titleBarWidgetChanged(w);
    }

    QWidget *WidgetWindowAgent::systemButton(SystemButton button) const {
        Q_D(const WidgetWindowAgent);
        return static_cast<QWidget *>(d->context->systemButton(button));
    }

    void WidgetWindowAgent::setSystemButton(SystemButton button, QWidget *w) {
        Q_D(WidgetWindowAgent);
        if (!d->context->setSystemButton(button, w)) {
            return;
        }
        Q_EMIT systemButtonChanged(button, w);
    }

    bool WidgetWindowAgent::isHitTestVisible(const QWidget *w) const {
        Q_D(const WidgetWindowAgent);
        return d->context->isHitTestVisible(w);
    }

    void WidgetWindowAgent::setHitTestVisible(const QWidget *w, bool visible) {
        Q_D(WidgetWindowAgent);
        d->context->setHitTestVisible(w, visible);
    }

    WidgetWindowAgent::WidgetWindowAgent(WidgetWindowAgentPrivate &d, QObject *parent)
        : WindowAgentBase(d, parent) {
        d.init();
    }
}
