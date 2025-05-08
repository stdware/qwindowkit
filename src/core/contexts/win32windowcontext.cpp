// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "win32windowcontext_p.h"

#include <optional>

#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>
#include <QtGui/QPalette>

#include <QtGui/qpa/qwindowsysteminterface.h>

#include <QtGui/private/qhighdpiscaling_p.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  include <QtGui/private/qguiapplication_p.h>
#endif
#include <QtGui/qpa/qplatformwindow.h>
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
#  include <QtGui/qpa/qplatformnativeinterface.h>
#else
#  include <QtGui/qpa/qplatformwindow_p.h>
#endif

#include <QWKCore/qwkconfig.h>

#include "qwkglobal_p.h"
#include "qwkwindowsextra_p.h"

#if (QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)) && (QT_VERSION <= QT_VERSION_CHECK(6, 6, 1))
#  error Current Qt version has a critical bug which will break QWK functionality. Please upgrade to > 6.6.1 or downgrade to < 6.6.0
#endif

#ifndef DWM_BB_ENABLE
#  define DWM_BB_ENABLE 0x00000001
#endif

#ifndef ABM_GETAUTOHIDEBAREX
#  define ABM_GETAUTOHIDEBAREX 0x0000000b
#endif

namespace QWK {

    enum IconButtonClickLevelFlag {
        IconButtonClicked = 1,
        IconButtonDoubleClicked = 2,
        IconButtonTriggersClose = 4,
    };

    // The thickness of an auto-hide taskbar in pixels.
    static constexpr const quint8 kAutoHideTaskBarThickness = 2;

    QWK_USED static constexpr const struct {
        const uint32_t activeLight = MAKE_RGBA_COLOR(110, 110, 110, 255);   // #6E6E6E
        const uint32_t activeDark = MAKE_RGBA_COLOR(51, 51, 51, 255);       // #333333
        const uint32_t inactiveLight = MAKE_RGBA_COLOR(167, 167, 167, 255); // #A7A7A7
        const uint32_t inactiveDark = MAKE_RGBA_COLOR(61, 61, 62, 255);     // #3D3D3E
    } kWindowsColorSet;

    // hWnd -> context
    using WndProcHash = QHash<HWND, Win32WindowContext *>;
    Q_GLOBAL_STATIC(WndProcHash, g_wndProcHash)

    // Original Qt window proc function
    static WNDPROC g_qtWindowProc = nullptr;

    static inline bool
#if !QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)
        constexpr
#endif

        isSystemBorderEnabled() {
        return
#if QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)
            isWin10OrGreater()
#else
            false
#endif
                ;
    }

