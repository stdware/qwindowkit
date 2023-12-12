#include "widgetwindowagent.h"
#include "widgetwindowagent_p.h"

#include <QtGui/QtEvents>
#include <QtGui/QPainter>
#include <QtCore/QDebug>

#ifdef Q_OS_WINDOWS
#  include <QWKCore/private/win10borderhandler_p.h>
#endif

#include "widgetitemdelegate_p.h"

namespace QWK {

#ifdef Q_OS_WINDOWS
    class WidgetBorderHandler : public QObject, public Win10BorderHandler {
    public:
        explicit WidgetBorderHandler(QWidget *widget)
            : Win10BorderHandler(widget->windowHandle()), widget(widget) {
            widget->installEventFilter(this);
        }

        void updateGeometry() override {
            if (widget->windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen)) {
                widget->setContentsMargins({});
            } else {
                widget->setContentsMargins({0, int(m_borderThickness), 0, 0});
            }
        }

        void requestUpdate() override {
            widget->update();
        }

        bool isActive() const override {
            return widget->isActiveWindow();
        }

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override {
            switch (event->type()) {
                case QEvent::Paint: {
                    if (widget->windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen))
                        break;
                    auto paintEvent = static_cast<QPaintEvent *>(event);
                    QPainter painter(widget);
                    paintBorder(painter, paintEvent->rect(), paintEvent->region());
                    return true;
                }
                default:
                    break;
            }
            return false;
        }

        QWidget *widget;
    };
#endif

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
        if (d->hostWidget) {
            return false;
        }

        w->setAttribute(Qt::WA_DontCreateNativeAncestors);
        w->setAttribute(Qt::WA_NativeWindow);

        if (!d->setup(w, new WidgetItemDelegate())) {
            return false;
        }
        d->hostWidget = w;

#ifdef Q_OS_WINDOWS
        // Install painting hook
        if (bool needPaintBorder;
            QMetaObject::invokeMethod(d->context.get(), "needWin10BorderHandler",
                                      Qt::DirectConnection, Q_RETURN_ARG(bool, needPaintBorder)),
            needPaintBorder) {
            QMetaObject::invokeMethod(d->context.get(), "setWin10BorderHandler",
                                      Qt::DirectConnection,
                                      Q_ARG(Win10BorderHandler *, new WidgetBorderHandler(w)));
        }
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

    void WidgetWindowAgent::setHitTestVisible(const QRect &rect, bool visible) {
        Q_D(WidgetWindowAgent);
        d->context->setHitTestVisible(rect, visible);
    }

    WidgetWindowAgent::WidgetWindowAgent(WidgetWindowAgentPrivate &d, QObject *parent)
        : WindowAgentBase(d, parent) {
        d.init();
    }
}
