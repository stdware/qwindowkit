// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2025-2027 Wing-summer (wingsummer)
// SPDX-License-Identifier: Apache-2.0

#ifndef QWINDOWKIT_X11_H
#define QWINDOWKIT_X11_H

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

namespace QWK {
    namespace Private {
        struct X11API {
            X11API() = default;
            Q_DISABLE_COPY(X11API)

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

        bool isX11Platform();

        const X11API &x11API();
    }
}
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#endif // QWINDOWKIT_X11_H
