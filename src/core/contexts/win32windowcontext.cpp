#include "win32windowcontext_p.h"

#include <optional>

#include <QtCore/QHash>
#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>

#include <QtGui/private/qhighdpiscaling_p.h>

#include "qwkcoreglobal_p.h"

namespace QWK {

    using WndProcHash = QHash<HWND, Win32WindowContext *>; // hWnd -> context
    Q_GLOBAL_STATIC(WndProcHash, g_wndProcHash);

    static WNDPROC g_qtWindowProc = nullptr; // Original Qt window proc function

    static bool g_lastMessageHandled = false;

    static LRESULT g_lastMessageResult = false;

    class WindowsNativeEventFilter : public QAbstractNativeEventFilter {
    public:
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            if (g_lastMessageHandled) {
                *result = static_cast<QT_NATIVE_EVENT_RESULT_TYPE>(g_lastMessageResult);
                return true;
            }
            return false;
        }
    };

    static WindowsNativeEventFilter *g_nativeFilter = nullptr;

    static inline QPoint fromNativeLocalPosition(const QWindow *window, const QPoint &point) {
#if 1
        return QHighDpi::fromNativeLocalPosition(point, window);
#else
        return QPointF(QPointF(point) / window->devicePixelRatio()).toPoint();
#endif
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

    static bool isValidWindow(WId windowId, bool checkVisible, bool checkTopLevel) {
        const auto hwnd = reinterpret_cast<HWND>(windowId);
        if (::IsWindow(hwnd) == FALSE) {
            return false;
        }
        const LONG_PTR styles = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
        if ((styles == 0) || (styles & WS_DISABLED)) {
            return false;
        }
        const LONG_PTR exStyles = ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        if ((exStyles == 0) || (exStyles & WS_EX_TOOLWINDOW)) {
            return false;
        }
        RECT rect = {0, 0, 0, 0};
        if (::GetWindowRect(hwnd, &rect) == FALSE) {
            return false;
        }
        if ((rect.left >= rect.right) || (rect.top >= rect.bottom)) {
            return false;
        }
        if (checkVisible) {
            if (::IsWindowVisible(hwnd) == FALSE) {
                return false;
            }
        }
        if (checkTopLevel) {
            if (::GetAncestor(hwnd, GA_ROOT) != hwnd) {
                return false;
            }
        }
        return true;
    }

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

        // Try hooked procedure and save result
        g_lastMessageHandled = ctx->windowProc(hWnd, message, wParam, lParam, &g_lastMessageResult);

        // TODO: Determine whether to show system menu
        // ...

        // Since Qt does the necessary processing of the message afterward, we still need to
        // continue dispatching it.
        return ::CallWindowProcW(g_qtWindowProc, hWnd, message, wParam, lParam);
    }

    Win32WindowContext::Win32WindowContext(QWindow *window, WindowItemDelegate *delegate)
        : AbstractWindowContext(window, delegate) {
    }

    Win32WindowContext::~Win32WindowContext() {
        // Remove window handle mapping
        if (auto hWnd = reinterpret_cast<HWND>(windowId); hWnd) {
            g_wndProcHash->remove(hWnd);

            // Remove event filter if the last window is destroyed
            if (g_wndProcHash->empty()) {
                qApp->removeNativeEventFilter(g_nativeFilter);
                delete g_nativeFilter;
                g_nativeFilter = nullptr;
            }
        }
    }

    bool Win32WindowContext::setup() {
        auto winId = m_windowHandle->winId();

        // Install window hook
        auto hWnd = reinterpret_cast<HWND>(winId);

        // Store original window proc
        if (!g_qtWindowProc) {
            g_qtWindowProc = reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(hWnd, GWLP_WNDPROC));
        }

        // Hook window proc
        ::SetWindowLongPtrW(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(QWKHookedWndProc));

        // Install global native event filter
        if (!g_nativeFilter) {
            g_nativeFilter = new WindowsNativeEventFilter();
            qApp->installNativeEventFilter(g_nativeFilter);
        }

        // Cache window ID
        windowId = winId;

        // Save window handle mapping
        g_wndProcHash->insert(hWnd, this);

        return true;
    }

    bool Win32WindowContext::windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                                        LRESULT *result) {
        *result = FALSE;

        // We should skip these messages otherwise we will get crashes.
        // NOTE: WM_QUIT won't be posted to the WindowProc function.
        switch (message) {
            case WM_CLOSE:
            case WM_DESTROY:
            case WM_NCDESTROY:
            // Undocumented messages:
            case WM_UAHDESTROYWINDOW:
            case WM_UNREGISTER_WINDOW_SERVICES:
                return false;
            default:
                break;
        }

        if (!isValidWindow(windowId, false, true)) {
            return false;
        }

        // Test snap layout
        if (snapLayoutHandler(hWnd, message, wParam, lParam, result)) {
            return true;
        }

        // TODO: Uncomment and do something
        // bool frameBorderVisible = Utils::isWindowFrameBorderVisible();

        // TODO: Implement
        // ...

        return false; // Not handled
    }

    static constexpr const auto kMessageTag = WPARAM(0x97CCEA99);

    static inline constexpr bool isTaggedMessage(WPARAM wParam) {
        return (wParam == kMessageTag);
    }

    static quint64 getKeyState() {
        quint64 result = 0;
        const auto &get = [](const int virtualKey) -> bool {
            return (::GetAsyncKeyState(virtualKey) < 0);
        };
        const bool buttonSwapped = (::GetSystemMetrics(SM_SWAPBUTTON) != FALSE);
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
                return kMessageTag;
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
            if (::ScreenToClient(hWnd, &clientPos) == FALSE) {
                return 0;
            }
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
                break;
        }

