// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2025-2027 Wing-summer (wingsummer)
// SPDX-License-Identifier: Apache-2.0

#include "qwindowkit_wayland.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QGuiApplication>
#include <QLibrary>

namespace QWK {
    namespace Private {

        bool isWaylandPlatform() {
            static const bool isWayland = QGuiApplication::platformName().startsWith(
                QStringLiteral("wayland"), Qt::CaseInsensitive);
            return isWayland;
        }

        const WaylandAPI &waylandAPI() {
            static WaylandAPI api;
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
                    api.wl_display_flush = reinterpret_cast<WaylandAPI::wl_display_flush_fn>(
                        waylib.resolve("wl_display_flush"));
                    api.wl_proxy_marshal_flags =
                        reinterpret_cast<WaylandAPI::wl_proxy_marshal_flags_fn>(
                            waylib.resolve("wl_proxy_marshal_flags"));
                    api.wl_proxy_get_version =
                        reinterpret_cast<WaylandAPI::wl_proxy_get_version_fn>(
                            waylib.resolve("wl_proxy_get_version"));
                }
            }
            guard = false;
            return api;
        }

    }
}
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
