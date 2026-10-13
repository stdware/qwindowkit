// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef WINDOWS10BORDERHANDLER_P_H
#define WINDOWS10BORDERHANDLER_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QtGui/QWindow>
#include <QtCore/QEvent>

#include <QWKCore/qwindowkit_windows.h>
#include <QWKCore/private/qwkglobal_p.h>
#include <QWKCore/private/abstractwindowcontext_p.h>

namespace QWK {

    class Windows10BorderHandler : public NativeEventFilter, public SharedEventFilter {
    public:
        inline Windows10BorderHandler(AbstractWindowContext *ctx, QObject *owner)
            : ctx(ctx), owner(owner) {
            ctx->installNativeEventFilter(this);
            ctx->installSharedEventFilter(this);
        }

        inline void setupNecessaryAttributes() {
            const auto guard = owner;
            const QPointer<AbstractWindowContext> context(ctx);
            const auto windowId = ctx->windowId();
            if (!ctx->windowAttribute(QStringLiteral("extra-margins")).isValid()) {
                // https://github.com/microsoft/terminal/blob/71a6f26e6ece656084e87de1a528c4a8072eeabd/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp#L940
                // Must extend top frame to client area
                static QVariant defaultMargins = QVariant::fromValue(QMargins(0, 1, 0, 0));
                ctx->setWindowAttribute(QStringLiteral("extra-margins"), defaultMargins);
            }
            if (!guard || !context || context->windowId() != windowId)
                return;

            // Enable dark mode by default, otherwise the system borders are white
            if (!ctx->windowAttribute(QStringLiteral("dark-mode")).isValid())
                ctx->setWindowAttribute(QStringLiteral("dark-mode"), true);
            if (guard && context && context->windowId() == windowId)
                updateExtraMargins(isWindowActive());
        }

        inline bool isNormalWindow() const {
            return !(ctx->window()->windowStates() &
                     (Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen));
        }

        inline void updateExtraMargins(bool windowActive) {
            // This handler is installed only when win10-border-needed is true.
            // Activation is platform state, not a new application attribute value.
            ctx->virtual_hook(AbstractWindowContext::Windows10BorderActivationHook, &windowActive);
        }

        virtual void updateGeometry() = 0;

        virtual bool isWindowActive() const {
            return ctx->window()->isActive();
        }

    protected:
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            Q_UNUSED(eventType)

            const auto msg = static_cast<const MSG *>(message);
            switch (msg->message) {
                case WM_DPICHANGED: {
                    const auto guard = owner;
                    const QPointer<AbstractWindowContext> context(ctx);
                    updateGeometry();
                    if (guard && context)
                        updateExtraMargins(isWindowActive());
                    break;
                }

                case WM_ACTIVATE: {
                    updateExtraMargins(LOWORD(msg->wParam) != WA_INACTIVE);
                    break;
                }

                case WM_THEMECHANGED:
                case WM_SYSCOLORCHANGE:
                case WM_DWMCOLORIZATIONCOLORCHANGED: {
                    // If we do not refresh this property, the native border will turn white
                    // permanently (like the dark mode is turned off) after the user changes
                    // the accent color in system personalization settings.
                    // So we need this ugly hack to re-apply dark mode to get rid of this
                    // strange Windows bug.
                    if (ctx->windowAttribute(QStringLiteral("dark-mode")).toBool()) {
                        ctx->setWindowAttribute(QStringLiteral("dark-mode"), true);
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

            if (event->type() == QEvent::WinIdChange) {
                if (ctx->windowId()) {
                    const auto guard = owner;
                    const QPointer<AbstractWindowContext> context(ctx);
                    setupNecessaryAttributes();
                    if (guard && context)
                        updateGeometry();
                }
            }
            return false;
        }

    protected:
        AbstractWindowContext *ctx;
        QPointer<QObject> owner;
    };

}

#endif // WINDOWS10BORDERHANDLER_P_H
