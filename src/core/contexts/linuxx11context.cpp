// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2025-2027 Wing-summer (wingsummer)
// SPDX-License-Identifier: Apache-2.0


#include "linuxx11context_p.h"

#include "qwindowkit_linux.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
// copy from X11 library and simplify it
typedef struct _XExtData XExtData;
struct ScreenFormat;
struct Depth;
struct Visual;
typedef XID Colormap;
typedef struct {
    XExtData *ext_data;        /* hook for extension to hang data */
    struct _XDisplay *display; /* back pointer to display structure */
    Window root;               /* Root window id. */
    int width, height;         /* width and height of screen */
    int mwidth, mheight;       /* width and height of  in millimeters */
    int ndepths;               /* number of depths possible */
    Depth *depths;             /* list of allowable depths on the screen */
    int root_depth;            /* bits per pixel */
    Visual *root_visual;       /* root visual */

    // typedef struct _XGC
    // #ifdef XLIB_ILLEGAL_ACCESS
    //     {
    //         XExtData *ext_data;	/* hook for extension to hang data */
    //         GContext gid;	/* protocol ID for graphics context */
    //         /* there is more to this structure, but it is private to Xlib */
    //     }
    // #endif
    // *GC;
    void * /*GC*/ default_gc; /* GC for the root root visual */

    Colormap cmap; /* default color map */
    unsigned long white_pixel;
    unsigned long black_pixel; /* White and Black pixel values */
    int max_maps, min_maps;    /* max and min color maps */
    int backing_store;         /* Never, WhenMapped, Always */
    Bool save_unders;
    long root_input_mask; /* initial root input mask */
} Screen;
typedef char *XPointer;
typedef struct {
    XExtData *ext_data; /* hook for extension to hang data */
    struct _XPrivate *private1;
    int fd; /* Network socket. */
    int private2;
    int proto_major_version; /* major version of server's X protocol */
    int proto_minor_version; /* minor version of servers X protocol */
    char *vendor;            /* vendor of the server hardware */
    XID private3;
    XID private4;
    XID private5;
    int private6;
    XID (*resource_alloc)(/* allocator function */
                          struct _XDisplay *);
    int byte_order;              /* screen byte order, LSBFirst, MSBFirst */
    int bitmap_unit;             /* padding and data requirements */
    int bitmap_pad;              /* padding requirements on bitmaps */
    int bitmap_bit_order;        /* LeastSignificant or MostSignificant */
    int nformats;                /* number of pixmap formats in list */
    ScreenFormat *pixmap_format; /* pixmap format list */
    int private8;
    int release; /* release of the server */
    struct _XPrivate *private9, *private10;
    int qlen;                        /* Length of input event queue */
    unsigned long last_request_read; /* seq number of last event read */
    unsigned long request;           /* sequence number of last request. */
    XPointer private11;
    XPointer private12;
    XPointer private13;
    XPointer private14;
    unsigned max_request_size; /* maximum number 32 bit words in request*/
    struct _XrmHashBucketRec *db;
    int (*private15)(struct _XDisplay *);
    char *display_name;          /* "host:display" string used on this connect*/
    int default_screen;          /* default screen for operations */
    int nscreens;                /* number of screens on this server*/
    Screen *screens;             /* pointer to list of screens */
    unsigned long motion_buffer; /* size of motion buffer */
    unsigned long private16;
    int min_keycode; /* minimum defined keycode */
    int max_keycode; /* maximum defined keycode */
    XPointer private17;
    XPointer private18;
    int private19;
    char *xdefaults; /* contents of defaults from server */
    /* there is more to this structure, but it is private to Xlib */
} *_XPrivDisplay;

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

#define ScreenOfDisplay(dpy, scr) (&((_XPrivDisplay) (dpy))->screens[scr])
#define DefaultScreen(dpy)        (((_XPrivDisplay) (dpy))->default_screen)
#define DefaultRootWindow(dpy)    (ScreenOfDisplay(dpy, DefaultScreen(dpy))->root)

namespace QWK {

    LinuxX11Context::LinuxX11Context() : QtWindowContext() {
    }

    LinuxX11Context::~LinuxX11Context() = default;

    QString LinuxX11Context::key() const {
        return QStringLiteral("xcb");
    }

    void LinuxX11Context::virtual_hook(int id, void *data) {
        if (id == ShowSystemMenuHook) {
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

            Window root = DefaultRootWindow(display);
            api.XUngrabPointer(display, 0L);
            api.XSendEvent(display, root, False, SubstructureRedirectMask | SubstructureNotifyMask,
                           &ev);
            api.XFlush(display);
        } else {
            QtWindowContext::virtual_hook(id, data);
        }
    }
}
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
