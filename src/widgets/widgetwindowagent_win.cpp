#include "widgetwindowagent_p.h"

#include <QtGui/QPainter>

#include <QWKCore/qwindowkit_windows.h>
#include <QWKCore/qwkconfig.h>
#include <QWKCore/private/nativeeventfilter_p.h>

namespace QWK {

#if QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDER)

    class WidgetBorderHandler;

    class WidgetBorderHandler : public QObject, public NativeEventFilter {
    public:
        explicit WidgetBorderHandler(QWidget *widget, AbstractWindowContext *ctx)
            : QObject(ctx), widget(widget), ctx(ctx) {
            widget->installEventFilter(this);

            ctx->installNativeEventFilter(this);
            updateGeometry();
        }

        inline bool isNormalWindow() const {
            return widget->windowState() &
                   (Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen);
        }

        void updateGeometry() {
            if (isNormalWindow()) {
                widget->setContentsMargins({
                    0,
                    ctx->property("borderThickness").toInt(),
                    0,
                    0,
                });
            } else {
                widget->setContentsMargins({});
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

                case WM_PAINT: {
                    // Let Qt paint first
                    m_dispatcher->resume(eventType, message, result);

                    // Upon receiving the WM_PAINT message, Qt will redraw the entire view, and we
                    // must wait for it to finish redrawing before drawing this top border area
                    ctx->virtual_hook(AbstractWindowContext::DrawWindows10BorderHook2, nullptr);
                    return true;
                }

                default:
                    break;
            }
            return false;
        }

        bool eventFilter(QObject *obj, QEvent *event) override {
            Q_UNUSED(obj)
            switch (event->type()) {
                case QEvent::UpdateRequest: {
                    // Friend class helping to call `event`
                    class HackedWidget : public QWidget {
                    public:
                        friend class QWK::WidgetBorderHandler;
                    };

                    // Let the widget paint first
                    static_cast<HackedWidget *>(widget)->event(event);

                    // Due to the timer or user action, Qt will redraw some regions spontaneously,
                    // even if there is no WM_PAINT message, we must wait for it to finish redrawing
                    // and then update the upper border area
                    ctx->virtual_hook(AbstractWindowContext::DrawWindows10BorderHook2, nullptr);
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
#endif

    void WidgetWindowAgentPrivate::setupWindows10BorderWorkaround() {
#if QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDER)
        // Install painting hook
        auto ctx = context.get();
        if (ctx->property("needBorderPainter").toBool()) {
            std::ignore = new WidgetBorderHandler(hostWidget, ctx);
        }
#endif
    }

}
