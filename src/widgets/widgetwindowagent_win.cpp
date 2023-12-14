#include "widgetwindowagent_p.h"

#include <QtGui/QPainter>

#include <QWKCore/qwindowkit_windows.h>
#include <QWKCore/private/nativeeventfilter_p.h>

namespace QWK {

    class WidgetBorderHandler : public QObject, public NativeEventFilter {
        Q_OBJECT
    public:
        explicit WidgetBorderHandler(QWidget *widget, AbstractWindowContext *ctx)
            : QObject(ctx), widget(widget), ctx(ctx) {
            widget->installEventFilter(this);

            ctx->installNativeEventFilter(this);
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
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            Q_UNUSED(eventType)
            const auto msg = static_cast<const MSG *>(message);
            switch (msg->message) {
                case WM_DPICHANGED: {
                    updateGeometry();
                    break;
                }

                case WM_THEMECHANGED:
                case WM_SYSCOLORCHANGE:
                case WM_DWMCOLORIZATIONCOLORCHANGED: {
                    widget->update();
                    break;
                }

                case WM_SETTINGCHANGE: {
                    if (!msg->wParam && msg->lParam &&
                        std::wcscmp(reinterpret_cast<LPCWSTR>(msg->lParam), L"ImmersiveColorSet") ==
                            0) {
                        widget->update();
                    }
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
                    if (widget->windowState() & (Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen))
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
                    break;
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
            std::ignore = new WidgetBorderHandler(hostWidget, ctx);
        }
    }

}

#include "widgetwindowagent_win.moc"
