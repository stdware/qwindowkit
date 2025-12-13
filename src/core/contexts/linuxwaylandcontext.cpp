// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2025-2027 Wing-summer (wingsummer)
// SPDX-License-Identifier: Apache-2.0

#include "linuxwaylandcontext_p.h"

#include "qwindowkit_linux.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
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

    LinuxWaylandContext::LinuxWaylandContext() : QtWindowContext() {
    }

    LinuxWaylandContext::~LinuxWaylandContext() = default;

    QString LinuxWaylandContext::key() const {
        return QStringLiteral("wayland");
    }

    void LinuxWaylandContext::virtual_hook(int id, void *data) {
        if (id == ShowSystemMenuHook) {
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
            auto pos = static_cast<const QPoint *>(data);
            xdg_toplevel_show_window_menu(toplevel, seat, serial, pos->x(), pos->y());

            wl_display *d = waylandApp->display();
            if (d) {
                const auto &api = QWK::Private::waylandAPI();
                Q_ASSERT(api.isValid());
                api.wl_display_flush(d);
            }
        } else {
            QtWindowContext::virtual_hook(id, data);
        }
    }
}
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