#undef SEND_MESSAGE
    }

    static bool requestForMouseLeaveMessage(HWND hWnd, bool nonClient) {
        TRACKMOUSEEVENT tme;
        SecureZeroMemory(&tme, sizeof(tme));
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        if (nonClient) {
            tme.dwFlags |= TME_NONCLIENT;
        }
        tme.hwndTrack = hWnd;
        tme.dwHoverTime = HOVER_DEFAULT;
        if (::TrackMouseEvent(&tme) == FALSE) {
            return false;
        }
        return true;
    }

    bool Win32WindowContext::snapLayoutHandler(HWND hWnd, UINT message, WPARAM wParam,
                                                     LPARAM lParam, LRESULT *result) {
        switch (message) {
            case WM_MOUSELEAVE: {
                if (!isTaggedMessage(wParam)) {
                    // Qt will call TrackMouseEvent() to get the WM_MOUSELEAVE message when it
                    // receives WM_MOUSEMOVE messages, and since we are converting every
                    // WM_NCMOUSEMOVE message to WM_MOUSEMOVE message and send it back to the window
                    // to be able to hover our controls, we also get lots of WM_MOUSELEAVE messages
                    // at the same time because of the reason above, and these superfluous mouse
                    // leave events cause Qt to think the mouse has left the control, and thus we
                    // actually lost the hover state. So we filter out these superfluous mouse leave
                    // events here to avoid this issue.
                    DWORD dwScreenPos = ::GetMessagePos();
                    QPoint qtScenePos =
                        fromNativeLocalPosition(m_windowHandle, QPoint(GET_X_LPARAM(dwScreenPos),
                                                                       GET_Y_LPARAM(dwScreenPos)));
                    auto dummy = CoreWindowAgent::Unknown;
                    if (isInSystemButtons(qtScenePos, &dummy)) {
                        mouseLeaveBlocked = true;
                        *result = FALSE;
                        return true;
                    }
                }
                mouseLeaveBlocked = false;
                break;
            }

            case WM_MOUSEMOVE: {
                if ((lastHitTestResult != WindowPart::ChromeButton) && mouseLeaveBlocked) {
                    mouseLeaveBlocked = false;
                    std::ignore = requestForMouseLeaveMessage(hWnd, false);
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
                const WindowPart currentWindowPart = lastHitTestResult;
                if (message == WM_NCMOUSEMOVE) {
                    if (currentWindowPart != WindowPart::ChromeButton) {
                        std::ignore = m_delegate->resetQtGrabbedControl();
                        if (mouseLeaveBlocked) {
                            emulateClientAreaMessage(hWnd, message, wParam, lParam,
                                                     WM_NCMOUSELEAVE);
                        }
                    }

                    // We need to make sure we get the right hit-test result when a WM_NCMOUSELEAVE
                    // comes, so we reset it when we receive a WM_NCMOUSEMOVE.

                    // If the mouse is entering the client area, there must be a WM_NCHITTEST
                    // setting it to `Client` before the WM_NCMOUSELEAVE comes; If the mouse is
                    // leaving the window, current window part remains as `Outside`.
                    lastHitTestResult = WindowPart::Outside;
                }

                if (currentWindowPart == WindowPart::ChromeButton) {
                    emulateClientAreaMessage(hWnd, message, wParam, lParam);
                    if (message == WM_NCMOUSEMOVE) {
                        // ### FIXME FIXME FIXME
                        // ### FIXME: Calling DefWindowProc() here is really dangerous, investigate
                        // how to avoid doing this.
                        // ### FIXME FIXME FIXME
                        *result = ::DefWindowProcW(hWnd, WM_NCMOUSEMOVE, wParam, lParam);
                    } else {
                        // According to MSDN, we should return non-zero for X button messages to
                        // indicate we have handled these messages (due to historical reasons), for
                        // all other messages we should return zero instead.
                        *result =
                            (((message >= WM_NCXBUTTONDOWN) && (message <= WM_NCXBUTTONDBLCLK))
                                 ? TRUE
                                 : FALSE);
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
                    // in advance.
                    if (mouseLeaveBlocked) {
                        mouseLeaveBlocked = false;
                        std::ignore = requestForMouseLeaveMessage(hWnd, false);
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
                        std::ignore = m_delegate->resetQtGrabbedControl();
                    }
                }
                break;
            }

            default:
                break;
        }
        return false;
    }

}