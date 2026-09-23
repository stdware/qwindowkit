// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2025-2027 Wing-summer (wingsummer)
// SPDX-License-Identifier: Apache-2.0

#include "waylandcontext_p.h"

#ifdef QWK_HAS_WAYLAND_CONTEXT
#include "qwindowkit_wayland.h"
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/qpa/qplatformnativeinterface.h>

namespace QWK {
    static inline void xdg_toplevel_show_window_menu(struct xdg_toplevel *xdg_toplevel,
                                                     struct wl_seat *seat, uint32_t serial,
                                                     int32_t x, int32_t y) {
        constexpr auto XDG_TOPLEVEL_SHOW_WINDOW_MENU = 4;
        const auto &api = QWK::Private::waylandAPI();
        Q_ASSERT(api.isValid());
        api.wl_proxy_marshal_flags(
            reinterpret_cast<struct wl_proxy *>(xdg_toplevel), XDG_TOPLEVEL_SHOW_WINDOW_MENU,
            nullptr, api.wl_proxy_get_version(reinterpret_cast<struct wl_proxy *>(xdg_toplevel)), 0,
            seat, serial, x, y);
    }

    WaylandContext::WaylandContext() = default;

    WaylandContext::~WaylandContext() = default;

    QString WaylandContext::key() const {
        return QStringLiteral("wayland");
    }

    void WaylandContext::showSystemMenu(const QPoint &globalPos) {
        if (!m_windowId || !m_windowHandle) {
            return;
        }
        auto *waylandApp = qApp->nativeInterface<QNativeInterface::QWaylandApplication>();
        if (!waylandApp) {
            return;
        }
        uint serial = waylandApp->lastInputSerial();
        wl_seat *seat = waylandApp->lastInputSeat();
        if (serial == 0 || !seat) {
            return;
        }

        auto toplevel = static_cast<xdg_toplevel *>(
            QGuiApplication::platformNativeInterface()->nativeResourceForWindow(
                "xdg_toplevel", m_windowHandle));
        if (!toplevel) {
            return;
        }
        const QPointF localPos = m_windowHandle->mapFromGlobal(QPointF(globalPos));
        // QtWindowContext uses FramelessWindowHint, so the content and surface origins
        // coincide. Undo Qt's coordinate scaling, without applying Wayland's buffer scale.
        const QPoint surfacePos =
            QHighDpi::toNativeLocalPosition(localPos, m_windowHandle.data()).toPoint();
        xdg_toplevel_show_window_menu(toplevel, seat, serial, surfacePos.x(), surfacePos.y());

        wl_display *d = waylandApp->display();
        if (d) {
            const auto &api = QWK::Private::waylandAPI();
            Q_ASSERT(api.isValid());
            api.wl_display_flush(d);
        }
    }
}
#endif // QWK_HAS_WAYLAND_CONTEXT
