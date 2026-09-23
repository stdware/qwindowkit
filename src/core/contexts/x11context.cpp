// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2025-2027 Wing-summer (wingsummer)
// SPDX-License-Identifier: Apache-2.0


#include "x11context_p.h"

#include "qwindowkit_x11.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
// Xlib client-message layout; Display remains opaque.
typedef struct {
    int type;
    unsigned long serial; /* # of last request processed by server */
    Bool send_event;      /* true if this came from a SendEvent request */
    Display *display;     /* Display the event was read from */
    Window window;
    Atom message_type;
    int format;
    union {
        char b[20];
        short s[10];
        long l[5];
    } data;
} XClientMessageEvent;

/*
 * this union is defined so Xlib can always use the same sized
 * event structure internally, to avoid memory fragmentation.
 */
union _XEvent {
    // delete so many members but size unchanged because of pad
    XClientMessageEvent xclient;
    long pad[24];
};

namespace QWK {

    X11Context::X11Context() = default;

    X11Context::~X11Context() = default;

    QString X11Context::key() const {
        return QStringLiteral("xcb");
    }

    void X11Context::virtual_hook(int id, void *data) {
        if (id == ShowSystemMenuHook) {
            // showSystemMenu() is public API and may be called before the window is created or
            // after it has been destroyed.
            if (!m_windowId || !m_windowHandle) {
                return;
            }

            auto *x11app = qApp->nativeInterface<QNativeInterface::QX11Application>();
            if (!x11app) {
                return;
            }

            auto display = x11app->display();
            if (!display) {
                return;
            }

            const auto &api = QWK::Private::x11API();
            Q_ASSERT(api.isValid());

            // some marcos to constexpr in X11
            constexpr auto None = 0L;
            constexpr auto ClientMessage = 33;
            constexpr auto False = 0;
            constexpr auto Button3 = 3;
            constexpr auto SubstructureNotifyMask = 1L << 19;
            constexpr auto SubstructureRedirectMask = 1L << 20;

            // use window id (XID)
            auto xwin = static_cast<Window>(m_windowId);
            Atom atom = api.XInternAtom(display, "_GTK_SHOW_WINDOW_MENU", False);
            if (atom == None)
                return; // WM might not support this atom
            auto pos = static_cast<const QPoint *>(data);
            XEvent ev{};
            ev.xclient.type = ClientMessage;
            ev.xclient.window = xwin;
            ev.xclient.message_type = atom;

            // The format member is set to 8, 16, or 32
            // and specifies whether the data should be viewed as
            // a list of bytes, shorts, or longs - typeof(xclient.data).
            ev.xclient.format = 32;

            qreal dpr = m_windowHandle->devicePixelRatio();
            int root_x = qRound(pos->x() * dpr);
            int root_y = qRound(pos->y() * dpr);

            ev.xclient.data.l[0] = Button3; // right button
            ev.xclient.data.l[1] = root_x;
            ev.xclient.data.l[2] = root_y;

            Window root = api.XDefaultRootWindow(display);
            api.XUngrabPointer(display, 0L);
            api.XSendEvent(display, root, False, SubstructureRedirectMask | SubstructureNotifyMask,
                           &ev);
            api.XFlush(display);
        } else {
            AbstractWindowContext::virtual_hook(id, data);
        }
    }
}
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
