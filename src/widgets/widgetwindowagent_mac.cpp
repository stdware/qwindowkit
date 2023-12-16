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
            switch (event->type()) {
                case QEvent::Move:
                case QEvent::Resize: {
                    ctx->setSystemButtonArea(widget->geometry());
                    break;
                }

                default:
                    break;
            }
            return QObject::eventFilter(obj, event);
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
        d->systemButtonAreaWidget = widget;
        if (!widget) {
            systemButtonAreaWidgetEventFilter.reset();
            d->context->setSystemButtonArea({});
            return;
        }
        systemButtonAreaWidgetEventFilter =
            std::make_unique<SystemButtonAreaWidgetEventFilter>(widget, d->context);
    }

}