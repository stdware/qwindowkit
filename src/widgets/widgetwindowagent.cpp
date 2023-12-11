#include "widgetwindowagent.h"
#include "widgetwindowagent_p.h"

#include <QtGui/QtEvents>
#include <QtGui/QPainter>
#include <QtCore/QDebug>

#include "widgetitemdelegate_p.h"

namespace QWK {

    class WidgetBorderHandler : public QObject {
    public:
        WidgetBorderHandler(QWidget *widget, AbstractWindowContext *ctx)
            : widget(widget), ctx(ctx) {
            widget->installEventFilter(this);
        }

        void updateMargins() {
            if (widget->windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen)) {
                widget->setContentsMargins({});
            } else {
                widget->setContentsMargins({0, 1, 0, 0});
            }
        }

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override {
            switch (event->type()) {
                case QEvent::Paint: {
                    if (widget->windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen))
                        break;

                    auto paintEvent = static_cast<QPaintEvent *>(event);
                    QPainter painter(widget);
                    QRect rect = paintEvent->rect();
                    QRegion region = paintEvent->region();
                    void *args[] = {
                        &painter,
                        &rect,
                        &region,
                    };
                    ctx->virtual_hook(AbstractWindowContext::DrawBordersHook, args);
                    return true;
                }

                case QEvent::WindowStateChange: {
                    updateMargins();
                    break;
                }
                default:
                    break;
            }
            return false;
        }

        QWidget *widget;
        AbstractWindowContext *ctx;
    };

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

        // Install painting hook
        if (bool needPaintBorder = false;
            d->context->virtual_hook(AbstractWindowContext::NeedsDrawBordersHook, &needPaintBorder),
            needPaintBorder) {
            auto borderHandler = std::make_unique<WidgetBorderHandler>(w, d->context.get());
            borderHandler->updateMargins();
            d->borderHandler = std::move(borderHandler);
        }
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
