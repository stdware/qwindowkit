#include "widgetwindowagent_p.h"

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtGui/QPainter>

#include <QWKCore/qwindowkit_windows.h>

namespace QWK {

#if QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDER)

    class WidgetBorderHandler : public QObject, public NativeEventFilter, public SharedEventFilter {
    public:
        explicit WidgetBorderHandler(QWidget *widget, AbstractWindowContext *ctx,
                                     QObject *parent = nullptr)
            : QObject(parent), widget(widget), ctx(ctx) {
            widget->installEventFilter(this);

            // https://github.com/microsoft/terminal/blob/71a6f26e6ece656084e87de1a528c4a8072eeabd/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp#L940
            // Must extend top frame to client area
            static QVariant defaultMargins = QVariant::fromValue(QMargins(0, 1, 0, 0));
            ctx->setWindowAttribute(QStringLiteral("extra-margins"), defaultMargins);
            ctx->setWindowAttribute(QStringLiteral("dark-mode"), true);

            ctx->installNativeEventFilter(this);
            ctx->installSharedEventFilter(this);

            updateGeometry();
        }

        inline bool isNormalWindow() const {
            return !(widget->windowState() &
                     (Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen));
        }

        inline void updateGeometry() {
            if (isNormalWindow()) {
                widget->setContentsMargins(
                    {0, ctx->windowAttribute(QStringLiteral("border-thickness")).toInt(), 0, 0});
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

                case WM_ACTIVATE: {
                    if (LOWORD(msg->wParam) == WA_INACTIVE) {
                        // https://github.com/microsoft/terminal/blob/71a6f26e6ece656084e87de1a528c4a8072eeabd/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp#L904
                        // When the window is inactive, there is a transparency bug in the top
                        // border, and we need to extend the non-client area to the whole title
                        // bar.
                        QRect frame =
                            ctx->windowAttribute(QStringLiteral("title-bar-rect")).toRect();
                        QMargins margins{0, -frame.top(), 0, 0};
                        ctx->setWindowAttribute(QStringLiteral("extra-margins"),
                                                QVariant::fromValue(margins));
                    } else {
                        // Restore margins when the window is active
                        static QVariant defaultMargins = QVariant::fromValue(QMargins(0, 1, 0, 0));
                        ctx->setWindowAttribute(QStringLiteral("extra-margins"), defaultMargins);
                    }
                    break;
                }

                default:
                    break;
            }
            return false;
        }

        bool sharedEventFilter(QObject *obj, QEvent *event) override {
            Q_UNUSED(obj)

            auto window = widget->windowHandle();
            if (event->type() == QEvent::Expose) {
                // Qt will absolutely send a QExposeEvent to the QWindow when it receives a
                // WM_PAINT message. When the control flow enters the expose handler, Qt must
                // have already called BeginPaint() and it's the best time for us to draw the
                // top border.
                auto ee = static_cast<QExposeEvent *>(event);
                if (isNormalWindow() && window->isExposed() && !ee->region().isNull()) {
                    // Friend class helping to call `event`
                    class HackedWindow : public QWindow {
                    public:
                        friend class QWK::WidgetBorderHandler;
                    };

                    // Let Qt paint first
                    static_cast<HackedWindow *>(window)->event(event);

                    // Upon receiving the WM_PAINT message, Qt will redraw the entire view, and we
                    // must wait for it to finish redrawing before drawing this top border area.
                    ctx->virtual_hook(AbstractWindowContext::DrawWindows10BorderHook2, nullptr);
                    return true;
                }
            }
            return false;
        }

        bool eventFilter(QObject *obj, QEvent *event) override {
            Q_UNUSED(obj)
            switch (event->type()) {
                case QEvent::UpdateRequest: {
                    if (!isNormalWindow())
                        break;

                    // Friend class helping to call `event`
                    class HackedWidget : public QWidget {
                    public:
                        friend class QWK::WidgetBorderHandler;
                    };

                    // Let the widget paint first
                    static_cast<HackedWidget *>(widget)->event(event);

                    // Due to the timer or user action, Qt will redraw some regions spontaneously,
                    // even if there is no WM_PAINT message, we must wait for it to finish redrawing
                    // and then update the top border area.
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

    void WidgetWindowAgentPrivate::setupWindows10BorderWorkaround() {
        // Install painting hook
        auto ctx = context.get();
        if (ctx->windowAttribute(QStringLiteral("win10-border-needed")).toBool()) {
            borderHandler = std::make_unique<WidgetBorderHandler>(hostWidget, ctx);
        }
    }
#endif

}
