// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "widgetwindowagent_p.h"

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtGui/QPainter>

#include <QWKCore/qwindowkit_windows.h>
#include <QWKCore/private/qwkglobal_p.h>

namespace QWK {

#if QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)
    // https://github.com/qt/qtbase/blob/e26a87f1ecc40bc8c6aa5b889fce67410a57a702/src/plugins/platforms/windows/qwindowsbackingstore.cpp#L42
    // In QtWidgets applications, when repainting happens, QPA at the last calls
    // QWindowsBackingStore::flush() to draw the contents of the buffer to the screen, we need to
    // call GDI drawing the top border after that.

    // After debugging, we know that there are two situations that will lead to repaint.
    //
    // 1. Windows sends a WM_PAINT message, after which Qt immediately generates a QExposeEvent or
    // QResizeEvent and send it to the corresponding QWidgetWindow instance, calling "flush" at the
    // end of its handler.
    //
    // 2. When a timer or user input triggers Qt to repaint spontaneously, the corresponding
    // QWidget receives a QEvent::UpdateRequest event and also calls "flush" at the end of its
    // handler.
    //
    // The above two cases are mutually exclusive, so we just need to intercept the two events
    // separately and draw the border area after the "flush" is called.

    // https://github.com/qt/qtbase/blob/e26a87f1ecc40bc8c6aa5b889fce67410a57a702/src/plugins/platforms/windows/qwindowswindow.cpp#L2440
    // Note that we can not draw the border right after WM_PAINT comes or right before the WndProc
    // returns, because Qt calls BeginPaint() and EndPaint() itself. We should make sure that we
    // draw the top border between these two calls, otherwise some display exceptions may arise.

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

            // Enable dark mode by default, otherwise the system borders are white
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

        inline void resumeWidgetEventAndDraw(QWidget *w, QEvent *event) {
            // Let the widget paint first
            static_cast<QObject *>(w)->event(event);

            // Due to the timer or user action, Qt will repaint some regions spontaneously,
            // even if there is no WM_PAINT message, we must wait for it to finish painting
            // and then update the top border area.
            ctx->virtual_hook(AbstractWindowContext::DrawWindows10BorderHook2, nullptr);
        }

        inline void resumeWindowEventAndDraw(QWindow *window, QEvent *event) {
            // Let Qt paint first
            static_cast<QObject *>(window)->event(event);

            // Upon receiving the WM_PAINT message, Qt will repaint the entire view, and we
            // must wait for it to finish painting before drawing this top border area.
            ctx->virtual_hook(AbstractWindowContext::DrawWindows10BorderHook2, nullptr);
        }

        inline void updateExtraMargins(bool windowActive) {
            if (windowActive) {
                // Restore margins when the window is active
                static QVariant defaultMargins = QVariant::fromValue(QMargins(0, 1, 0, 0));
                ctx->setWindowAttribute(QStringLiteral("extra-margins"), defaultMargins);
                return;
            }

            // https://github.com/microsoft/terminal/blob/71a6f26e6ece656084e87de1a528c4a8072eeabd/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp#L904
            // When the window is inactive, there is a transparency bug in the top
            // border, and we need to extend the non-client area to the whole title
            // bar.
            QRect frame = ctx->windowAttribute(QStringLiteral("window-rect")).toRect();
            QMargins margins{0, -frame.top(), 0, 0};
            ctx->setWindowAttribute(QStringLiteral("extra-margins"), QVariant::fromValue(margins));
        }

    protected:
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            Q_UNUSED(eventType)

            const auto msg = static_cast<const MSG *>(message);
            switch (msg->message) {
                case WM_DPICHANGED: {
                    updateGeometry();
                    updateExtraMargins(widget->isActiveWindow());
                    break;
                }

                case WM_ACTIVATE: {
                    updateExtraMargins(LOWORD(msg->wParam) != WA_INACTIVE);
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
            switch (event->type()) {
                case QEvent::Expose: {
                    // Qt will absolutely send a QExposeEvent or QResizeEvent to the QWindow when it
                    // receives a WM_PAINT message. When the control flow enters the expose handler,
                    // Qt must have already called BeginPaint() and it's the best time for us to
                    // draw the top border.

                    // Since a QExposeEvent will be sent immediately after the QResizeEvent, we can
                    // simply ignore it.
                    auto ee = static_cast<QExposeEvent *>(event);
                    if (window->isExposed() && isNormalWindow() && !ee->region().isNull()) {
                        resumeWindowEventAndDraw(window, event);
                        return true;
                    }
                    break;
                }
                case QEvent::WinIdChange: {
                    if (auto winId = ctx->windowId()) {
                        updateGeometry();
                    }
                    break;
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
                    if (!isNormalWindow())
                        break;
                    resumeWidgetEventAndDraw(widget, event);
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
