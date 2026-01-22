// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2025-2027 Wing-summer (wingsummer)
// SPDX-License-Identifier: Apache-2.0

#ifndef QWINDOWKIT_LINUX_H
#define QWINDOWKIT_LINUX_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QtCore/qglobal.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <qguiapplication_platform.h>

// some declarations about x11
using Atom = unsigned long;
using Bool = int;
using XID = unsigned long;
using Window = XID;

union _XEvent;
using XEvent = union _XEvent;

// for wayland
struct wl_proxy;

namespace QWK {
    namespace Private {
        struct LinuxX11API {
            LinuxX11API() = default;
            Q_DISABLE_COPY(LinuxX11API)

            using XInternAtomFn = Atom (*)(Display *, const char *, Bool);
            using XSendEventFn = int (*)(Display *, Window, Bool, long, XEvent *);
            using XFlushFn = int (*)(Display *);
            using XUngrabPointerFn = int (*)(Display *, unsigned long);

            XInternAtomFn XInternAtom = nullptr;
            XSendEventFn XSendEvent = nullptr;
            XFlushFn XFlush = nullptr;
            XUngrabPointerFn XUngrabPointer = nullptr;

            inline bool isValid() const {
                return XInternAtom && XSendEvent && XFlush && XUngrabPointer;
            }
        };

        struct LinuxWaylandAPI {
            LinuxWaylandAPI() = default;
            Q_DISABLE_COPY(LinuxWaylandAPI)

            using wl_display_flush_fn = int (*)(struct wl_display *);
            using wl_proxy_marshal_flags_fn = void (*)(struct wl_proxy *, uint32_t,
                                                       const struct wl_interface *, uint32_t,
                                                       uint32_t, ...);
            using wl_proxy_get_version_fn = int (*)(struct wl_proxy *);

            wl_display_flush_fn wl_display_flush = nullptr;
            wl_proxy_marshal_flags_fn wl_proxy_marshal_flags = nullptr;
            wl_proxy_get_version_fn wl_proxy_get_version = nullptr;

            inline bool isValid() const {
                return wl_display_flush && wl_proxy_marshal_flags && wl_proxy_get_version;
            }
        };


        bool isWaylandPlatform();

        bool isX11Platform();

        const LinuxX11API &x11API();

        const LinuxWaylandAPI &waylandAPI();
    }
}
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#endif // QWINDOWKIT_LINUX_H