    static inline void triggerFrameChange(HWND hwnd) {
        ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                       SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER |
                           SWP_FRAMECHANGED);
    }

    static void setInternalWindowFrameMargins(QWindow *window, const QMargins &margins) {
        const QVariant marginsVar = QVariant::fromValue(margins);

        // We need to tell Qt we have set a custom margin, because we are hiding
        // the title bar by pretending the whole window is filled by client area,
        // this however confuses Qt's internal logic. We need to do the following
        // hack to let Qt consider the extra margin when changing window geometry.
        window->setProperty("_q_windowsCustomMargins", marginsVar);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (QPlatformWindow *platformWindow = window->handle()) {
            if (const auto ni = QGuiApplication::platformNativeInterface()) {
                ni->setWindowProperty(platformWindow, QStringLiteral("WindowsCustomMargins"),
                                      marginsVar);
            }
        }
#else
        if (const auto platformWindow =
                dynamic_cast<QNativeInterface::Private::QWindowsWindow *>(window->handle())) {
            platformWindow->setCustomMargins(margins);
        }
#endif
    }

    static inline MONITORINFOEXW getMonitorForWindow(HWND hwnd) {
        // Use "MONITOR_DEFAULTTONEAREST" here so that we can still get the correct
        // monitor even if the window is minimized.
        HMONITOR monitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEXW monitorInfo{};
        monitorInfo.cbSize = sizeof(monitorInfo);
        ::GetMonitorInfoW(monitor, &monitorInfo);
        return monitorInfo;
    }

    static inline void moveWindowToMonitor(HWND hwnd, const MONITORINFOEXW &activeMonitor) {
        RECT currentMonitorRect = getMonitorForWindow(hwnd).rcMonitor;
        RECT activeMonitorRect = activeMonitor.rcMonitor;
        // We are in the same monitor, nothing to adjust here.
        if (currentMonitorRect == activeMonitorRect) {
            return;
        }
        RECT currentWindowRect{};
        ::GetWindowRect(hwnd, &currentWindowRect);
        auto newWindowX =
            activeMonitorRect.left + (currentWindowRect.left - currentMonitorRect.left);
        auto newWindowY = activeMonitorRect.top + (currentWindowRect.top - currentMonitorRect.top);
        ::SetWindowPos(hwnd, nullptr, newWindowX, newWindowY, RECT_WIDTH(currentWindowRect),
                       RECT_HEIGHT(currentWindowRect),
                       SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }

    static inline bool isFullScreen(HWND hwnd) {
        RECT windowRect{};
        ::GetWindowRect(hwnd, &windowRect);
        // Compare to the full area of the screen, not the work area.
        return (windowRect == getMonitorForWindow(hwnd).rcMonitor);
    }

    static inline bool isMaximized(HWND hwnd) {
        return ::IsZoomed(hwnd);
    }

    static inline bool isMinimized(HWND hwnd) {
        return ::IsIconic(hwnd);
    }

    static inline bool isWindowNoState(HWND hwnd) {
#if 0
        WINDOWPLACEMENT wp{};
        wp.length = sizeof(wp);
        ::GetWindowPlacement(hwnd, &wp);
        return ((wp.showCmd == SW_NORMAL) || (wp.showCmd == SW_RESTORE));
#else
        if (isFullScreen(hwnd)) {
            return false;
        }
        const auto style = static_cast<DWORD>(::GetWindowLongPtrW(hwnd, GWL_STYLE));
        return (!(style & (WS_MINIMIZE | WS_MAXIMIZE)));
#endif
    }

    static inline void bringWindowToFront(HWND hwnd) {
        HWND oldForegroundWindow = ::GetForegroundWindow();
        if (!oldForegroundWindow) {
            // The foreground window can be NULL, it's not an API error.
            return;
        }
        MONITORINFOEXW activeMonitor = getMonitorForWindow(oldForegroundWindow);
        // We need to show the window first, otherwise we won't be able to bring it to front.
        if (!::IsWindowVisible(hwnd)) {
            ::ShowWindow(hwnd, SW_SHOW);
        }
        if (isMinimized(hwnd)) {
            // Restore the window if it is minimized.
            ::ShowWindow(hwnd, SW_RESTORE);
            // Once we've been restored, throw us on the active monitor.
            moveWindowToMonitor(hwnd, activeMonitor);
            // When the window is restored, it will always become the foreground window.
            // So return early here, we don't need the following code to bring it to front.
            return;
        }
        // OK, our window is not minimized, so now we will try to bring it to front manually.
        // First try to send a message to the current foreground window to check whether
        // it is currently hanging or not.
        if (!::SendMessageTimeoutW(oldForegroundWindow, WM_NULL, 0, 0,
                                   SMTO_BLOCK | SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 1000,
                                   nullptr)) {
            // The foreground window hangs, can't activate current window.
            return;
        }
        DWORD windowThreadProcessId = ::GetWindowThreadProcessId(oldForegroundWindow, nullptr);
        DWORD currentThreadId = ::GetCurrentThreadId();
        // We won't be able to change a window's Z order if it's not our own window,
        // so we use this small technique to pretend the foreground window is ours.
        ::AttachThreadInput(windowThreadProcessId, currentThreadId, TRUE);

        [[maybe_unused]] const auto &cleaner =
            qScopeGuard([windowThreadProcessId, currentThreadId]() {
                ::AttachThreadInput(windowThreadProcessId, currentThreadId, FALSE);
            });

        ::BringWindowToTop(hwnd);
        // Activate the window too. This will force us to the virtual desktop this
        // window is on, if it's on another virtual desktop.
        ::SetActiveWindow(hwnd);
        // Throw us on the active monitor.
        moveWindowToMonitor(hwnd, activeMonitor);
    }

    // Returns false if the menu is canceled
    static bool showSystemMenu_sys(HWND hWnd, const POINT &pos, const bool selectFirstEntry,
                                   const bool fixedSize) {
        HMENU hMenu = ::GetSystemMenu(hWnd, FALSE);
        if (!hMenu) {
            // The corresponding window doesn't have a system menu, most likely due to the
            // lack of the "WS_SYSMENU" window style. This situation should not be treated
            // as an error so just ignore it and return early.
            return true;
        }

        const auto windowStyles = ::GetWindowLongPtrW(hWnd, GWL_STYLE);
        const bool allowMaximize = windowStyles & WS_MAXIMIZEBOX;
        const bool allowMinimize = windowStyles & WS_MINIMIZEBOX;

        const bool maxOrFull = isMaximized(hWnd) || isFullScreen(hWnd);
        ::EnableMenuItem(hMenu, SC_CLOSE, (MF_BYCOMMAND | MFS_ENABLED));
        ::EnableMenuItem(
            hMenu, SC_MAXIMIZE,
            (MF_BYCOMMAND |
             ((maxOrFull || fixedSize || !allowMaximize) ? MFS_DISABLED : MFS_ENABLED)));
        ::EnableMenuItem(
            hMenu, SC_RESTORE,
            (MF_BYCOMMAND |
             ((maxOrFull && !fixedSize && allowMaximize) ? MFS_ENABLED : MFS_DISABLED)));
        // The first menu item should be selected by default if the menu is brought
        // up by keyboard. I don't know how to pre-select a menu item but it seems
        // highlight can do the job. However, there's an annoying issue if we do
        // this manually: the highlighted menu item is really only highlighted,
        // not selected, so even if the mouse cursor hovers on other menu items
        // or the user navigates to other menu items through keyboard, the original
        // highlight bar will not move accordingly, the OS will generate another
        // highlight bar to indicate the current selected menu item, which will make
        // the menu look kind of weird. Currently I don't know how to fix this issue.
        ::HiliteMenuItem(hWnd, hMenu, SC_RESTORE,
                         (MF_BYCOMMAND | (selectFirstEntry ? MFS_HILITE : MFS_UNHILITE)));
        ::EnableMenuItem(hMenu, SC_MINIMIZE,
                         (MF_BYCOMMAND | (allowMinimize ? MFS_ENABLED : MFS_DISABLED)));
        ::EnableMenuItem(hMenu, SC_SIZE,
                         (MF_BYCOMMAND | ((maxOrFull || fixedSize) ? MFS_DISABLED : MFS_ENABLED)));
        ::EnableMenuItem(hMenu, SC_MOVE, (MF_BYCOMMAND | (maxOrFull ? MFS_DISABLED : MFS_ENABLED)));

        // The default menu item will appear in bold font. There can only be one default
        // menu item per menu at most. Set the item ID to "UINT_MAX" (or simply "-1")
        // can clear the default item for the given menu.
        UINT defaultItemId = UINT_MAX;
        if (isWin11OrGreater()) {
            if (maxOrFull) {
                defaultItemId = SC_RESTORE;
            } else {
                defaultItemId = SC_MAXIMIZE;
            }
        }
        if (defaultItemId == UINT_MAX) {
            defaultItemId = SC_CLOSE;
        }
        ::SetMenuDefaultItem(hMenu, defaultItemId, FALSE);

        // Popup the system menu at the required position.
        const auto result = ::TrackPopupMenu(
            hMenu,
            (TPM_RETURNCMD | (QGuiApplication::isRightToLeft() ? TPM_RIGHTALIGN : TPM_LEFTALIGN) |
             TPM_RIGHTBUTTON),
            pos.x, pos.y, 0, hWnd, nullptr);

        // Unhighlight the first menu item after the popup menu is closed, otherwise it will keep
        // highlighting until we unhighlight it manually.
        ::HiliteMenuItem(hWnd, hMenu, SC_RESTORE, (MF_BYCOMMAND | MFS_UNHILITE));

        if (!result) {
            // The user canceled the menu, no need to continue.
            return false;
        }

        // Send the command that the user chooses to the corresponding window.
        ::PostMessageW(hWnd, WM_SYSCOMMAND, result, 0);
        return true;
    }

    static inline Win32WindowContext::WindowPart getHitWindowPart(int hitTestResult) {
        switch (hitTestResult) {
            case HTCLIENT:
                return Win32WindowContext::ClientArea;
            case HTCAPTION:
                return Win32WindowContext::TitleBar;
            case HTSYSMENU:
            case HTHELP:
            case HTREDUCE:
            case HTZOOM:
            case HTCLOSE:
                return Win32WindowContext::ChromeButton;
            case HTLEFT:
            case HTRIGHT:
            case HTTOP:
            case HTTOPLEFT:
            case HTTOPRIGHT:
            case HTBOTTOM:
            case HTBOTTOMLEFT:
            case HTBOTTOMRIGHT:
                return Win32WindowContext::ResizeBorder;
            case HTBORDER:
                return Win32WindowContext::FixedBorder;
            default:
                break;
        }
        return Win32WindowContext::Outside;
    }

    static bool isValidWindow(HWND hWnd, bool checkVisible, bool checkTopLevel) {
        if (!::IsWindow(hWnd)) {
            return false;
        }
        const LONG_PTR styles = ::GetWindowLongPtrW(hWnd, GWL_STYLE);
        if (styles & WS_DISABLED) {
            return false;
        }
        const LONG_PTR exStyles = ::GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
        if (exStyles & WS_EX_TOOLWINDOW) {
            return false;
        }
        RECT rect{};
        if (!::GetWindowRect(hWnd, &rect)) {
            return false;
        }
        if ((rect.left >= rect.right) || (rect.top >= rect.bottom)) {
            return false;
        }
        if (checkVisible) {
            if (!::IsWindowVisible(hWnd)) {
                return false;
            }
        }
        if (checkTopLevel) {
            if (::GetAncestor(hWnd, GA_ROOT) != hWnd) {
                return false;
            }
        }
        return true;
    }

    static inline constexpr bool isNonClientMessage(const UINT message) {
        if (((message >= WM_NCCREATE) && (message <= WM_NCACTIVATE)) ||
            ((message >= WM_NCMOUSEMOVE) && (message <= WM_NCMBUTTONDBLCLK)) ||
            ((message >= WM_NCXBUTTONDOWN) && (message <= WM_NCXBUTTONDBLCLK))
#if (WINVER >= _WIN32_WINNT_WIN8)
            || ((message >= WM_NCPOINTERUPDATE) && (message <= WM_NCPOINTERUP))
#endif
            || ((message == WM_NCMOUSEHOVER) || (message == WM_NCMOUSELEAVE))) {
            return true;
        } else {
            return false;
        }
    }

    static MSG createMessageBlock(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        MSG msg;
        msg.hwnd = hWnd;
        msg.message = message;
        msg.wParam = wParam;
        msg.lParam = lParam;

        const DWORD dwScreenPos = ::GetMessagePos();
        msg.pt.x = GET_X_LPARAM(dwScreenPos);
        msg.pt.y = GET_Y_LPARAM(dwScreenPos);
        if (!isNonClientMessage(message)) {
            ::ScreenToClient(hWnd, &msg.pt);
        }

        msg.time = ::GetMessageTime();
        return msg;
    }

    static inline constexpr bool isInputMessage(UINT m) {
        switch (m) {
            case WM_IME_STARTCOMPOSITION:
            case WM_IME_ENDCOMPOSITION:
            case WM_IME_COMPOSITION:
            case WM_INPUT:
            case WM_TOUCH:
            case WM_MOUSEHOVER:
            case WM_MOUSELEAVE:
            case WM_NCMOUSEHOVER:
            case WM_NCMOUSELEAVE:
            case WM_SIZING:
            case WM_MOVING:
            case WM_SYSCOMMAND:
            case WM_COMMAND:
            case WM_DWMNCRENDERINGCHANGED:
            case WM_PAINT:
                return true;
            default:
                break;
        }
        return (m >= WM_MOUSEFIRST && m <= WM_MOUSELAST) ||
               (m >= WM_NCMOUSEMOVE && m <= WM_NCXBUTTONDBLCLK) ||
               (m >= WM_KEYFIRST && m <= WM_KEYLAST);
    }

    static inline QByteArray nativeEventType() {
        return QByteArrayLiteral("windows_generic_MSG");
    }

    // Send to QAbstractEventDispatcher
    static bool filterNativeEvent(MSG *msg, LRESULT *result) {
        auto dispatcher = QAbstractEventDispatcher::instance();
        QT_NATIVE_EVENT_RESULT_TYPE filterResult = *result;
        if (dispatcher && dispatcher->filterNativeEvent(nativeEventType(), msg, &filterResult)) {
            *result = LRESULT(filterResult);
            return true;
        }
        return false;
    }

    // Send to QWindowSystemInterface
    static bool filterNativeEvent(QWindow *window, MSG *msg, LRESULT *result) {
        QT_NATIVE_EVENT_RESULT_TYPE filterResult = *result;
        if (QWindowSystemInterface::handleNativeEvent(window, nativeEventType(), msg,
                                                      &filterResult)) {
            *result = LRESULT(filterResult);
            return true;
        }
        return false;
    }

    static inline bool forwardFilteredEvent(QWindow *window, HWND hWnd, UINT message, WPARAM wParam,
                                            LPARAM lParam, LRESULT *result) {
        MSG msg = createMessageBlock(hWnd, message, wParam, lParam);

        // https://github.com/qt/qtbase/blob/e26a87f1ecc40bc8c6aa5b889fce67410a57a702/src/plugins/platforms/windows/qwindowscontext.cpp#L1025
        // Do exact the same as what Qt Windows plugin does.

        // Run the native event filters. QTBUG-67095: Exclude input messages which are sent
        // by QEventDispatcherWin32::processEvents()
        if (!isInputMessage(msg.message) && filterNativeEvent(&msg, result))
            return true;

        auto platformWindow = window->handle();
        if (platformWindow && filterNativeEvent(platformWindow->window(), &msg, result))
            return true;

        return false;
    }

    // https://github.com/qt/qtbase/blob/e26a87f1ecc40bc8c6aa5b889fce67410a57a702/src/plugins/platforms/windows/qwindowscontext.cpp#L1556
    // In QWindowsContext::windowsProc(), the messages will be passed to all global native event
    // filters, but because we have already filtered the messages in the hook WndProc function for
    // convenience, Qt does not know we may have already processed the messages and thus will call
    // DefWindowProc(). Consequently, we have to add a global native filter that forwards the result
    // of the hook function, telling Qt whether we have filtered the events before. Since Qt only
    // handles Windows window messages in the main thread, it is safe to do so.
    class WindowsNativeEventFilter : public AppNativeEventFilter {
    public:
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            Q_UNUSED(eventType)

            // It has been observed that the pointer that Qt gives us is sometimes null on some
            // machines. We need to guard against it in such scenarios.
            if (!result) {
                return false;
            }

            auto msg = static_cast<const MSG *>(message);
            switch (msg->message) {
                case WM_NCCALCSIZE: {
                    // https://github.com/qt/qtbase/blob/e26a87f1ecc40bc8c6aa5b889fce67410a57a702/src/plugins/platforms/windows/qwindowscontext.cpp#L1546
                    // Qt needs to refer to the WM_NCCALCSIZE message data that hasn't been
                    // processed, so we have to process it after Qt acquires the initial data.
                    if (lastMessageContext) {
                        LRESULT res;
                        if (lastMessageContext->nonClientCalcSizeHandler(
                                msg->hwnd, msg->message, msg->wParam, msg->lParam, &res)) {
                            *result = decltype(*result)(res);
                            return true;
                        }
                    }
                    break;
                }

                    // case WM_NCHITTEST: {
                    //     // The child window must return HTTRANSPARENT when processing
                    //     WM_NCHITTEST for
                    //     // the parent window to receive WM_NCHITTEST.
                    //     if (!lastMessageContext) {
                    //         auto rootHWnd = ::GetAncestor(msg->hwnd, GA_ROOT);
                    //         if (rootHWnd != msg->hwnd) {
                    //             if (auto ctx = g_wndProcHash->value(rootHWnd)) {
                    //                 *result = HTTRANSPARENT;
                    //                 return true;
                    //             }
                    //         }
                    //     }
                    //     break;
                    // }
            }
            return false;
        }

        static inline WindowsNativeEventFilter *instance = nullptr;
        static inline Win32WindowContext *lastMessageContext = nullptr;

        static inline void install() {
            if (instance) {
                return;
            }
            instance = new WindowsNativeEventFilter();
        }

        static inline void uninstall() {
            if (!instance) {
                return;
            }
            delete instance;
            instance = nullptr;
        }
    };

    // https://github.com/qt/qtbase/blob/e26a87f1ecc40bc8c6aa5b889fce67410a57a702/src/plugins/platforms/windows/qwindowscontext.cpp#L1025
    // We can see from the source code that Qt will filter out some messages first and then send the
    // unfiltered messages to the event dispatcher. To activate the Snap Layout feature on Windows
    // 11, we must process some non-client area messages ourselves, but unfortunately these messages
    // have been filtered out already in that line, and thus we'll never have the chance to process
    // them ourselves. This is Qt's low level platform specific code, so we don't have any official
    // ways to change this behavior. But luckily we can replace the window procedure function of
    // Qt's windows, and in this hooked window procedure function, we finally have the chance to
    // process window messages before Qt touches them. So we reconstruct the MSG structure and send
    // it to our own custom native event filter to do all the magic works. But since the system menu
    // feature doesn't necessarily belong to the native implementation, we seperate the handling
    // code and always process the system menu part in this function for both implementations.
    //
    // Original event flow:
    //      [Entry]             Windows Message Queue
    //                          |
    //      [Qt Window Proc]    qwindowscontext.cpp#L1547: qWindowsWndProc()
    //                              ```
    //                              const bool handled = QWindowsContext::instance()->windowsProc
    //                                  (hwnd, message, et, wParam, lParam, &result,
    //                                  &platformWindow);
    //                              ```
    //                          |
    //      [Non-Input Filter]  qwindowscontext.cpp#L1025: QWindowsContext::windowsProc()
    //                              ```
    //                              if (!isInputMessage(msg.message) &&
    //                                  filterNativeEvent(&msg, result))
    //                                  return true;
    //                              ```
    //                          |
    //      [User Filter]       qwindowscontext.cpp#L1588: QWindowsContext::windowsProc()
    //                              ```
    //                              QAbstractEventDispatcher *dispatcher =
    //                              QAbstractEventDispatcher::instance();
    //                              qintptr filterResult = 0;
    //                              if (dispatcher &&
    //                              dispatcher->filterNativeEvent(nativeEventType(), msg,
    //                              &filterResult)) {
    //                                  *result = LRESULT(filterResult);
    //                                  return true;
    //                              }
    //                              ```
    //                          |
    //      [Extra work]        The rest of QWindowsContext::windowsProc() and qWindowsWndProc()
    //
    // Notice: Only non-input messages will be processed by the user-defined global native event
    // filter!!! These events are then passed to the widget class's own overridden
    // QWidget::nativeEvent() as a local filter, where all native events can be handled, but we must
    // create a new class derived from QWidget which we don't intend to. Therefore, we don't expect
    // to process events from the global native event filter, but instead hook Qt's window
    // procedure.

    extern "C" LRESULT QT_WIN_CALLBACK QWKHookedWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                                        LPARAM lParam) {
        Q_ASSERT(hWnd);
        if (!hWnd) {
            return FALSE;
        }

        // Search window context
        auto ctx = g_wndProcHash->value(hWnd);
        if (!ctx) {
            return ::DefWindowProcW(hWnd, message, wParam, lParam);
        }

        WindowsNativeEventFilter::lastMessageContext = ctx;
        const auto &contextCleaner = qScopeGuard([]() {
            WindowsNativeEventFilter::lastMessageContext = nullptr; //
        });

        // Since Qt does the necessary processing of the WM_NCCALCSIZE message, we need to
        // forward it right away and process it in our native event filter.
        if (message == WM_NCCALCSIZE) {
            return ::CallWindowProcW(g_qtWindowProc, hWnd, message, wParam, lParam);
        }

        // Try hooked procedure and save result
        LRESULT result;
        if (ctx->windowProc(hWnd, message, wParam, lParam, &result)) {
            // https://github.com/stdware/qwindowkit/issues/45
            // Forward the event to user-defined native event filters, there may be some messages
            // that need to be processed by the user.
            std::ignore =
                forwardFilteredEvent(ctx->window(), hWnd, message, wParam, lParam, &result);
            return result;
        }

        // Continue dispatching.
        return ::CallWindowProcW(g_qtWindowProc, hWnd, message, wParam, lParam);
    }

    static inline void addManagedWindow(QWindow *window, HWND hWnd, Win32WindowContext *ctx) {
        if (isSystemBorderEnabled()) {
            // Inform Qt we want and have set custom margins
            setInternalWindowFrameMargins(window, QMargins(0, -int(getTitleBarHeight(hWnd)), 0, 0));
        }

        // Store original window proc
        if (!g_qtWindowProc) {
            g_qtWindowProc = reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(hWnd, GWLP_WNDPROC));
        }

        // Hook window proc
        ::SetWindowLongPtrW(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(QWKHookedWndProc));

        // Install global native event filter
        WindowsNativeEventFilter::install();

        // Save window handle mapping
        g_wndProcHash->insert(hWnd, ctx);

        // Force a WM_NCCALCSIZE message manually to avoid the title bar become visible
        // while Qt is re-creating the window (such as setWindowFlag(s) calls). It has
        // been observed by our users.
        triggerFrameChange(hWnd);
    }

    static inline void removeManagedWindow(HWND hWnd) {
        // Remove window handle mapping
        if (!g_wndProcHash->remove(hWnd))
            return;

        // Remove event filter if the all windows has been destroyed
        if (g_wndProcHash->empty()) {
            WindowsNativeEventFilter::uninstall();
        }
    }

    Win32WindowContext::Win32WindowContext() : AbstractWindowContext() {
    }

    Win32WindowContext::~Win32WindowContext() {
        if (m_windowId) {
            removeManagedWindow(reinterpret_cast<HWND>(m_windowId));
        }
    }

    QString Win32WindowContext::key() const {
        return QStringLiteral("win32");
    }

    void Win32WindowContext::virtual_hook(int id, void *data) {
        switch (id) {
            case RaiseWindowHook: {
                if (!m_windowId)
                    return;
                m_delegate->setWindowVisible(m_host, true);
                const auto hwnd = reinterpret_cast<HWND>(m_windowId);
                bringWindowToFront(hwnd);
                return;
            }

            case ShowSystemMenuHook: {
                if (!m_windowId)
                    return;
                const auto &pos = *static_cast<const QPoint *>(data);
                auto hWnd = reinterpret_cast<HWND>(m_windowId);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                const QPoint nativeGlobalPos =
                    QHighDpi::toNativeGlobalPosition(pos, m_windowHandle.data());
#else
                const QPoint nativeGlobalPos = QHighDpi::toNativePixels(pos, m_windowHandle.data());
#endif
                std::ignore = showSystemMenu_sys(hWnd, qpoint2point(nativeGlobalPos), false,
                                                 isHostSizeFixed());
                return;
            }

            case DefaultColorsHook: {
                auto &map = *static_cast<QMap<QString, QColor> *>(data);
                map.clear();
                map.insert(QStringLiteral("activeLight"), kWindowsColorSet.activeLight);
                map.insert(QStringLiteral("activeDark"), kWindowsColorSet.activeDark);
                map.insert(QStringLiteral("inactiveLight"), kWindowsColorSet.inactiveLight);
                map.insert(QStringLiteral("inactiveDark"), kWindowsColorSet.inactiveDark);
                return;
            }

#if QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)
            case DrawWindows10BorderHook_Emulated: {
                if (!m_windowId)
                    return;

                auto args = static_cast<void **>(data);
                auto &painter = *static_cast<QPainter *>(args[0]);
                const auto &rect = *static_cast<const QRect *>(args[1]);
                const auto &region = *static_cast<const QRegion *>(args[2]);
                const auto hwnd = reinterpret_cast<HWND>(m_windowId);

                QPen pen;
                pen.setWidth(int(getWindowFrameBorderThickness(hwnd)) * 2);

                const bool dark = isDarkThemeActive() && isDarkWindowFrameEnabled(hwnd);
                if (m_delegate->isWindowActive(m_host)) {
                    if (isWindowFrameBorderColorized()) {
                        pen.setColor(getAccentColor());
                    } else {
                        static QColor frameBorderActiveColorLight(kWindowsColorSet.activeLight);
                        static QColor frameBorderActiveColorDark(kWindowsColorSet.activeDark);
                        pen.setColor(dark ? frameBorderActiveColorDark
                                          : frameBorderActiveColorLight);
                    }
                } else {
                    static QColor frameBorderInactiveColorLight(kWindowsColorSet.inactiveLight);
                    static QColor frameBorderInactiveColorDark(kWindowsColorSet.inactiveDark);
                    pen.setColor(dark ? frameBorderInactiveColorDark
                                      : frameBorderInactiveColorLight);
                }
                painter.save();

                // We need antialiasing to give us better result.
                painter.setRenderHint(QPainter::Antialiasing);

                painter.setPen(pen);
                painter.drawLine(QLine{
                    QPoint{0,                       0},
                    QPoint{m_windowHandle->width(), 0}
                });
                painter.restore();
                return;
            }

            case DrawWindows10BorderHook_Native: {
                if (!m_windowId)
                    return;

                // https://github.com/microsoft/terminal/blob/71a6f26e6ece656084e87de1a528c4a8072eeabd/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp#L1025
                // https://docs.microsoft.com/en-us/windows/win32/dwm/customframe#extending-the-client-frame
                // Draw a black rectangle to make Windows native top border show

                auto hWnd = reinterpret_cast<HWND>(m_windowId);
                HDC hdc = ::GetDC(hWnd);
                RECT windowRect{};
                ::GetClientRect(hWnd, &windowRect);
                RECT rcTopBorder = {
                    0,
                    0,
                    RECT_WIDTH(windowRect),
                    int(getWindowFrameBorderThickness(hWnd)),
                };
                ::FillRect(hdc, &rcTopBorder,
                           reinterpret_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
                ::ReleaseDC(hWnd, hdc);
                return;
            }
#endif

            default:
                break;
        }
        AbstractWindowContext::virtual_hook(id, data);
    }

    QVariant Win32WindowContext::windowAttribute(const QString &key) const {
        if (key == QStringLiteral("window-rect")) {
            if (!m_windowId)
                return {};

            RECT frame{};
            auto hwnd = reinterpret_cast<HWND>(m_windowId);
            // According to MSDN, WS_OVERLAPPED is not allowed for AdjustWindowRect.
            auto style = static_cast<DWORD>(::GetWindowLongPtrW(hwnd, GWL_STYLE) & ~WS_OVERLAPPED);
            auto exStyle = static_cast<DWORD>(::GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
            const DynamicApis &apis = DynamicApis::instance();
            if (apis.pAdjustWindowRectExForDpi) {
                apis.pAdjustWindowRectExForDpi(&frame, style, FALSE, exStyle,
                                               getDpiForWindow(hwnd));
            } else {
                ::AdjustWindowRectEx(&frame, style, FALSE, exStyle);
            }
            return QVariant::fromValue(rect2qrect(frame));
        }

        if (key == QStringLiteral("win10-border-needed")) {
            return isSystemBorderEnabled() && !isWin11OrGreater();
        }

        if (key == QStringLiteral("windows-system-border-enabled")) {
            return isSystemBorderEnabled();
        }

        if (key == QStringLiteral("border-thickness")) {
            return m_windowId
                       ? int(getWindowFrameBorderThickness(reinterpret_cast<HWND>(m_windowId)))
                       : 0;
        }

        if (key == QStringLiteral("title-bar-height")) {
            return m_windowId ? int(getTitleBarHeight(reinterpret_cast<HWND>(m_windowId))) : 0;
        }
        return AbstractWindowContext::windowAttribute(key);
    }

    void Win32WindowContext::winIdChanged(WId winId, WId oldWinId) {
        // Reset the context data
        mouseLeaveBlocked = false;
        lastHitTestResult = WindowPart::Outside;
        lastHitTestResultRaw = HTNOWHERE;

        // If the original window id is valid, remove all resources related
        if (oldWinId) {
            removeManagedWindow(reinterpret_cast<HWND>(oldWinId));
        }
        if (!winId) {
            return;
        }

        // Install window hook
        auto hWnd = reinterpret_cast<HWND>(winId);
        if (!isSystemBorderEnabled()) {
            static auto margins = QVariant::fromValue(QMargins(1, 1, 1, 1));

            // If we remove the system border, the window will lose its shadow. If dwm is enabled,
            // then we need to set at least 1px margins, otherwise the following operation will
            // fail with no effect.
            setWindowAttribute(QStringLiteral("extra-margins"), margins);
        }

        // We should disable WS_SYSMENU, otherwise the system button icons will be visible if mica
        // is enabled and the title bar is transparent.
        {
            auto style = ::GetWindowLongPtrW(hWnd, GWL_STYLE);
            if (isSystemBorderEnabled()) {
                ::SetWindowLongPtrW(hWnd, GWL_STYLE, style & (~WS_SYSMENU));
            } else {
                ::SetWindowLongPtrW(hWnd, GWL_STYLE,
                                    (style | WS_THICKFRAME | WS_CAPTION) & (~WS_SYSMENU));
            }
        }

        // Add managed window
        addManagedWindow(m_windowHandle, hWnd, this);
    }

    bool Win32WindowContext::windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                                        LRESULT *result) {
        *result = FALSE;

        // We should skip these messages otherwise we will get crashes.
        // NOTE: WM_QUIT won't be posted to the WindowProc function.
        switch (message) {
            case WM_DESTROY:
            case WM_CLOSE:
            case WM_NCDESTROY:
            // Undocumented messages:
            case WM_UAHDESTROYWINDOW:
            case WM_UNREGISTER_WINDOW_SERVICES:
                return false;
            default:
                break;
        }

        if (!isValidWindow(hWnd, false, true)) {
            return false;
        }

        // Test snap layout
        if (snapLayoutHandler(hWnd, message, wParam, lParam, result)) {
            return true;
        }

        // Main implementation
        if (customWindowHandler(hWnd, message, wParam, lParam, result)) {
            return true;
        }

        // Whether to show system menu
        if (systemMenuHandler(hWnd, message, wParam, lParam, result)) {
            return true;
        }

        // Forward to native event filter subscribers
        if (!m_nativeEventFilters.isEmpty()) {
            MSG msg = createMessageBlock(hWnd, message, wParam, lParam);
            QT_NATIVE_EVENT_RESULT_TYPE res = 0;
            if (nativeDispatch(nativeEventType(), &msg, &res)) {
                *result = LRESULT(res);
                return true;
            }
        }
        return false; // Not handled
    }

    bool Win32WindowContext::windowAttributeChanged(const QString &key, const QVariant &attribute,
                                                    const QVariant &oldAttribute) {
        Q_UNUSED(oldAttribute)

        const auto hwnd = reinterpret_cast<HWND>(m_windowId);
        Q_ASSERT(hwnd);

        const DynamicApis &apis = DynamicApis::instance();
        const auto &extendMargins = [this, &apis, hwnd]() {
            // For some unknown reason, the window background is totally black and extending
            // the window frame into the client area seems to fix it magically.
            // After many times of trying, we found that the Acrylic/Mica/Mica Alt background
            // only appears on the native Win32 window's background, so naturally we want to
            // extend the window frame into the whole client area to be able to let the special
            // material fill the whole window. Previously we are using negative margins because
            // it's widely known that using negative margins will let the window frame fill
            // the whole window and that's indeed what we wanted to do, however, later we found
            // that doing so is causing issues. When the user enabled the "show accent color on
            // window title bar and borders" option on system personalize settings, a 30px bar
            // would appear on window top. It has the same color with the system accent color.
            // Actually it's the original title bar we've already hidden, and it magically
            // appears again when we use negative margins to extend the window frame. And again
            // after some experiments, I found that the title bar won't appear if we don't extend
            // from the top side. In the end I found that we only need to extend from the left
            // side if we extend long enough. In this way we can see the special material even
            // when the host object is a QWidget and the title bar still remain hidden. But even
            // though this solution seems perfect, I really don't know why it works. The following
            // hack is totally based on experiments.
            static constexpr const MARGINS margins = {65536, 0, 0, 0};
            apis.pDwmExtendFrameIntoClientArea(hwnd, &margins);
        };
        const auto &restoreMargins = [this, &apis, hwnd]() {
            auto margins = qmargins2margins(
                windowAttribute(QStringLiteral("extra-margins")).value<QMargins>());
            apis.pDwmExtendFrameIntoClientArea(hwnd, &margins);
        };

        const auto &effectBugWorkaround = [this, hwnd]() {
            // We don't need the following *HACK* for QWidget windows.
            // Completely based on actual experiments, root reason is totally unknown.
            if (m_host->isWidgetType()) {
                return;
            }
            static QSet<WId> bugWindowSet{};
            if (bugWindowSet.contains(m_windowId)) {
                return;
            }
            bugWindowSet.insert(m_windowId);
            RECT rect{};
            ::GetWindowRect(hwnd, &rect);
            ::MoveWindow(hwnd, rect.left, rect.top, 1, 1, FALSE);
            ::MoveWindow(hwnd, rect.right - 1, rect.bottom - 1, 1, 1, FALSE);
            ::MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                         FALSE);
        };

        if (key == QStringLiteral("extra-margins")) {
            auto margins = qmargins2margins(attribute.value<QMargins>());
            return SUCCEEDED(apis.pDwmExtendFrameIntoClientArea(hwnd, &margins));
        }

        if (key == QStringLiteral("dark-mode")) {
            if (!isWin101809OrGreater()) {
                return false;
            }

            BOOL enable = attribute.toBool();
            if (isWin101903OrGreater()) {
                apis.pSetPreferredAppMode(enable ? PAM_AUTO : PAM_DEFAULT);
            } else {
                apis.pAllowDarkModeForApp(enable);
            }
            const auto attr = isWin1020H1OrGreater() ? _DWMWA_USE_IMMERSIVE_DARK_MODE
                                                     : _DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1;
            apis.pDwmSetWindowAttribute(hwnd, attr, &enable, sizeof(enable));

            apis.pFlushMenuThemes();
            return true;
        }

        // For Win11 or later
        if (key == QStringLiteral("mica")) {
            if (!isWin11OrGreater()) {
                return false;
            }
            if (attribute.toBool()) {
                extendMargins();
                if (isWin1122H2OrGreater()) {
                    // Use official DWM API to enable Mica, available since Windows 11 22H2
                    // (10.0.22621).
                    const _DWM_SYSTEMBACKDROP_TYPE backdropType = _DWMSBT_MAINWINDOW;
                    apis.pDwmSetWindowAttribute(hwnd, _DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                                                sizeof(backdropType));
                } else {
                    // Use undocumented DWM API to enable Mica, available since Windows 11
                    // (10.0.22000).
                    const BOOL enable = TRUE;
                    apis.pDwmSetWindowAttribute(hwnd, _DWMWA_MICA_EFFECT, &enable, sizeof(enable));
                }
            } else {
                if (isWin1122H2OrGreater()) {
                    const _DWM_SYSTEMBACKDROP_TYPE backdropType = _DWMSBT_AUTO;
                    apis.pDwmSetWindowAttribute(hwnd, _DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                                                sizeof(backdropType));
                } else {
                    const BOOL enable = FALSE;
                    apis.pDwmSetWindowAttribute(hwnd, _DWMWA_MICA_EFFECT, &enable, sizeof(enable));
                }
                restoreMargins();
            }
            effectBugWorkaround();
            return true;
        }

        if (key == QStringLiteral("mica-alt")) {
            if (!isWin1122H2OrGreater()) {
                return false;
            }
            if (attribute.toBool()) {
                extendMargins();
                // Use official DWM API to enable Mica Alt, available since Windows 11 22H2
                // (10.0.22621).
                const _DWM_SYSTEMBACKDROP_TYPE backdropType = _DWMSBT_TABBEDWINDOW;
                apis.pDwmSetWindowAttribute(hwnd, _DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                                            sizeof(backdropType));
            } else {
                const _DWM_SYSTEMBACKDROP_TYPE backdropType = _DWMSBT_AUTO;
                apis.pDwmSetWindowAttribute(hwnd, _DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                                            sizeof(backdropType));
                restoreMargins();
            }
            effectBugWorkaround();
            return true;
        }

        if (key == QStringLiteral("acrylic-material")) {
            if (!isWin11OrGreater()) {
                return false;
            }
            if (attribute.toBool()) {
                extendMargins();

                const _DWM_SYSTEMBACKDROP_TYPE backdropType = _DWMSBT_TRANSIENTWINDOW;
                apis.pDwmSetWindowAttribute(hwnd, _DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                                            sizeof(backdropType));

                // PRIVATE API REFERENCE:
                //     QColor gradientColor = {};
                //     ACCENT_POLICY policy{};
                //     policy.dwAccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
                //     policy.dwAccentFlags = ACCENT_ENABLE_ACRYLIC_WITH_LUMINOSITY;
                //     // This API expects the #AABBGGRR format.
                //     policy.dwGradientColor =
                //         DWORD(qRgba(gradientColor.blue(), gradientColor.green(),
                //                     gradientColor.red(), gradientColor.alpha()));
                //     WINDOWCOMPOSITIONATTRIBDATA wcad{};
                //     wcad.Attrib = WCA_ACCENT_POLICY;
                //     wcad.pvData = &policy;
                //     wcad.cbData = sizeof(policy);
                //     apis.pSetWindowCompositionAttribute(hwnd, &wcad);
            } else {
                const _DWM_SYSTEMBACKDROP_TYPE backdropType = _DWMSBT_AUTO;
                apis.pDwmSetWindowAttribute(hwnd, _DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                                            sizeof(backdropType));

                // PRIVATE API REFERENCE:
                //     ACCENT_POLICY policy{};
                //     policy.dwAccentState = ACCENT_DISABLED;
                //     policy.dwAccentFlags = ACCENT_NONE;
                //     WINDOWCOMPOSITIONATTRIBDATA wcad{};
                //     wcad.Attrib = WCA_ACCENT_POLICY;
                //     wcad.pvData = &policy;
                //     wcad.cbData = sizeof(policy);
                //     apis.pSetWindowCompositionAttribute(hwnd, &wcad);

                restoreMargins();
            }
            effectBugWorkaround();
            return true;
        }

        if (key == QStringLiteral("dwm-blur")) {
            // Extending window frame would break this effect for some unknown reason.
            restoreMargins();
            if (attribute.toBool()) {
                if (isWin8OrGreater()) {
                    ACCENT_POLICY policy{};
                    policy.dwAccentState = ACCENT_ENABLE_BLURBEHIND;
                    policy.dwAccentFlags = ACCENT_NONE;
                    WINDOWCOMPOSITIONATTRIBDATA wcad{};
                    wcad.Attrib = WCA_ACCENT_POLICY;
                    wcad.pvData = &policy;
                    wcad.cbData = sizeof(policy);
                    apis.pSetWindowCompositionAttribute(hwnd, &wcad);
                } else {
                    DWM_BLURBEHIND bb{};
                    bb.fEnable = TRUE;
                    bb.dwFlags = DWM_BB_ENABLE;
                    apis.pDwmEnableBlurBehindWindow(hwnd, &bb);
                }
            } else {
                if (isWin8OrGreater()) {
                    ACCENT_POLICY policy{};
                    policy.dwAccentState = ACCENT_DISABLED;
                    policy.dwAccentFlags = ACCENT_NONE;
                    WINDOWCOMPOSITIONATTRIBDATA wcad{};
                    wcad.Attrib = WCA_ACCENT_POLICY;
                    wcad.pvData = &policy;
                    wcad.cbData = sizeof(policy);
                    apis.pSetWindowCompositionAttribute(hwnd, &wcad);
                } else {
                    DWM_BLURBEHIND bb{};
                    bb.fEnable = FALSE;
                    bb.dwFlags = DWM_BB_ENABLE;
                    apis.pDwmEnableBlurBehindWindow(hwnd, &bb);
                }
            }
            effectBugWorkaround();
            return true;
        }
        return false;
    }

    QWK_USED static constexpr const struct {
        const WPARAM wParam = MAKEWPARAM(44500, 61897);
        const LPARAM lParam = MAKELPARAM(62662, 44982); // Not used. Reserve for future use.
    } kMessageTag;

    static inline quint64 getKeyState() {
        quint64 result = 0;
        const auto &get = [](const int virtualKey) -> bool {
            return (::GetAsyncKeyState(virtualKey) < 0);
        };
        const bool buttonSwapped = ::GetSystemMetrics(SM_SWAPBUTTON);
        if (get(VK_LBUTTON)) {
            result |= (buttonSwapped ? MK_RBUTTON : MK_LBUTTON);
        }
        if (get(VK_RBUTTON)) {
            result |= (buttonSwapped ? MK_LBUTTON : MK_RBUTTON);
        }
        if (get(VK_SHIFT)) {
            result |= MK_SHIFT;
        }
        if (get(VK_CONTROL)) {
            result |= MK_CONTROL;
        }
        if (get(VK_MBUTTON)) {
            result |= MK_MBUTTON;
        }
        if (get(VK_XBUTTON1)) {
            result |= MK_XBUTTON1;
        }
        if (get(VK_XBUTTON2)) {
            result |= MK_XBUTTON2;
        }
        return result;
    }

    static void emulateClientAreaMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                                         const std::optional<int> &overrideMessage = std::nullopt) {
        const int myMsg = overrideMessage.value_or(message);
        const auto wParamNew = [myMsg, wParam]() -> WPARAM {
            if (myMsg == WM_NCMOUSELEAVE) {
                // wParam is always ignored in mouse leave messages, but here we
                // give them a special tag to be able to distinguish which messages
                // are sent by ourselves.
                return kMessageTag.wParam;
            }
            const quint64 keyState = getKeyState();
            if ((myMsg >= WM_NCXBUTTONDOWN) && (myMsg <= WM_NCXBUTTONDBLCLK)) {
                const auto xButtonMask = GET_XBUTTON_WPARAM(wParam);
                return MAKEWPARAM(keyState, xButtonMask);
            }
            return keyState;
        }();
        const auto lParamNew = [myMsg, lParam, hWnd]() -> LPARAM {
            if (myMsg == WM_NCMOUSELEAVE) {
                // lParam is always ignored in mouse leave messages.
                return 0;
            }
            const auto screenPos = POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            POINT clientPos = screenPos;
            ::ScreenToClient(hWnd, &clientPos);
            return MAKELPARAM(clientPos.x, clientPos.y);
        }();
#if 0
#  define SEND_MESSAGE ::SendMessageW
#else
#  define SEND_MESSAGE ::PostMessageW
#endif
        switch (myMsg) {
            case WM_NCHITTEST: // Treat hit test messages as mouse move events.
            case WM_NCMOUSEMOVE:
                SEND_MESSAGE(hWnd, WM_MOUSEMOVE, wParamNew, lParamNew);
                break;
            case WM_NCLBUTTONDOWN:
                SEND_MESSAGE(hWnd, WM_LBUTTONDOWN, wParamNew, lParamNew);
                break;
            case WM_NCLBUTTONUP:
                SEND_MESSAGE(hWnd, WM_LBUTTONUP, wParamNew, lParamNew);
                break;
            case WM_NCLBUTTONDBLCLK:
                SEND_MESSAGE(hWnd, WM_LBUTTONDBLCLK, wParamNew, lParamNew);
                break;
            case WM_NCRBUTTONDOWN:
                SEND_MESSAGE(hWnd, WM_RBUTTONDOWN, wParamNew, lParamNew);
                break;
            case WM_NCRBUTTONUP:
                SEND_MESSAGE(hWnd, WM_RBUTTONUP, wParamNew, lParamNew);
                break;
            case WM_NCRBUTTONDBLCLK:
                SEND_MESSAGE(hWnd, WM_RBUTTONDBLCLK, wParamNew, lParamNew);
                break;
            case WM_NCMBUTTONDOWN:
                SEND_MESSAGE(hWnd, WM_MBUTTONDOWN, wParamNew, lParamNew);
                break;
            case WM_NCMBUTTONUP:
                SEND_MESSAGE(hWnd, WM_MBUTTONUP, wParamNew, lParamNew);
                break;
            case WM_NCMBUTTONDBLCLK:
                SEND_MESSAGE(hWnd, WM_MBUTTONDBLCLK, wParamNew, lParamNew);
                break;
            case WM_NCXBUTTONDOWN:
                SEND_MESSAGE(hWnd, WM_XBUTTONDOWN, wParamNew, lParamNew);
                break;
            case WM_NCXBUTTONUP:
                SEND_MESSAGE(hWnd, WM_XBUTTONUP, wParamNew, lParamNew);
                break;
            case WM_NCXBUTTONDBLCLK:
                SEND_MESSAGE(hWnd, WM_XBUTTONDBLCLK, wParamNew, lParamNew);
                break;
#if 0 // ### TODO: How to handle touch events?
        case WM_NCPOINTERUPDATE:
        case WM_NCPOINTERDOWN:
        case WM_NCPOINTERUP:
            break;
#endif
            case WM_NCMOUSEHOVER:
                SEND_MESSAGE(hWnd, WM_MOUSEHOVER, wParamNew, lParamNew);
                break;
            case WM_NCMOUSELEAVE:
                SEND_MESSAGE(hWnd, WM_MOUSELEAVE, wParamNew, lParamNew);
                break;
            default:
                // unreachable
                break;
        }

#undef SEND_MESSAGE
    }

    static inline void requestForMouseLeaveMessage(HWND hWnd, bool nonClient) {
        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        if (nonClient) {
            tme.dwFlags |= TME_NONCLIENT;
        }
        tme.hwndTrack = hWnd;
        tme.dwHoverTime = HOVER_DEFAULT;
        ::TrackMouseEvent(&tme);
    }

    bool Win32WindowContext::snapLayoutHandler(HWND hWnd, UINT message, WPARAM wParam,
                                               LPARAM lParam, LRESULT *result) {
        switch (message) {
            case WM_MOUSELEAVE: {
                if (wParam != kMessageTag.wParam) {
                    // Qt will call TrackMouseEvent() to get the WM_MOUSELEAVE message when it
                    // receives WM_MOUSEMOVE messages, and since we are converting every
                    // WM_NCMOUSEMOVE message to WM_MOUSEMOVE message and send it back to the window
                    // to be able to hover our controls, we also get lots of WM_MOUSELEAVE messages
                    // at the same time because of the reason above, and these superfluous mouse
                    // leave events cause Qt to think the mouse has left the control, and thus we
                    // actually lost the hover state. So we filter out these superfluous mouse leave
                    // events here to avoid this issue.
                    DWORD dwScreenPos = ::GetMessagePos();
                    POINT screenPoint{GET_X_LPARAM(dwScreenPos), GET_Y_LPARAM(dwScreenPos)};
                    ::ScreenToClient(hWnd, &screenPoint);
                    QPoint qtScenePos = QHighDpi::fromNativeLocalPosition(point2qpoint(screenPoint),
                                                                          m_windowHandle.data());
                    auto dummy = WindowAgentBase::Unknown;
                    if (isInSystemButtons(qtScenePos, &dummy)) {
                        // We must record whether the last WM_MOUSELEAVE was filtered, because if
                        // Qt does not receive this message it will not call TrackMouseEvent()
                        // again, resulting in the client area not responding to any mouse event.
                        mouseLeaveBlocked = true;
                        *result = FALSE;
                        return true;
                    }
                }
                mouseLeaveBlocked = false;
                break;
            }

            case WM_MOUSEMOVE: {
                // At appropriate time, we will call TrackMouseEvent() for Qt. Simultaneously,
                // we unset `mouseLeaveBlocked` mark and pretend as if Qt has received
                // WM_MOUSELEAVE.
                if (lastHitTestResult != WindowPart::ChromeButton && mouseLeaveBlocked) {
                    mouseLeaveBlocked = false;
                    requestForMouseLeaveMessage(hWnd, false);
                }
                break;
            }

            case WM_NCMOUSEMOVE:
            case WM_NCLBUTTONDOWN:
            case WM_NCLBUTTONUP:
            case WM_NCLBUTTONDBLCLK:
            case WM_NCRBUTTONDOWN:
            case WM_NCRBUTTONUP:
            case WM_NCRBUTTONDBLCLK:
            case WM_NCMBUTTONDOWN:
            case WM_NCMBUTTONUP:
            case WM_NCMBUTTONDBLCLK:
            case WM_NCXBUTTONDOWN:
            case WM_NCXBUTTONUP:
            case WM_NCXBUTTONDBLCLK:
#if 0 // ### TODO: How to handle touch events?
    case WM_NCPOINTERUPDATE:
    case WM_NCPOINTERDOWN:
    case WM_NCPOINTERUP:
#endif
            case WM_NCMOUSEHOVER: {
                if (message == WM_NCMOUSEMOVE) {
                    if (lastHitTestResult != WindowPart::ChromeButton) {
                        // https://github.com/qt/qtbase/blob/e26a87f1ecc40bc8c6aa5b889fce67410a57a702/src/widgets/kernel/qwidgetwindow.cpp#L472
                        // When the mouse press event arrives, QWidgetWindow will implicitly grab
                        // the top widget right under the mouse, and set `qt_button_down` to this
                        // widget. At this time, no other widgets will accept the mouse event until
                        // QWidgetWindow receives the mouse release event, then set `qt_button_down`
                        // to null.

                        // Imagine the following situation, now the main window has a pop-up menu,
                        // the focus is not on the main window, if we click outside the pop-up menu,
                        // the menu will close, which seems to be completely fine. But if we close
                        // the menu by clicking on the title bar draggable area, then other widgets
                        // won't accept the mouse message afterwards.

                        // Here's the reason.
                        // When the mouse is placed in the draggable area of the title bar, there
                        // are two situations.

                        // 1. If the focus is on the main window, and the last result of
                        // WM_NCHITTEST is HTCAPTION, the mouse click event in the title bar is
                        // taken over by Windows and Qt does not receive the mouse click event.

                        // 2. If the main window has a pop-up menu, it is completely different. When
                        // the mouse is pressed on the title bar, Windows sends the WM_LBUTTONDOWN
                        // message to the window plane of the pop-up menu, the menu is closed, but
                        // Qt will continue to forward the event to the QWidget under the mouse, and
                        // the event will be processed by QWidgetWindow, causing the title bar
                        // widget to be implicitly grabbed. After the menu is closed, Windows
                        // immediately sends WM_NCHITTEST, because the mouse is in the title bar
                        // draggable area, the result is HTCAPTION, so when the mouse is released,
                        // Windows sends WM_NCLBUTTONUP, which is a non-client message, and it
                        // will be ignored by Qt. As a consequence, QWidgetWindow can't receive a
                        // mouse release message in the client area, so the grab remains, and other
                        // widgets cannot receive mouse events.

                        // Since we didn't watch the menu window, we cannot capture any mouse
                        // press events sent by Windows, so we cannot solve this problem by
                        // recording mouse events. Fortunately, we found that the main window will
                        // receive a WM_NCMOUSEMOVE message immediately after the menu is closed, so
                        // we just manually send a mouse release event when this message arrives and
                        // set qt_button_down to null. Don't worry, when receiving WM_NCMOUSEMOVE,
                        // there shouldn't be any control in the state of being grabbed.

                        // In the native window, although QWidgetWindow handles the forwarded mouse
                        // press event when the menu is closed, since the native title bar is not a
                        // QWidget, no widget will be grabbed, and `qt_button_down` remains empty,
                        // the above problems would not arise.

                        m_delegate->resetQtGrabbedControl(m_host);

                        // If the mouse moves from chrome buttons to other non-client areas, a
                        // WM_MOUSELEAVE message should be sent.
                        if (mouseLeaveBlocked) {
                            emulateClientAreaMessage(hWnd, message, wParam, lParam,
                                                     WM_NCMOUSELEAVE);
                        }
                    }
                }

                if (lastHitTestResult == WindowPart::ChromeButton) {
                    if (message == WM_NCMOUSEMOVE) {
                        // ### FIXME FIXME FIXME
                        // ### FIXME: Calling DefWindowProc() here is really dangerous, investigate
                        // how to avoid doing this.
                        // ### FIXME FIXME FIXME
                        *result = ::DefWindowProcW(hWnd, WM_NCMOUSEMOVE, wParam, lParam);
                        emulateClientAreaMessage(hWnd, message, wParam, lParam);
                        return true;
                    }

                    if (lastHitTestResultRaw == HTSYSMENU) {
                        switch (message) {
                            case WM_NCLBUTTONDOWN:
                                if (iconButtonClickLevel == 0) {
                                    // A message of WM_SYSCOMMAND with SC_MOUSEMENU will be sent by
                                    // Windows, and the current control flow will be blocked by the
                                    // menu while Windows will create and execute a new event loop
                                    // until the menu returns
                                    iconButtonClickTime = ::GetTickCount64();
                                    *result = ::DefWindowProcW(hWnd, message, wParam, lParam);
                                    iconButtonClickTime = 0;
                                    if (iconButtonClickLevel & IconButtonTriggersClose) {
                                        ::PostMessageW(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
                                    }
                                    if (iconButtonClickLevel & IconButtonDoubleClicked) {
                                        iconButtonClickLevel = 0;
                                    }
                                    // Otherwise, no need to reset `iconButtonClickLevel` if not to
                                    // close, if it has value, there must be another incoming
                                    // WM_NCLBUTTONDOWN
                                } else {
                                    iconButtonClickLevel = 0;
                                }
                                break;
                            case WM_NCLBUTTONDBLCLK:
                                // A message of WM_SYSCOMMAND with SC_CLOSE will be sent by Windows
                                *result = ::DefWindowProcW(hWnd, message, wParam, lParam);
                                break;
                            default:
                                *result = FALSE;
                                emulateClientAreaMessage(hWnd, message, wParam, lParam);
                                break;
                        }
                    } else {
                        // According to MSDN, we should return non-zero for X button messages to
                        // indicate we have handled these messages (due to historical reasons), for
                        // all other messages we should return zero instead.
                        *result =
                            (((message >= WM_NCXBUTTONDOWN) && (message <= WM_NCXBUTTONDBLCLK))
                                 ? TRUE
                                 : FALSE);
                        emulateClientAreaMessage(hWnd, message, wParam, lParam);
                    }
                    return true;
                }
                break;
            }

            case WM_NCMOUSELEAVE: {
                const WindowPart currentWindowPart = lastHitTestResult;
                if (currentWindowPart == WindowPart::ChromeButton) {
                    // If we press on the chrome button and move mouse, Windows will take the
                    // pressing area as HTCLIENT which maybe because of our former retransmission of
                    // WM_NCLBUTTONDOWN, as a result, a WM_NCMOUSELEAVE will come immediately and a
                    // lot of WM_MOUSEMOVE will come if we move the mouse, we should track the mouse
                    // in advance. (May be redundant?)
                    if (mouseLeaveBlocked) {
                        mouseLeaveBlocked = false;
                        requestForMouseLeaveMessage(hWnd, false);
                    }
                } else {
                    if (mouseLeaveBlocked) {
                        // The mouse is moving from the chrome button to other non-client area, we
                        // should emulate a WM_MOUSELEAVE message to reset the button state.
                        emulateClientAreaMessage(hWnd, message, wParam, lParam, WM_NCMOUSELEAVE);
                    }

                    if (currentWindowPart == WindowPart::Outside) {
                        // Notice: we're not going to clear window part cache when the mouse leaves
                        // window from client area, which means we will get previous window part as
                        // HTCLIENT if the mouse leaves window from client area and enters window
                        // from non-client area, but it has no bad effect.

                        // Why do we need to call this function here?
                        m_delegate->resetQtGrabbedControl(m_host);
                    }
                }
                break;
            }

            default:
                break;
        }
        return false;
    }

    bool Win32WindowContext::customWindowHandler(HWND hWnd, UINT message, WPARAM wParam,
                                                 LPARAM lParam, LRESULT *result) {
        switch (message) {
            case WM_NCHITTEST: {
                // 原生Win32窗口只有顶边是在窗口内部resize的，其余三边都是在窗口
                // 外部进行resize的，其原理是，WS_THICKFRAME这个窗口样式会在窗
                // 口的左、右和底边添加三个透明的resize区域，这三个区域在正常状态
                // 下是完全不可见的，它们由DWM负责绘制和控制。这些区域的宽度等于
                // (SM_CXSIZEFRAME + SM_CXPADDEDBORDER)，高度等于
                // (SM_CYSIZEFRAME + SM_CXPADDEDBORDER)，在100%缩放时，均等
                // 于8像素。它们属于窗口区域的一部分，但不属于客户区，而是属于非客
                // 户区，因此GetWindowRect获取的区域中是包含这三个resize区域的，
                // 而GetClientRect获取的区域是不包含它们的。当把
                // DWMWA_EXTENDED_FRAME_BOUNDS作为参数调用
                // DwmGetWindowAttribute时，也能获取到一个窗口大小，这个大小介
                // 于前面两者之间，暂时不知道这个数据的意义及其作用。我们在
                // WM_NCCALCSIZE消息的处理中，已经把整个窗口都设置为客户区了，也
                // 就是说，我们的窗口已经没有非客户区了，因此那三个透明的resize区
                // 域，此刻也已经成为窗口客户区的一部分了，从而变得不透明了。所以
                // 现在的resize，看起来像是在窗口内部resize，是因为原本透明的地方
                // 现在变得不透明了，实际上，单纯从范围上来看，现在我们resize的地方，
                // 就是普通窗口的边框外部，那三个透明区域的范围。
                // 因此，如果我们把边框完全去掉（就是我们正在做的事情），resize就
                // 会看起来是在内部进行，这个问题通过常规方法非常难以解决。我测试过
                // QQ和钉钉的窗口，它们的窗口就是在外部resize，但实际上它们是通过
                // 把窗口实际的内容，嵌入到一个完全透明的但尺寸要大一圈的窗口中实现
                // 的，虽然看起来效果还不错，但对于此项目而言，代码和窗口结构过于复
                // 杂，因此我没有采用此方案。然而，对于具体的软件项目而言，其做法也
                // 不失为一个优秀的解决方案，毕竟其在大多数条件下的表现都还可以。
                //
                // 和1.x的做法不同，现在的2.x选择了保留窗口三边，去除整个窗口顶部，
                // 好处是保留了系统的原生边框，外观较好，且与系统结合紧密，而且resize
                // 的表现也有很大改善，缺点是需要自行绘制顶部边框线。原本以为只能像
                // Windows Terminal那样在WM_PAINT里搞黑魔法，但后来发现，其实只
                // 要颜色相近，我们自行绘制一根实线也几乎能以假乱真，而且这样也不会
                // 破坏Qt自己的绘制系统，能做到不依赖黑魔法就能实现像Windows Terminal
                // 那样外观和功能都比较完美的自定义边框。

                // A normal Win32 window can be resized outside of it. Here is the
                // reason: the WS_THICKFRAME window style will cause a window has three
                // transparent areas beside the window's left, right and bottom
                // edge. Their width or height is eight pixels if the window is not
                // scaled. In most cases, they are totally invisible. It's DWM's
                // responsibility to draw and control them. They exist to let the
                // user resize the window, visually outside of it. They are in the
                // window area, but not the client area, so they are in the
                // non-client area actually. But we have turned the whole window
                // area into client area in WM_NCCALCSIZE, so the three transparent
                // resize areas also become a part of the client area and thus they
                // become visible. When we resize the window, it looks like we are
                // resizing inside of it, however, that's because the transparent
                // resize areas are visible now, we ARE resizing outside of the
                // window actually. But I don't know how to make them become
                // transparent again without breaking the frame shadow drawn by DWM.
                // If you really want to solve it, you can try to embed your window
                // into a larger transparent window and draw the frame shadow
                // yourself. As what we have said in WM_NCCALCSIZE, you can only
                // remove the top area of the window, this will let us be able to
                // resize outside of the window and don't need much process in this
                // message, it looks like a perfect plan, however, the top border is
                // missing due to the whole top area is removed, and it's very hard
                // to bring it back because we have to use a trick in WM_PAINT
                // (learned from Windows Terminal), but no matter what we do in
                // WM_PAINT, it will always break the backing store mechanism of Qt,
                // so actually we can't do it. And it's very difficult to do such
                // things in NativeEventFilters as well. What's worse, if we really
                // do this, the four window borders will become white and they look
                // horrible in dark mode. This solution only supports Windows 10
                // because the border width on Win10 is only one pixel, however it's
                // eight pixels on Windows 7 so preserving the three window borders
                // looks terrible on old systems.
                //
                // Unlike the 1.x code, we choose to preserve the three edges of the
                // window in 2.x, and get rid of the whole top part of the window.
                // There are quite some advantages such as the appearance looks much
                // better and due to we have the original system window frame, our
                // window can behave just like a normal Win32 window even if we now
                // doesn't have a title bar at all. Most importantly, the flicker and
                // jitter during window resizing is totally gone now. The disadvantage
                // is we have to draw a top frame border ourselves. Previously I thought
                // we have to do the black magic in WM_PAINT just like what Windows
                // Terminal does, however, later I found that if we choose a proper
                // color, our homemade top border can almost have exactly the same
                // appearance with the system's one.
                [[maybe_unused]] const auto &hitTestRecorder = qScopeGuard([this, result]() {
                    lastHitTestResultRaw = int(*result);
                    lastHitTestResult = getHitWindowPart(lastHitTestResultRaw);
                });

                POINT nativeGlobalPos{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                POINT nativeLocalPos = nativeGlobalPos;
                ::ScreenToClient(hWnd, &nativeLocalPos);

                RECT clientRect{0, 0, 0, 0};
                ::GetClientRect(hWnd, &clientRect);
                auto clientWidth = RECT_WIDTH(clientRect);
                auto clientHeight = RECT_HEIGHT(clientRect);

                QPoint qtScenePos = QHighDpi::fromNativeLocalPosition(point2qpoint(nativeLocalPos),
                                                                      m_windowHandle.data());

                int frameSize = getResizeBorderThickness(hWnd);

                bool isFixedWidth = isHostWidthFixed();
                bool isFixedHeight = isHostHeightFixed();
                bool isFixedSize = isHostSizeFixed();
                bool isInLeftBorder = nativeLocalPos.x <= frameSize;
                bool isInTopBorder = nativeLocalPos.y <= frameSize;
                bool isInRightBorder = nativeLocalPos.x > clientWidth - frameSize;
                bool isInBottomBorder = nativeLocalPos.y > clientHeight - frameSize;
                bool isInTitleBar = isInTitleBarDraggableArea(qtScenePos);
                WindowAgentBase::SystemButton sysButtonType = WindowAgentBase::Unknown;
                bool isInCaptionButtons = isInSystemButtons(qtScenePos, &sysButtonType);
                static constexpr bool dontOverrideCursor = false; // ### TODO

                if (isInCaptionButtons) {
                    // Firstly, we set the hit test result to a default value to be able to detect
                    // whether we have changed it or not afterwards.
                    *result = HTNOWHERE;
                    // Even if the mouse is inside the chrome button area now, we should still allow
                    // the user to be able to resize the window with the top or right window border,
                    // this is also the normal behavior of a native Win32 window (but only when the
                    // window is not maximized/fullscreen/minimized, of course).
                    if (isWindowNoState(hWnd)) {
                        static constexpr const quint8 kBorderSize = 2;
                        bool isTop = nativeLocalPos.y <= kBorderSize;
                        bool isLeft = nativeLocalPos.x <= kBorderSize;
                        bool isRight = nativeLocalPos.x > clientWidth - kBorderSize;
                        if (isTop || isLeft || isRight) {
                            if (isFixedSize || dontOverrideCursor) {
                                // The user doesn't want the window to be resized, so we tell
                                // Windows we are in the client area so that the controls beneath
                                // the mouse cursor can still be hovered or clicked.
                                *result = isInTitleBar ? HTCAPTION : HTCLIENT;
                            } else {
                                if (isTop) {
                                    if (isLeft) {
                                        if (isFixedWidth) {
                                            *result = HTTOP;
                                        } else if (isFixedHeight) {
                                            *result = HTLEFT;
                                        } else {
                                            *result = HTTOPLEFT;
                                        }
                                    } else if (isRight) {
                                        if (isFixedWidth) {
                                            *result = HTTOP;
                                        } else if (isFixedHeight) {
                                            *result = HTRIGHT;
                                        } else {
                                            *result = HTTOPRIGHT;
                                        }
                                    } else {
                                        *result = isFixedHeight ? HTBORDER : HTTOP;
                                    }
                                } else {
                                    if (isFixedWidth) {
                                        *result = HTBORDER;
                                    } else {
                                        *result = isLeft ? HTLEFT : HTRIGHT;
                                    }
                                }
                            }
                        }
                    }
                    if (*result == HTNOWHERE) {
                        // OK, we are now really inside one of the chrome buttons, tell Windows the
                        // exact role of our button. The Snap Layout feature introduced in Windows
                        // 11 won't work without this.
                        switch (sysButtonType) {
                            case WindowAgentBase::WindowIcon:
                                *result = HTSYSMENU;
                                break;
                            case WindowAgentBase::Help:
                                *result = HTHELP;
                                break;
                            case WindowAgentBase::Minimize:
                                *result = HTREDUCE;
                                break;
                            case WindowAgentBase::Maximize:
                                *result = HTZOOM;
                                break;
                            case WindowAgentBase::Close:
                                *result = HTCLOSE;
                                break;
                            default:
                                // unreachable
                                break;
                        }
                    }
                    if (*result == HTNOWHERE) {
                        // OK, it seems we are not inside the window resize area, nor inside the
                        // chrome buttons, tell Windows we are in the client area to let Qt handle
                        // this event.
                        *result = HTCLIENT;
                    }
                    return true;
                }
                // OK, we are not inside any chrome buttons, try to find out which part of the
                // window are we hitting.

                bool max = isMaximized(hWnd);
                bool full = isFullScreen(hWnd);

                if (isSystemBorderEnabled()) {
                    // This will handle the left, right and bottom parts of the frame
                    // because we didn't change them.
                    LRESULT originalHitTestResult = ::DefWindowProcW(hWnd, WM_NCHITTEST, 0, lParam);
                    if (originalHitTestResult != HTCLIENT) {
                        // Even if the window is not resizable, we still can't return HTCLIENT here
                        // because when we enter this code path, it means the mouse cursor is
                        // outside the window, that is, the three transparent window resize area.
                        // Returning HTCLIENT will confuse Windows, we can't put our controls there
                        // anyway.
                        *result = HTNOWHERE; // Make sure we can know we don't set any value
                                             // explicitly later.
                        if (originalHitTestResult == HTCAPTION) {
                        } else if (isFixedSize || dontOverrideCursor) {
                            *result = HTBORDER;
                        } else if (isFixedWidth || isFixedHeight) {
                            if (originalHitTestResult == HTTOPLEFT) {
                                if (isFixedWidth) {
                                    *result = HTTOP;
                                } else {
                                    *result = HTLEFT;
                                }
                            } else if (originalHitTestResult == HTTOPRIGHT) {
                                if (isFixedWidth) {
                                    *result = HTTOP;
                                } else {
                                    *result = HTRIGHT;
                                }
                            } else if (originalHitTestResult == HTBOTTOMRIGHT) {
                                if (isFixedWidth) {
                                    *result = HTBOTTOM;
                                } else {
                                    *result = HTRIGHT;
                                }
                            } else if (originalHitTestResult == HTBOTTOMLEFT) {
                                if (isFixedWidth) {
                                    *result = HTBOTTOM;
                                } else {
                                    *result = HTLEFT;
                                }
                            } else if (originalHitTestResult == HTLEFT ||
                                       originalHitTestResult == HTRIGHT) {
                                if (isFixedWidth) {
                                    *result = HTBORDER;
                                }
                            } else if (originalHitTestResult == HTTOP ||
                                       originalHitTestResult == HTBOTTOM) {
                                if (isFixedHeight) {
                                    *result = HTBORDER;
                                }
                            }
                        }
                        if (*result == HTNOWHERE) {
                            *result = originalHitTestResult;
                        }
                        return true;
                    }
                    if (full) {
                        *result = HTCLIENT;
                        return true;
                    }
                    if (max) {
                        *result = isInTitleBar ? HTCAPTION : HTCLIENT;
                        return true;
                    }
                    // At this point, we know that the cursor is inside the client area,
                    // so it has to be either the little border at the top of our custom
                    // title bar or the drag bar. Apparently, it must be the drag bar or
                    // the little border at the top which the user can use to move or
                    // resize the window.
                    if (isInTopBorder) {
                        // Return HTCLIENT instead of HTBORDER here, because the mouse is
                        // inside our homemade title bar now, return HTCLIENT to let our
                        // title bar can still capture mouse events.
                        *result = [&]() {
                            if (isFixedSize || isFixedHeight || dontOverrideCursor ||
                                (isFixedWidth && (isInLeftBorder || isInRightBorder))) {
                                if (isInTitleBar) {
                                    return HTCAPTION;
                                } else {
                                    return HTCLIENT;
                                }
                            } else {
                                return HTTOP;
                            }
                        }();
                        return true;
                    }
                    if (isInTitleBar) {
                        *result = HTCAPTION;
                        return true;
                    }
                    *result = HTCLIENT;
                    return true;
                } else {
                    if (full) {
                        *result = HTCLIENT;
                        return true;
                    }
                    if (max || isFixedSize || dontOverrideCursor) {
                        *result = isInTitleBar ? HTCAPTION : HTCLIENT;
                        return true;
                    }
                    if (isFixedWidth || isFixedHeight) {
                        if (isInLeftBorder && isInTopBorder) {
                            if (isFixedWidth) {
                                *result = HTTOP;
                            } else {
                                *result = HTLEFT;
                            }
                        } else if (isInRightBorder && isInTopBorder) {
                            if (isFixedWidth) {
                                *result = HTTOP;
                            } else {
                                *result = HTRIGHT;
                            }
                        } else if (isInRightBorder && isInBottomBorder) {
                            if (isFixedWidth) {
                                *result = HTBOTTOM;
                            } else {
                                *result = HTRIGHT;
                            }
                        } else if (isInLeftBorder && isInBottomBorder) {
                            if (isFixedWidth) {
                                *result = HTBOTTOM;
                            } else {
                                *result = HTLEFT;
                            }
                        } else if (isInLeftBorder || isInRightBorder) {
                            if (isFixedWidth) {
                                *result = HTCLIENT;
                            } else {
                                *result = isInLeftBorder ? HTLEFT : HTRIGHT;
                            }
                        } else if (isInTopBorder || isInBottomBorder) {
                            if (isFixedHeight) {
                                *result = HTCLIENT;
                            } else {
                                *result = isInTopBorder ? HTTOP : HTBOTTOM;
                            }
                        } else {
                            *result = HTCLIENT;
                        }
                        return true;
                    } else {
                        if (isInTopBorder) {
                            if (isInLeftBorder) {
                                *result = HTTOPLEFT;
                                return true;
                            }
                            if (isInRightBorder) {
                                *result = HTTOPRIGHT;
                                return true;
                            }
                            *result = HTTOP;
                            return true;
                        }
                        if (isInBottomBorder) {
                            if (isInLeftBorder) {
                                *result = HTBOTTOMLEFT;
                                return true;
                            }
                            if (isInRightBorder) {
                                *result = HTBOTTOMRIGHT;
                                return true;
                            }
                            *result = HTBOTTOM;
                            return true;
                        }
                        if (isInLeftBorder) {
                            *result = HTLEFT;
                            return true;
                        }
                        if (isInRightBorder) {
                            *result = HTRIGHT;
                            return true;
                        }
                    }
                    if (isInTitleBar) {
                        *result = HTCAPTION;
                        return true;
                    }
                    *result = HTCLIENT;
                    return true;
                }
            }

            case WM_WINDOWPOSCHANGING: {
                // ### FIXME: How does this problem happen and why is it solved?
                // When toggling the "Show theme color in title bar and window border" setting in
                // Windows Settings, or calling `DrawMenuBar()`, Windows sends a message of
                // WM_WINDOWPOSCHANGING with flags 0x37. If we do not process this message,
                // the client area as a whole will shift to the left, which looks very abnormal if
                // we don't repaint it. This exception disappears if we add SWP_NOCOPYBITS flag.
                // But I don't know what caused the problem, or why this would solve it.
                static constexpr const auto kBadWindowPosFlag =
                    SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED;
                const auto windowPos = reinterpret_cast<LPWINDOWPOS>(lParam);
                if (windowPos->flags == kBadWindowPosFlag) {
                    windowPos->flags |= SWP_NOCOPYBITS;
                }
                break;
            }

            case WM_SHOWWINDOW: {
                if (!wParam || !isWindowNoState(hWnd) || isFullScreen(hWnd)) {
                    break;
                }
                RECT windowRect{};
                ::GetWindowRect(hWnd, &windowRect);
                static constexpr const auto swpFlags = SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOOWNERZORDER;
                ::SetWindowPos(hWnd, nullptr, 0, 0, RECT_WIDTH(windowRect) + 1, RECT_HEIGHT(windowRect) + 1, swpFlags);
                ::SetWindowPos(hWnd, nullptr, 0, 0, RECT_WIDTH(windowRect), RECT_HEIGHT(windowRect), swpFlags);
                break;
            }

            default:
                break;
        }

        if (!isSystemBorderEnabled()) {
            switch (message) {
                case WM_NCUAHDRAWCAPTION:
                case WM_NCUAHDRAWFRAME: {
                    // These undocumented messages are sent to draw themed window
                    // borders. Block them to prevent drawing borders over the client
                    // area.
                    *result = FALSE;
                    return true;
                }
                case WM_NCPAINT: {
                    // 边框阴影处于非客户区的范围，因此如果直接阻止非客户区的绘制，会导致边框阴影丢失

                    if (!isDwmCompositionEnabled()) {
                        // Only block WM_NCPAINT when DWM composition is disabled. If
                        // it's blocked when DWM composition is enabled, the frame
                        // shadow won't be drawn.
                        *result = FALSE;
                        return true;
                    } else {
                        break;
                    }
                }
                case WM_NCACTIVATE: {
                    if (isDwmCompositionEnabled()) {
                        // DefWindowProc won't repaint the window border if lParam (normally a HRGN)
                        // is -1. See the following link's "lParam" section:
                        // https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-ncactivate
                        // Don't use "*result = 0" here, otherwise the window won't respond to the
                        // window activation state change.
                        *result = ::DefWindowProcW(hWnd, WM_NCACTIVATE, wParam, -1);
                    } else {
                        *result = TRUE;
                    }
                    return true;
                }
                case WM_SETICON:
                case WM_SETTEXT: {
                    // Disable painting while these messages are handled to prevent them
                    // from drawing a window caption over the client area.
                    const auto oldStyle = static_cast<DWORD>(::GetWindowLongPtrW(hWnd, GWL_STYLE));
                    // Prevent Windows from drawing the default title bar by temporarily
                    // toggling the WS_VISIBLE style.
                    const DWORD newStyle = (oldStyle & ~WS_VISIBLE);
                    ::SetWindowLongPtrW(hWnd, GWL_STYLE, static_cast<LONG_PTR>(newStyle));
                    triggerFrameChange(hWnd);
                    const LRESULT originalResult = ::DefWindowProcW(hWnd, message, wParam, lParam);
                    ::SetWindowLongPtrW(hWnd, GWL_STYLE, static_cast<LONG_PTR>(oldStyle));
                    triggerFrameChange(hWnd);
                    *result = originalResult;
                    return true;
                }
                default:
                    break;
            }
        }
        return false;
    }

    bool Win32WindowContext::nonClientCalcSizeHandler(HWND hWnd, UINT message, WPARAM wParam,
                                                      LPARAM lParam, LRESULT *result) {
        Q_UNUSED(message)
        Q_UNUSED(this)

        // Windows是根据这个消息的返回值来设置窗口的客户区（窗口中真正显示的内容）
        // 和非客户区（标题栏、窗口边框、菜单栏和状态栏等Windows系统自行提供的部分
        // ，不过对于Qt来说，除了标题栏和窗口边框，非客户区基本也都是自绘的）的范
        // 围的，lParam里存放的就是新客户区的几何区域，默认是整个窗口的大小，正常
        // 的程序需要修改这个参数，告知系统窗口的客户区和非客户区的范围（一般来说可
        // 以完全交给Windows，让其自行处理，使用默认的客户区和非客户区），因此如果
        // 我们不修改lParam，就可以使客户区充满整个窗口，从而去掉标题栏和窗口边框
        // （因为这些东西都被客户区给盖住了。但边框阴影也会因此而丢失，不过我们会使
        // 用其他方式将其带回，请参考其他消息的处理，此处不过多提及）。但有个情况要
        // 特别注意，那就是窗口最大化后，窗口的实际尺寸会比屏幕的尺寸大一点，从而使
        // 用户看不到窗口的边界，这样用户就不能在窗口最大化后调整窗口的大小了（虽然
        // 这个做法听起来特别奇怪，但Windows确实就是这样做的），因此如果我们要自行
        // 处理窗口的非客户区，就要在窗口最大化后，将窗口边框的宽度和高度（一般是相
        // 等的）从客户区裁剪掉，否则我们窗口所显示的内容就会超出屏幕边界，显示不全。
        // 如果用户开启了任务栏自动隐藏，在窗口最大化后，还要考虑任务栏的位置。因为
        // 如果窗口最大化后，其尺寸和屏幕尺寸相等（因为任务栏隐藏了，所以窗口最大化
        // 后其实是充满了整个屏幕，变相的全屏了），Windows会认为窗口已经进入全屏的
        // 状态，从而导致自动隐藏的任务栏无法弹出。要避免这个状况，就要使窗口的尺寸
        // 小于屏幕尺寸。我下面的做法参考了火狐、Chromium和Windows Terminal
        // 如果没有开启任务栏自动隐藏，是不存在这个问题的，所以要先进行判断。
        // 一般情况下，*result设置为0（相当于DefWindowProc的返回值为0）就可以了，
        // 根据MSDN的说法，返回0意为此消息已经被程序自行处理了，让Windows跳过此消
        // 息，否则Windows会添加对此消息的默认处理，对于当前这个消息而言，就意味着
        // 标题栏和窗口边框又会回来，这当然不是我们想要的结果。根据MSDN，当wParam
        // 为FALSE时，只能返回0，但当其为TRUE时，可以返回0，也可以返回一个WVR_常
        // 量。根据Chromium的注释，当存在非客户区时，如果返回WVR_REDRAW会导致子
        // 窗口/子控件出现奇怪的bug（自绘控件错位），并且Lucas在Windows 10
        // 上成功复现，说明这个bug至今都没有解决。我查阅了大量资料，发现唯一的解决
        // 方案就是返回0。但如果不存在非客户区，且wParam为TRUE，最好返回
        // WVR_REDRAW，否则窗口在调整大小可能会产生严重的闪烁现象。
        // 虽然对大多数消息来说，返回0都代表让Windows忽略此消息，但实际上不同消息
        // 能接受的返回值是不一样的，请注意自行查阅MSDN。

        // Sent when the size and position of a window's client area must be
        // calculated. By processing this message, an application can
        // control the content of the window's client area when the size or
        // position of the window changes. If wParam is TRUE, lParam points
        // to an NCCALCSIZE_PARAMS structure that contains information an
        // application can use to calculate the new size and position of the
        // client rectangle. If wParam is FALSE, lParam points to a RECT
        // structure. On entry, the structure contains the proposed window
        // rectangle for the window. On exit, the structure should contain
        // the screen coordinates of the corresponding window client area.
        // The client area is the window's content area, the non-client area
        // is the area which is provided by the system, such as the title
        // bar, the four window borders, the frame shadow, the menu bar, the
        // status bar, the scroll bar, etc. But for Qt, it draws most of the
        // window area (client + non-client) itself. We now know that the
        // title bar and the window frame is in the non-client area, and we
        // can set the scope of the client area in this message, so we can
        // remove the title bar and the window frame by let the non-client
        // area be covered by the client area (because we can't really get
        // rid of the non-client area, it will always be there, all we can
        // do is to hide it) , which means we should let the client area's
        // size the same with the whole window's size. So there is no room
        // for the non-client area and then the user won't be able to see it
        // again. But how to achieve this? Very easy, just leave lParam (the
        // re-calculated client area) untouched. But of course you can
        // modify lParam, then the non-client area will be seen and the
        // window borders and the window frame will show up. However, things
        // are quite different when you try to modify the top margin of the
        // client area. DWM will always draw the whole title bar no matter
        // what margin value you set for the top, unless you don't modify it
        // and remove the whole top area (the title bar + the one pixel
        // height window border). This can be confirmed in Windows
        // Terminal's source code, you can also try yourself to verify
        // it. So things will become quite complicated if you want to
        // preserve the four window borders.

        // If `wParam` is `FALSE`, `lParam` points to a `RECT` that contains
        // the proposed window rectangle for our window. During our
        // processing of the `WM_NCCALCSIZE` message, we are expected to
        // modify the `RECT` that `lParam` points to, so that its value upon
        // our return is the new client area. We must return 0 if `wParam`
        // is `FALSE`.
        // If `wParam` is `TRUE`, `lParam` points to a `NCCALCSIZE_PARAMS`
        // struct. This struct contains an array of 3 `RECT`s, the first of
        // which has the exact same meaning as the `RECT` that is pointed to
        // by `lParam` when `wParam` is `FALSE`. The remaining `RECT`s, in
        // conjunction with our return value, can
        // be used to specify portions of the source and destination window
        // rectangles that are valid and should be preserved. We opt not to
        // implement an elaborate client-area preservation technique, and
        // simply return 0, which means "preserve the entire old client area
        // and align it with the upper-left corner of our new client area".

        const auto clientRect = wParam ? &(reinterpret_cast<LPNCCALCSIZE_PARAMS>(lParam))->rgrc[0]
                                       : reinterpret_cast<LPRECT>(lParam);
        [[maybe_unused]] const auto &flickerReducer = qScopeGuard([this]() {
            // When we receive this message, it means the window size has changed
            // already, and it seems this message always come before any client
            // area size notifications (eg. WM_WINDOWPOSCHANGED and WM_SIZE). Let
            // D3D/VK paint immediately to let user see the latest result as soon
            // as possible.
            const auto &isTargetSurface = [](const QSurface::SurfaceType st) {
                return st != QSurface::RasterSurface && st != QSurface::OpenGLSurface &&
                       st != QSurface::RasterGLSurface && st != QSurface::OpenVGSurface;
            };
            if (m_windowHandle && isTargetSurface(m_windowHandle->surfaceType()) &&
                isDwmCompositionEnabled() && DynamicApis::instance().pDwmFlush) {
                DynamicApis::instance().pDwmFlush();
            }
        });
        if (isSystemBorderEnabled()) {
            // Store the original top margin before the default window procedure applies the
            // default frame.
            const LONG originalTop = clientRect->top;
            // Apply the default frame because we don't want to remove the whole window
            // frame, we still need the standard window frame (the resizable frame border
            // and the frame shadow) for the left, bottom and right edges. If we return 0
            // here directly, the whole window frame will be removed (which means there will
            // be no resizable frame border and the frame shadow will also disappear), and
            // that's also how most applications customize their title bars on Windows. It's
            // totally OK but since we want to preserve as much original frame as possible,
            // we can't use that solution.
            const LRESULT originalResult = ::DefWindowProcW(hWnd, WM_NCCALCSIZE, wParam, lParam);
            if (originalResult != 0) {
                *result = originalResult;
                return true;
            }
            // Re-apply the original top from before the size of the default frame was
            // applied, and the whole top frame (the title bar and the top border) is gone
            // now. For the top frame, we only has 2 choices: (1) remove the top frame
            // entirely, or (2) don't touch it at all. We can't preserve the top border by
            // adjusting the top margin here. If we try to modify the top margin, the
            // original title bar will always be painted by DWM regardless what margin we
            // set, so here we can only remove the top frame entirely and use some special
            // technique to bring the top border back.
            clientRect->top = originalTop;
        }

        const bool max = isMaximized(hWnd);
        const bool full = isFullScreen(hWnd);
        // We don't need this correction when we're fullscreen. We will
        // have the WS_POPUP size, so we don't have to worry about
        // borders, and the default frame will be fine.
        if (max && !full) {
            // When a window is maximized, its size is actually a little bit more
            // than the monitor's work area. The window is positioned and sized in
            // such a way that the resize handles are outside the monitor and
            // then the window is clipped to the monitor so that the resize handle
            // do not appear because you don't need them (because you can't resize
            // a window when it's maximized unless you restore it).
            const quint32 frameSize = getResizeBorderThickness(hWnd);
            clientRect->top += frameSize;
            if (!isSystemBorderEnabled()) {
                clientRect->bottom -= frameSize;
                clientRect->left += frameSize;
                clientRect->right -= frameSize;
            }
        }
        // Attempt to detect if there's an autohide taskbar, and if
        // there is, reduce our size a bit on the side with the taskbar,
        // so the user can still mouse-over the taskbar to reveal it.
        // Make sure to use MONITOR_DEFAULTTONEAREST, so that this will
        // still find the right monitor even when we're restoring from
        // minimized.
        if (max || full) {
            APPBARDATA abd{};
            abd.cbSize = sizeof(abd);
            const UINT taskbarState = ::SHAppBarMessage(ABM_GETSTATE, &abd);
            // First, check if we have an auto-hide taskbar at all:
            if (taskbarState & ABS_AUTOHIDE) {
                bool top = false, bottom = false, left = false, right = false;
                // Due to ABM_GETAUTOHIDEBAREX was introduced in Windows 8.1,
                // we have to use another way to judge this if we are running
                // on Windows 7 or Windows 8.
                if (isWin8Point1OrGreater()) {
                    const RECT monitorRect = getMonitorForWindow(hWnd).rcMonitor;
                    // This helper can be used to determine if there's an
                    // auto-hide taskbar on the given edge of the monitor
                    // we're currently on.
                    const auto hasAutohideTaskbar = [monitorRect](const UINT edge) -> bool {
                        APPBARDATA abd2{};
                        abd2.cbSize = sizeof(abd2);
                        abd2.uEdge = edge;
                        abd2.rc = monitorRect;
                        const auto hTaskbar =
                            reinterpret_cast<HWND>(::SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &abd2));
                        return (hTaskbar != nullptr);
                    };
                    top = hasAutohideTaskbar(ABE_TOP);
                    bottom = hasAutohideTaskbar(ABE_BOTTOM);
                    left = hasAutohideTaskbar(ABE_LEFT);
                    right = hasAutohideTaskbar(ABE_RIGHT);
                } else {
                    int edge = -1;
                    APPBARDATA abd2{};
                    abd2.cbSize = sizeof(abd2);
                    abd2.hWnd = ::FindWindowW(L"Shell_TrayWnd", nullptr);
                    HMONITOR windowMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
                    HMONITOR taskbarMonitor =
                        ::MonitorFromWindow(abd2.hWnd, MONITOR_DEFAULTTOPRIMARY);
                    if (taskbarMonitor == windowMonitor) {
                        ::SHAppBarMessage(ABM_GETTASKBARPOS, &abd2);
                        edge = int(abd2.uEdge);
                    }
                    top = (edge == ABE_TOP);
                    bottom = (edge == ABE_BOTTOM);
                    left = (edge == ABE_LEFT);
                    right = (edge == ABE_RIGHT);
                }
                // If there's a taskbar on any side of the monitor, reduce
                // our size a little bit on that edge.
                // Note to future code archeologists:
                // This doesn't seem to work for fullscreen on the primary
                // display. However, testing a bunch of other apps with
                // fullscreen modes and an auto-hiding taskbar has
                // shown that _none_ of them reveal the taskbar from
                // fullscreen mode. This includes Edge, Firefox, Chrome,
                // Sublime Text, PowerPoint - none seemed to support this.
                // This does however work fine for maximized.
                if (top) {
                    // Peculiarly, when we're fullscreen,
                    clientRect->top += kAutoHideTaskBarThickness;
                } else if (bottom) {
                    clientRect->bottom -= kAutoHideTaskBarThickness;
                } else if (left) {
                    clientRect->left += kAutoHideTaskBarThickness;
                } else if (right) {
                    clientRect->right -= kAutoHideTaskBarThickness;
                }
            }
        }
        // By returning WVR_REDRAW we can make the window resizing look
        // less broken. But we must return 0 if wParam is FALSE, according to Microsoft
        // Docs.
        // **IMPORTANT NOTE**:
        // If you are drawing something manually through D3D in your window, don't
        // try to return WVR_REDRAW here, otherwise Windows exhibits bugs where
        // client pixels and child windows are mispositioned by the width/height
        // of the upper-left non-client area. It's confirmed that this issue exists
        // from Windows 7 to Windows 10. Not tested on Windows 11 yet. Don't know
        // whether it exists on Windows XP to Windows Vista or not.

        // https://github.com/chromium/chromium/blob/5d297da3cf2a642e9ace2b23fed097370bc70814/ui/views/win/hwnd_message_handler.cc#L2330
        // Do not return WVR_REDRAW otherwise child HWNDs will be mispositioned.
        *result = FALSE;
        return true;
    }

    bool Win32WindowContext::systemMenuHandler(HWND hWnd, UINT message, WPARAM wParam,
                                               LPARAM lParam, LRESULT *result) {
        const auto getNativePosFromMouse = [lParam]() -> POINT {
            return {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        };
        const auto getNativeGlobalPosFromKeyboard = [hWnd]() -> POINT {
            const bool maxOrFull = isMaximized(hWnd) || isFullScreen(hWnd);
            const quint32 frameSize = getResizeBorderThickness(hWnd);
            const quint32 horizontalOffset =
                ((maxOrFull || !isSystemBorderEnabled()) ? 0 : frameSize);
            const auto verticalOffset = [hWnd, maxOrFull, frameSize]() -> quint32 {
                const quint32 titleBarHeight = getTitleBarHeight(hWnd);
                if (!isSystemBorderEnabled()) {
                    return titleBarHeight;
                }
                if (isWin11OrGreater()) {
                    if (maxOrFull) {
                        return (titleBarHeight + frameSize);
                    }
                    return titleBarHeight;
                }
                if (maxOrFull) {
                    return titleBarHeight;
                }
                return titleBarHeight - frameSize;
            }();
            RECT windowPos{};
            ::GetWindowRect(hWnd, &windowPos);
            return {static_cast<LONG>(windowPos.left + horizontalOffset),
                    static_cast<LONG>(windowPos.top + verticalOffset)};
        };
        bool shouldShowSystemMenu = false;
        bool broughtByKeyboard = false;
        POINT nativeGlobalPos{};

        switch (message) {
            case WM_RBUTTONUP: {
                const POINT nativeLocalPos = getNativePosFromMouse();
                const QPoint qtScenePos = QHighDpi::fromNativeLocalPosition(
                    point2qpoint(nativeLocalPos), m_windowHandle.data());
                WindowAgentBase::SystemButton sysButtonType = WindowAgentBase::Unknown;
                if (isInTitleBarDraggableArea(qtScenePos) ||
                    (isInSystemButtons(qtScenePos, &sysButtonType) &&
                     sysButtonType == WindowAgentBase::WindowIcon)) {
                    shouldShowSystemMenu = true;
                    nativeGlobalPos = nativeLocalPos;
                    ::ClientToScreen(hWnd, &nativeGlobalPos);
                }
                break;
            }
            case WM_NCRBUTTONUP: {
                if (wParam == HTCAPTION) {
                    shouldShowSystemMenu = true;
                    nativeGlobalPos = getNativePosFromMouse();
                }
                break;
            }
            case WM_SYSCOMMAND: {
                const WPARAM filteredWParam = (wParam & 0xFFF0);
                switch (filteredWParam) {
                    case SC_MOUSEMENU:
                        shouldShowSystemMenu = true;
                        nativeGlobalPos = getNativeGlobalPosFromKeyboard();
                        break;
                    case SC_KEYMENU:
                        if (lParam == VK_SPACE) {
                            shouldShowSystemMenu = true;
                            broughtByKeyboard = true;
                            nativeGlobalPos = getNativeGlobalPosFromKeyboard();
                        }
                        break;
                    default:
                        break;
                }
                break;
            }
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN: {
                const bool altPressed = ((wParam == VK_MENU) || (::GetKeyState(VK_MENU) < 0));
                const bool spacePressed = ((wParam == VK_SPACE) || (::GetKeyState(VK_SPACE) < 0));
                if (altPressed && spacePressed) {
                    shouldShowSystemMenu = true;
                    broughtByKeyboard = true;
                    nativeGlobalPos = getNativeGlobalPosFromKeyboard();
                }
                break;
            }
            default:
                break;
        }
        if (shouldShowSystemMenu) {
            static HHOOK mouseHook = nullptr;
            static std::optional<POINT> mouseClickPos;
            static bool mouseDoubleClicked = false;
            bool mouseHookedLocal = false;

            // The menu is triggered by a click on icon button
            if (iconButtonClickTime > 0) {
                POINT menuPos{0, static_cast<LONG>(getTitleBarHeight(hWnd))};
                if (const auto tb = titleBar()) {
                    auto titleBarHeight = qreal(m_delegate->mapGeometryToScene(tb).height());
                    titleBarHeight *= m_windowHandle->devicePixelRatio();
                    menuPos.y = qRound(titleBarHeight);
                }
                ::ClientToScreen(hWnd, &menuPos);
                nativeGlobalPos = menuPos;

                // Install mouse hook
                if (!mouseHook) {
                    mouseHook = ::SetWindowsHookExW(
                        WH_MOUSE,
                        [](int nCode, WPARAM wParam, LPARAM lParam) -> LRESULT {
                            if (nCode >= 0) {
                                switch (wParam) {
                                    case WM_LBUTTONDBLCLK:
                                        mouseDoubleClicked = true;
                                        Q_FALLTHROUGH();

                                        // case WM_POINTERDOWN:

                                    case WM_LBUTTONDOWN: {
                                        auto pMouseStruct =
                                            reinterpret_cast<MOUSEHOOKSTRUCT *>(lParam);
                                        if (pMouseStruct) {
                                            mouseClickPos = pMouseStruct->pt;
                                        }
                                        break;
                                    }
                                    default:
                                        break;
                                }
                            }
                            return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
                        },
                        nullptr, ::GetCurrentThreadId());
                    mouseHookedLocal = true;
                }
            }

            bool res =
                showSystemMenu_sys(hWnd, nativeGlobalPos, broughtByKeyboard, isHostSizeFixed());

            // Uninstall mouse hook and check if it's a double-click
            if (mouseHookedLocal) {
                ::UnhookWindowsHookEx(mouseHook);

                // Emulate the Windows icon button's behavior
                if (!res && mouseClickPos.has_value()) {
                    POINT nativeLocalPos = mouseClickPos.value();
                    ::ScreenToClient(hWnd, &nativeLocalPos);
                    QPoint qtScenePos = QHighDpi::fromNativeLocalPosition(
                        point2qpoint(nativeLocalPos), m_windowHandle.data());
                    WindowAgentBase::SystemButton sysButtonType = WindowAgentBase::Unknown;
                    if (isInSystemButtons(qtScenePos, &sysButtonType) &&
                        sysButtonType == WindowAgentBase::WindowIcon) {
                        iconButtonClickLevel |= IconButtonClicked;
                        if (::GetTickCount64() - iconButtonClickTime <= ::GetDoubleClickTime()) {
                            iconButtonClickLevel |= IconButtonTriggersClose;
                        }
                    }
                }

                if (mouseDoubleClicked) {
                    iconButtonClickLevel |= IconButtonDoubleClicked;
                }

                mouseHook = nullptr;
                mouseClickPos.reset();
                mouseDoubleClicked = false;
            }

            // QPA's internal code will handle system menu events separately, and its
            // behavior is not what we would want to see because it doesn't know our
            // window doesn't have any window frame now, so return early here to avoid
            // entering Qt's own handling logic.
            *result = FALSE;
            return true;
        }
        return false;
    }

}
