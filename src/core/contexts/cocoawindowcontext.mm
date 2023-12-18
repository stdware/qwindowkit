#include "cocoawindowcontext_p.h"

#include <objc/runtime.h>
#include <AppKit/AppKit.h>

#include <QtGui/QGuiApplication>

namespace QWK {

    struct NSWindowProxy {
        NSWindowProxy(NSWindow *macWindow) {
            if (instances.contains(macWindow)) {
                return;
            }
            nswindow = macWindow;
            instances.insert(macWindow, this);
            if (!windowClass) {
                windowClass = [nswindow class];
                replaceImplementations();
            }
        }

        ~NSWindowProxy() {
            instances.remove(nswindow);
            if (instances.count() <= 0) {
                restoreImplementations();
                windowClass = nil;
            }
            nswindow = nil;
        }

        void replaceImplementations() {
            Method method = class_getInstanceMethod(windowClass, @selector(setStyleMask:));
            oldSetStyleMask = reinterpret_cast<setStyleMaskPtr>(
                method_setImplementation(method, reinterpret_cast<IMP>(setStyleMask)));

            method =
                class_getInstanceMethod(windowClass, @selector(setTitlebarAppearsTransparent:));
            oldSetTitlebarAppearsTransparent =
                reinterpret_cast<setTitlebarAppearsTransparentPtr>(method_setImplementation(
                    method, reinterpret_cast<IMP>(setTitlebarAppearsTransparent)));

#if 0
            method = class_getInstanceMethod(windowClass, @selector(canBecomeKeyWindow));
            oldCanBecomeKeyWindow = reinterpret_cast<canBecomeKeyWindowPtr>(method_setImplementation(method, reinterpret_cast<IMP>(canBecomeKeyWindow)));

            method = class_getInstanceMethod(windowClass, @selector(canBecomeMainWindow));
            oldCanBecomeMainWindow = reinterpret_cast<canBecomeMainWindowPtr>(method_setImplementation(method, reinterpret_cast<IMP>(canBecomeMainWindow)));
#endif

            method = class_getInstanceMethod(windowClass, @selector(sendEvent:));
            oldSendEvent = reinterpret_cast<sendEventPtr>(
                method_setImplementation(method, reinterpret_cast<IMP>(sendEvent)));
        }

        void restoreImplementations() {
            Method method = class_getInstanceMethod(windowClass, @selector(setStyleMask:));
            method_setImplementation(method, reinterpret_cast<IMP>(oldSetStyleMask));
            oldSetStyleMask = nil;

            method =
                class_getInstanceMethod(windowClass, @selector(setTitlebarAppearsTransparent:));
            method_setImplementation(method,
                                     reinterpret_cast<IMP>(oldSetTitlebarAppearsTransparent));
            oldSetTitlebarAppearsTransparent = nil;

#if 0
            method = class_getInstanceMethod(windowClass, @selector(canBecomeKeyWindow));
            method_setImplementation(method, reinterpret_cast<IMP>(oldCanBecomeKeyWindow));
            oldCanBecomeKeyWindow = nil;

            method = class_getInstanceMethod(windowClass, @selector(canBecomeMainWindow));
            method_setImplementation(method, reinterpret_cast<IMP>(oldCanBecomeMainWindow));
            oldCanBecomeMainWindow = nil;
#endif

            method = class_getInstanceMethod(windowClass, @selector(sendEvent:));
            method_setImplementation(method, reinterpret_cast<IMP>(oldSendEvent));
            oldSendEvent = nil;
        }

        void setSystemTitleBarVisible(const bool visible) {
            NSView *nsview = [nswindow contentView];
            if (!nsview) {
                return;
            }
            nsview.wantsLayer = YES;
            nswindow.styleMask |= NSWindowStyleMaskResizable;
            if (visible) {
                nswindow.styleMask &= ~NSWindowStyleMaskFullSizeContentView;
            } else {
                nswindow.styleMask |= NSWindowStyleMaskFullSizeContentView;
            }
            nswindow.titlebarAppearsTransparent = (visible ? NO : YES);
            nswindow.titleVisibility = (visible ? NSWindowTitleVisible : NSWindowTitleHidden);
            nswindow.hasShadow = YES;
            nswindow.showsToolbarButton = NO;
            nswindow.movableByWindowBackground = NO;
            nswindow.movable = NO; // This line causes the window in the wrong position when
                                   // become fullscreen.
            //  For some unknown reason, we don't need the following hack in Qt versions below or
            //  equal to 6.2.4.
#if (QT_VERSION > QT_VERSION_CHECK(6, 2, 4))
            [nswindow standardWindowButton:NSWindowCloseButton].hidden = (visible ? NO : YES);
            [nswindow standardWindowButton:NSWindowMiniaturizeButton].hidden = (visible ? NO : YES);
            [nswindow standardWindowButton:NSWindowZoomButton].hidden = (visible ? NO : YES);
#endif
        }

