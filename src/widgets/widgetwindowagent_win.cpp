#include "widgetwindowagent_p.h"

#include <QtGui/QPainter>

#include <QWKCore/private/eventobserver_p.h>

namespace QWK {

    class WidgetBorderHandler : public QObject, public EventObserver {
        Q_OBJECT
    public:
        explicit WidgetBorderHandler(QWidget *widget, AbstractWindowContext *ctx,
                                     QObject *parent = nullptr)
            : QObject(parent), widget(widget), ctx(ctx) {
            widget->installEventFilter(this);

            ctx->addObserver(this);
            updateGeometry();
        }

        void updateGeometry() {
            if (widget->windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen)) {
                widget->setContentsMargins({});
            } else {
                widget->setContentsMargins({
                    0,
                    ctx->property("borderThickness").toInt(),
                    0,
                    0,
                });
            }
        }

    protected:
        bool observe(QEvent *event) override {
            switch (event->type()) {
                case QEvent::UpdateLater: {
                    widget->update();
                    break;
                }

                case QEvent::ScreenChangeInternal: {
                    updateGeometry();
                    break;
                }

                default:
                    break;
            }
            return false;
        }

        bool eventFilter(QObject *obj, QEvent *event) override {
            switch (event->type()) {
                case QEvent::Paint: {
                    if (widget->windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen))
                        break;

                    auto paintEvent = static_cast<QPaintEvent *>(event);
                    auto rect = paintEvent->rect();
                    auto region = paintEvent->region();

                    QPainter painter(widget);
                    void *args[] = {
                        &painter,
                        &rect,
                        &region,
                    };
                    ctx->virtual_hook(AbstractWindowContext::DrawWindows10BorderHook, args);
                    return true;
                }

                case QEvent::WindowStateChange: {
                    updateGeometry();
                    break;
                }

                case QEvent::WindowActivate:
                case QEvent::WindowDeactivate: {
                    widget->update();
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

    void WidgetWindowAgentPrivate::setupWindows10BorderWorkaround() {
        // Install painting hook
        auto ctx = context.get();
        if (ctx->property("needBorderPainter").toBool()) {
            std::ignore = new WidgetBorderHandler(hostWidget, ctx, ctx);
        }
    }

}

#include "widgetwindowagent_win.moc"