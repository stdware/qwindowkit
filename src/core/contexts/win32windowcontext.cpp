#include "win32windowcontext_p.h"

#include <QtCore/QHash>

namespace QWK {

    using WndProcHash = QHash<HWND, Win32WindowContext *>;
    Q_GLOBAL_STATIC(WndProcHash, g_wndProcHash);

    Win32WindowContext::Win32WindowContext(QWindow *window, WindowItemDelegate *delegate)
        : AbstractWindowContext(window, delegate), windowId(0), qtWindowProc(nullptr) {
    }

    Win32WindowContext::~Win32WindowContext() {
        auto hWnd = reinterpret_cast<HWND>(windowId);
        g_wndProcHash->remove(hWnd);
    }

    bool Win32WindowContext::setup() {
        auto winId = m_windowHandle->winId();
        Q_ASSERT(winId);
        if (!winId) {
            return false;
        }

        // Install window hook
        auto hWnd = reinterpret_cast<HWND>(winId);
        auto orgWndProc = reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(hWnd, GWLP_WNDPROC));
        Q_ASSERT(orgWndProc);
        if (!orgWndProc) {
            QWK_WARNING << winLastErrorMessage();
            return false;
        }

        if (::SetWindowLongPtrW(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(windowProc)) == 0) {
            QWK_WARNING << winLastErrorMessage();
            return false;
        }

        windowId = winId;
        qtWindowProc = orgWndProc;         // Store original window proc
        g_wndProcHash->insert(hWnd, this); // Save window handle mapping
        return true;
    }

    LRESULT Win32WindowContext::windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        Q_ASSERT(hWnd);
        if (!hWnd) {
            return FALSE;
        }

        const auto *ctx = g_wndProcHash->value(hWnd);
        if (!ctx) {
            return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }

        auto winId = reinterpret_cast<WId>(hWnd);

        // Further procedure
        Q_UNUSED(winId)

        return ::CallWindowProcW(ctx->qtWindowProc, hWnd, uMsg, wParam, lParam);
    }

}