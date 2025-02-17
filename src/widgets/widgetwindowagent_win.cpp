// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "widgetwindowagent_p.h"

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtGui/QPainter>

#include <QtCore/private/qcoreapplication_p.h>

#include <QWKCore/qwindowkit_windows.h>
#include <QWKCore/private/qwkglobal_p.h>
#include <QWKCore/private/windows10borderhandler_p.h>

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

    class WidgetBorderHandler : public QObject, public Windows10BorderHandler {
    public:
        explicit WidgetBorderHandler(QWidget *widget, AbstractWindowContext *ctx,
                                     QObject *parent = nullptr)
            : QObject(parent), Windows10BorderHandler(ctx), widget(widget) {
            widget->installEventFilter(this);

            // First update
            if (ctx->windowId()) {
                setupNecessaryAttributes();
            }
            WidgetBorderHandler::updateGeometry();
        }

        void updateGeometry() override {
            // The window top border is manually painted by QWK so we want to give
            // some margins to avoid it covering real window contents, however, we
            // found that there are some rounding issues for the thin border and
            // thus this small trick doesn't work very well when the DPR is not
            // integer. So far we haven't found a perfect solution, so just don't
            // set any margins. In theory the window content will only be covered
            // by 1px or so, it should not be a serious issue in the real world.
            //
            // widget->setContentsMargins(isNormalWindow() ? QMargins(0, borderThickness(), 0, 0)
            //                                             : QMargins());
        }

        bool isWindowActive() const override {
            return widget->isActiveWindow();
        }

        inline void forwardEventToWidgetAndDraw(QWidget *w, QEvent *event) {
            // https://github.com/qt/qtbase/blob/e26a87f1ecc40bc8c6aa5b889fce67410a57a702/src/widgets/kernel/qapplication.cpp#L3286
            // Deliver the event
            if (!forwardObjectEventFilters(this, w, event)) {
                // Let the widget paint first
                std::ignore = static_cast<QObject *>(w)->event(event);
                QCoreApplicationPrivate::setEventSpontaneous(event, false);
            }

            // Due to the timer or user action, Qt will repaint some regions spontaneously,
            // even if there is no WM_PAINT message, we must wait for it to finish painting
            // and then update the top border area.
            drawBorderNative();
        }

        inline void forwardEventToWindowAndDraw(QWindow *window, QEvent *event) {
            // https://github.com/qt/qtbase/blob/e26a87f1ecc40bc8c6aa5b889fce67410a57a702/src/widgets/kernel/qapplication.cpp#L3286
            // Deliver the event
            if (!forwardObjectEventFilters(ctx, window, event)) {
                // Let Qt paint first
                std::ignore = static_cast<QObject *>(window)->event(event);
                QCoreApplicationPrivate::setEventSpontaneous(event, false);
            }

            // Upon receiving the WM_PAINT message, Qt will repaint the entire view, and we
            // must wait for it to finish painting before drawing this top border area.
            drawBorderNative();
        }

    protected:
        bool sharedEventFilter(QObject *obj, QEvent *event) override {
            Q_UNUSED(obj)

            switch (event->type()) {
                case QEvent::Expose: {
                    // Qt will absolutely send a QExposeEvent or QResizeEvent to the QWindow when it
                    // receives a WM_PAINT message. When the control flow enters the expose handler,
                    // Qt must have already called BeginPaint() and it's the best time for us to
                    // draw the top border.

                    // Since a QExposeEvent will be sent immediately after the QResizeEvent, we can
                    // simply ignore it.
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                    struct ExposeEvent : public QExposeEvent {
                        inline const QRegion &getRegion() const { return m_region; }
                    };
                    auto ee = static_cast<ExposeEvent *>(event);
                    bool exposeRegionValid = !ee->getRegion().isNull();
#else
                    auto ee = static_cast<QExposeEvent *>(event);
                    bool exposeRegionValid = !ee->region().isNull();
#endif
                    auto window = widget->windowHandle();
                    if (window->isExposed() && isNormalWindow() && exposeRegionValid) {
                        forwardEventToWindowAndDraw(window, event);
                        return true;
                    }
                    break;
                }
                default:
                    break;
            }
            return Windows10BorderHandler::sharedEventFilter(obj, event);
        }

        bool eventFilter(QObject *obj, QEvent *event) override {
            Q_UNUSED(obj)

            switch (event->type()) {
                case QEvent::UpdateRequest: {
                    if (!isNormalWindow())
                        break;
                    forwardEventToWidgetAndDraw(widget, event);
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