    private:
        static BOOL canBecomeKeyWindow(id obj, SEL sel) {
            if (instances.contains(reinterpret_cast<NSWindow *>(obj))) {
                return YES;
            }

            if (oldCanBecomeKeyWindow) {
                return oldCanBecomeKeyWindow(obj, sel);
            }

            return YES;
        }

        static BOOL canBecomeMainWindow(id obj, SEL sel) {
            if (instances.contains(reinterpret_cast<NSWindow *>(obj))) {
                return YES;
            }

            if (oldCanBecomeMainWindow) {
                return oldCanBecomeMainWindow(obj, sel);
            }

            return YES;
        }

        static void setStyleMask(id obj, SEL sel, NSWindowStyleMask styleMask) {
            if (instances.contains(reinterpret_cast<NSWindow *>(obj))) {
                styleMask |= NSWindowStyleMaskFullSizeContentView;
            }

            if (oldSetStyleMask) {
                oldSetStyleMask(obj, sel, styleMask);
            }
        }

        static void setTitlebarAppearsTransparent(id obj, SEL sel, BOOL transparent) {
            if (instances.contains(reinterpret_cast<NSWindow *>(obj))) {
                transparent = YES;
            }

            if (oldSetTitlebarAppearsTransparent) {
                oldSetTitlebarAppearsTransparent(obj, sel, transparent);
            }
        }

        static void sendEvent(id obj, SEL sel, NSEvent *event) {
            if (oldSendEvent) {
                oldSendEvent(obj, sel, event);
            }

#if 0
            const auto nswindow = reinterpret_cast<NSWindow *>(obj);
            const auto it = instances.find(nswindow);
            if (it == instances.end()) {
                return;
            }

            NSWindowProxy *proxy = it.value();
            if (event.type == NSEventTypeLeftMouseDown) {
                proxy->lastMouseDownEvent = event;
                QCoreApplication::processEvents();
                proxy->lastMouseDownEvent = nil;
            }
#endif
        }

    private:
        Q_DISABLE_COPY(NSWindowProxy)

        NSWindow *nswindow = nil;
        // NSEvent *lastMouseDownEvent = nil;

        static inline QHash<NSWindow *, NSWindowProxy *> instances = {};

        static inline Class windowClass = nil;

        using setStyleMaskPtr = void (*)(id, SEL, NSWindowStyleMask);
        static inline setStyleMaskPtr oldSetStyleMask = nil;

        using setTitlebarAppearsTransparentPtr = void (*)(id, SEL, BOOL);
        static inline setTitlebarAppearsTransparentPtr oldSetTitlebarAppearsTransparent = nil;

        using canBecomeKeyWindowPtr = BOOL (*)(id, SEL);
        static inline canBecomeKeyWindowPtr oldCanBecomeKeyWindow = nil;

        using canBecomeMainWindowPtr = BOOL (*)(id, SEL);
        static inline canBecomeMainWindowPtr oldCanBecomeMainWindow = nil;

        using sendEventPtr = void (*)(id, SEL, NSEvent *);
        static inline sendEventPtr oldSendEvent = nil;
    };

    using ProxyList = QHash<WId, NSWindowProxy *>;
    Q_GLOBAL_STATIC(ProxyList, g_proxyList);

    static inline NSWindow *mac_getNSWindow(const WId windowId) {
        const auto nsview = reinterpret_cast<NSView *>(windowId);
        return [nsview window];
    }

    static inline NSWindowProxy *ensureWindowProxy(const WId windowId) {
        auto it = g_proxyList()->find(windowId);
        if (it == g_proxyList()->end()) {
            NSWindow *nswindow = mac_getNSWindow(windowId);
            const auto proxy = new NSWindowProxy(nswindow);
            it = g_proxyList()->insert(windowId, proxy);
        }
        return it.value();
    }

    static inline void releaseWindowProxy(const WId windowId) {
        if (const auto proxy = g_proxyList()->take(windowId)) {
            delete proxy;
        }
    }

    class CocoaWindowEventFilter : public QObject {
    public:
        explicit CocoaWindowEventFilter(AbstractWindowContext *context, QObject *parent = nullptr);
        ~CocoaWindowEventFilter() override;

