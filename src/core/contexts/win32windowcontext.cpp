#include "win32windowcontext_p.h"

#include <QtCore/QHash>

namespace QWK {

    using WndProcHash = QHash<HWND, Win32WindowContext *>; // hWnd -> context
    Q_GLOBAL_STATIC(WndProcHash, g_wndProcHash);

    static WNDPROC g_qtWindowProc = nullptr; // Original Qt window proc function

    extern "C" LRESULT QT_WIN_CALLBACK QWK_WindowsWndProc(HWND hWnd, UINT message, WPARAM wParam,
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

        // Try hooked procedure
        LRESULT result;
        if (ctx->windowProc(hWnd, message, wParam, lParam, &result)) {
            return result;
        }

        // Fallback to Qt's procedure
        return ::CallWindowProcW(g_qtWindowProc, hWnd, message, wParam, lParam);
    }

    Win32WindowContext::Win32WindowContext(QWindow *window, WindowItemDelegatePtr delegate)
        : AbstractWindowContext(window, std::move(delegate)), windowId(0) {
    }

    Win32WindowContext::~Win32WindowContext() {
        // Remove window handle mapping
        auto hWnd = reinterpret_cast<HWND>(windowId);
        g_wndProcHash->remove(hWnd);
    }

    bool Win32WindowContext::setup() {
        auto winId = m_windowHandle->winId();

        // Install window hook
        auto hWnd = reinterpret_cast<HWND>(winId);
        auto qtWindowProc = reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(hWnd, GWLP_WNDPROC));
        ::SetWindowLongPtrW(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(QWK_WindowsWndProc));

        windowId = winId;

        // Store original window proc
        if (!g_qtWindowProc) {
            g_qtWindowProc = qtWindowProc;
        }

        // Save window handle mapping
        g_wndProcHash->insert(hWnd, this);

        return true;
    }

    bool Win32WindowContext::windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                                        LRESULT *result) {
        *result = FALSE;

        // TODO: Implement
        // ...

        return false; // Not handled
    }

}