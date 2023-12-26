#include "widgetwindowagent_p.h"

#include <QtGui/QtEvents>

namespace QWK {

    class SystemButtonAreaWidgetEventFilter : public QObject {
    public:
        SystemButtonAreaWidgetEventFilter(QWidget *widget, AbstractWindowContext *ctx,
                                          QObject *parent = nullptr)
            : QObject(parent), widget(widget), ctx(ctx) {
            widget->installEventFilter(this);
        }
        ~SystemButtonAreaWidgetEventFilter() = default;

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override {
            Q_UNUSED(obj)
            switch (event->type()) {
                case QEvent::Move:
                case QEvent::Resize: {
                    QRect rect(widget->mapTo(widget->window(), {}), widget->size());
                    ctx->setSystemButtonArea(rect);
                    break;
                }

                default:
                    break;
            }
            return false;
        }

    protected:
        QWidget *widget;
        AbstractWindowContext *ctx;
    };

    QWidget *WidgetWindowAgent::systemButtonArea() const {
        Q_D(const WidgetWindowAgent);
        return d->systemButtonAreaWidget;
    }

    void WidgetWindowAgent::setSystemButtonArea(QWidget *widget) {
        Q_D(WidgetWindowAgent);
        auto ctx = d->context.get();
        d->systemButtonAreaWidget = widget;
        if (!widget) {
            d->systemButtonAreaWidgetEventFilter.reset();
            ctx->setSystemButtonArea({});
            return;
        }
        d->systemButtonAreaWidgetEventFilter =
            std::make_unique<SystemButtonAreaWidgetEventFilter>(widget, ctx);
    }

}