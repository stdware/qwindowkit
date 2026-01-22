// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2025-2027 Wing-summer (wingsummer)
// SPDX-License-Identifier: Apache-2.0

#include "qwindowkit_linux.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QGuiApplication>
#include <QLibrary>

namespace QWK {
    namespace Private {

        bool isX11Platform() {
            static const bool isX11 = QGuiApplication::platformName().startsWith(
                QStringLiteral("xcb"), Qt::CaseInsensitive);
            return isX11;
        }

        bool isWaylandPlatform() {
            static const bool isWayland = QGuiApplication::platformName().startsWith(
                QStringLiteral("wayland"), Qt::CaseInsensitive);
            return isWayland;
        }

        const LinuxWaylandAPI &waylandAPI() {
            static LinuxWaylandAPI api;
            static bool guard = true;
            if (guard && isWaylandPlatform()) {
                QLibrary waylib(QStringLiteral("libwayland-client.so"));
                bool loaded = false;
                if (waylib.load()) {
                    loaded = true;
                } else {
                    waylib.setFileName(QStringLiteral("libwayland-client.so.0"));
                    if (waylib.load()) {
                        loaded = true;
                    }
                }

                if (loaded) {
                    api.wl_display_flush = reinterpret_cast<LinuxWaylandAPI::wl_display_flush_fn>(
                        waylib.resolve("wl_display_flush"));
                    api.wl_proxy_marshal_flags =
                        reinterpret_cast<LinuxWaylandAPI::wl_proxy_marshal_flags_fn>(
                            waylib.resolve("wl_proxy_marshal_flags"));
                    api.wl_proxy_get_version =
                        reinterpret_cast<LinuxWaylandAPI::wl_proxy_get_version_fn>(
                            waylib.resolve("wl_proxy_get_version"));
                }
            }
            guard = false;
            return api;
        }

        const LinuxX11API &x11API() {
            static LinuxX11API api;
            static bool guard = true;
            if (guard && isX11Platform()) {
                QString libName = QStringLiteral(
#if defined(__CYGWIN__)
                    "libX11-6.so"
#elif defined(__OpenBSD__) || defined(__NetBSD__)
                    "libX11.so"
#else
                    "libX11.so.6"
#endif
                );
                QLibrary x11lib(libName);
                if (x11lib.load()) {
                    api.XInternAtom =
                        reinterpret_cast<LinuxX11API::XInternAtomFn>(x11lib.resolve("XInternAtom"));
                    api.XSendEvent =
                        reinterpret_cast<LinuxX11API::XSendEventFn>(x11lib.resolve("XSendEvent"));
                    api.XFlush = reinterpret_cast<LinuxX11API::XFlushFn>(x11lib.resolve("XFlush"));
                    api.XUngrabPointer = reinterpret_cast<LinuxX11API::XUngrabPointerFn>(
                        x11lib.resolve("XUngrabPointer"));
                }
            }
            guard = false;
            return api;
        }

    }
}
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
