#include "win32windowcontext_p.h"

#include <QtCore/QHash>
#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>

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
        : AbstractWindowContext(window, delegate), windowId(0) {
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

        Q_UNUSED(windowId)

        // TODO: Implement
        // ...

        return false; // Not handled
    }

}