        enum WindowStatus {
            Idle,
            WaitingRelease,
            PreparingMove,
            Moving,
        };

    protected:
        bool eventFilter(QObject *object, QEvent *event) override;

    private:
        AbstractWindowContext *m_context;
        bool m_cursorShapeChanged;
        WindowStatus m_windowStatus;
    };

    CocoaWindowEventFilter::CocoaWindowEventFilter(AbstractWindowContext *context, QObject *parent)
        : QObject(parent), m_context(context), m_cursorShapeChanged(false), m_windowStatus(Idle) {
        m_context->window()->installEventFilter(this);
    }

    CocoaWindowEventFilter::~CocoaWindowEventFilter() = default;

    bool CocoaWindowEventFilter::eventFilter(QObject *obj, QEvent *event) {
        Q_UNUSED(obj)
        auto type = event->type();
        if (type < QEvent::MouseButtonPress || type > QEvent::MouseMove) {
            return false;
        }
        QObject *host = m_context->host();
        QWindow *window = m_context->window();
        WindowItemDelegate *delegate = m_context->delegate();
        auto me = static_cast<const QMouseEvent *>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QPoint scenePos = me->scenePosition().toPoint();
        QPoint globalPos = me->globalPosition().toPoint();
#else
        QPoint scenePos = me->windowPos().toPoint();
        QPoint globalPos = me->screenPos().toPoint();
#endif
        bool inTitleBar = m_context->isInTitleBarDraggableArea(scenePos);
        switch (type) {
            case QEvent::MouseButtonPress: {
                switch (me->button()) {
                    case Qt::LeftButton: {
                        if (inTitleBar) {
                            // If we call startSystemMove() now but release the mouse without actual
                            // movement, there will be no MouseReleaseEvent, so we defer it when the
                            // mouse is actually moving for the first time
                            m_windowStatus = PreparingMove;
                            event->accept();
                            return true;
                        }
                        m_windowStatus = WaitingRelease;
                        break;
                    }
                    case Qt::RightButton: {
                        m_context->showSystemMenu(globalPos);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }

            case QEvent::MouseButtonRelease: {
                switch (m_windowStatus) {
                    case PreparingMove:
                    case Moving: {
                        m_windowStatus = Idle;
                        event->accept();
                        return true;
                    }
                    case WaitingRelease: {
                        m_windowStatus = Idle;
                        break;
                    }
                    default: {
                        if (inTitleBar) {
                            event->accept();
                            return true;
                        }
                        break;
                    }
                }
                break;
            }

            case QEvent::MouseMove: {
                switch (m_windowStatus) {
                    case Moving: {
                        return true;
                    }
                    case PreparingMove: {
                        m_windowStatus = Moving;
                        window->startSystemMove();
                        event->accept();
                        return true;
                    }
                    default:
                        break;
                }
                break;
            }

            case QEvent::MouseButtonDblClick: {
                if (me->button() == Qt::LeftButton && inTitleBar &&
                    !delegate->isHostSizeFixed(host)) {
                    Qt::WindowStates windowState = delegate->getWindowState(host);
                    if (!(windowState & Qt::WindowFullScreen)) {
                        if (windowState & Qt::WindowMaximized) {
                            delegate->setWindowState(host, windowState & ~Qt::WindowMaximized);
                        } else {
                            delegate->setWindowState(host, windowState | Qt::WindowMaximized);
                        }
                        event->accept();
                        return true;
                    }
                }
                break;
            }

            default:
                break;
        }
        return false;
    }

    CocoaWindowContext::CocoaWindowContext() : AbstractWindowContext() {
    }

    CocoaWindowContext::~CocoaWindowContext() {
        releaseWindowProxy(windowId);
    }

    QString CocoaWindowContext::key() const {
        return QStringLiteral("cocoa");
    }

    void CocoaWindowContext::virtual_hook(int id, void *data) {
        switch (id) {
            case ShowSystemMenuHook:
                // TODO: mac system menu
                return;
            case SystemButtonAreaChangedHook:
                // TODO: mac system button rect updated
                return;
            default:
                break;
        }
        AbstractWindowContext::virtual_hook(id, data);
    }

    void CocoaWindowContext::winIdChanged(QWindow *oldWindow) {
        if (windowId) {
            releaseWindowProxy(windowId);
        }
        windowId = m_windowHandle->winId();
        ensureWindowProxy(windowId)->setSystemTitleBarVisible(false);
        cocoaWindowEventFilter = std::make_unique<CocoaWindowEventFilter>(this, this);
    }

}
