#include "widgetwindowagent.h"
#include "widgetwindowagent_p.h"

#include <QtGui/QtEvents>
#include <QtGui/QPainter>

#include "widgetitemdelegate_p.h"

namespace QWK {

    class WidgetPaintFilter : public QObject {
    public:
        WidgetPaintFilter(QWidget *widget, AbstractWindowContext *ctx) : widget(widget), ctx(ctx) {
            widget->installEventFilter(this);
        }

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override {
            switch (event->type()) {
                case QEvent::Paint: {
                    auto pe = static_cast<QPaintEvent *>(event);
                    QPainter painter(widget);
                    QRect rect = pe->rect();
                    QRegion region = pe->region();
                    void *args[] = {
                        &painter,
                        &rect,
                        &region,
                    };
                    ctx->virtual_hook(AbstractWindowContext::DrawBordersHook, args);
                    return true;
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
            d->paintFilter = std::make_unique<WidgetPaintFilter>(w, d->context.get());
        }

        if (d->context->key() == "win32") {
            w->setContentsMargins(0, 1, 0, 0);
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
