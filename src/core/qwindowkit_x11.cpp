// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2025-2027 Wing-summer (wingsummer)
// SPDX-License-Identifier: Apache-2.0

#include "qwindowkit_x11.h"

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

        const X11API &x11API() {
            static X11API api;
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
                        reinterpret_cast<X11API::XInternAtomFn>(x11lib.resolve("XInternAtom"));
                    api.XSendEvent =
                        reinterpret_cast<X11API::XSendEventFn>(x11lib.resolve("XSendEvent"));
                    api.XFlush = reinterpret_cast<X11API::XFlushFn>(x11lib.resolve("XFlush"));
                    api.XUngrabPointer = reinterpret_cast<X11API::XUngrabPointerFn>(
                        x11lib.resolve("XUngrabPointer"));
                    api.XDefaultRootWindow = reinterpret_cast<X11API::XDefaultRootWindowFn>(
                        x11lib.resolve("XDefaultRootWindow"));
                }
            }
            guard = false;
            return api;
        }

    }
}
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
