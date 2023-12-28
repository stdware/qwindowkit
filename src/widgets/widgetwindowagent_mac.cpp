#include "widgetwindowagent_p.h"

#include <QtGui/QtEvents>

namespace QWK {

    static inline QRect getWidgetSceneRect(QWidget *widget) {
        return {widget->mapTo(widget->window(), QPoint()), widget->size()};
    }

    class SystemButtonAreaWidgetEventFilter : public QObject {
    public:
        SystemButtonAreaWidgetEventFilter(QWidget *widget, AbstractWindowContext *ctx,
                                          QObject *parent = nullptr)
            : QObject(parent), widget(widget), ctx(ctx) {
            widget->installEventFilter(this);
        }
        ~SystemButtonAreaWidgetEventFilter() override = default;

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override {
            Q_UNUSED(obj)
            switch (event->type()) {
                case QEvent::Move:
                case QEvent::Resize: {
                    ctx->setSystemButtonArea(getWidgetSceneRect(widget));
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

    /*!
        Returns the widget that acts as the system button area.
    */
    QWidget *WidgetWindowAgent::systemButtonArea() const {
        Q_D(const WidgetWindowAgent);
        return d->systemButtonAreaWidget;
    }

    /*!
        Sets the widget that acts as the system button area. The system button will be centered in
        its area, it is recommended to place the widget in a layout and set a fixed size policy.
    */
    void WidgetWindowAgent::setSystemButtonArea(QWidget *widget) {
        Q_D(WidgetWindowAgent);
        if (d->systemButtonAreaWidget == widget)
            return;

        auto ctx = d->context.get();
        d->systemButtonAreaWidget = widget;
        if (!widget) {
            d->systemButtonAreaWidgetEventFilter.reset();
            ctx->setSystemButtonArea({});
            return;
        }
        d->systemButtonAreaWidgetEventFilter =
            std::make_unique<SystemButtonAreaWidgetEventFilter>(widget, ctx);
        ctx->setSystemButtonArea(getWidgetSceneRect(widget));
    }

}